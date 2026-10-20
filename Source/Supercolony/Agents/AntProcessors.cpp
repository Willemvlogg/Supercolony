#include "AntProcessors.h"
#include "AntFragments.h"
#include "AntRenderActor.h"
#include "AntSettings.h"
#include "AntSteering.h"
#include "Colony/ColonySubsystem.h"
#include "Pheromones/PheromoneGridSubsystem.h"
#include "Mass/EntityFragments.h"
#include "MassExecutionContext.h"

namespace
{
	/** Large frame hitches would otherwise teleport ants through targets and walls. */
	constexpr float MaxDeltaTime = 0.1f;

	float GetDeltaTime(const FMassExecutionContext& Context)
	{
		return FMath::Min(MaxDeltaTime, Context.GetDeltaTimeSeconds());
	}

	EPheromoneChannel GetFollowedChannel(EAntState State)
	{
		return State == EAntState::Searching ? EPheromoneChannel::Food : EPheromoneChannel::Home;
	}

	EPheromoneChannel GetDepositedChannel(EAntState State)
	{
		return State == EAntState::Searching ? EPheromoneChannel::Home : EPheromoneChannel::Food;
	}

	/** The nearest food source (Searching) or the nest (Returning), if within SightRadius. */
	bool FindGoalInSight(EAntState State, const FVector2D& Position, const UColonySubsystem& Colony, float SightRadius, FVector2D& OutGoal)
	{
		float BestDistSquared = FMath::Square(SightRadius);
		bool bFound = false;

		if (State == EAntState::Returning)
		{
			const float DistSquared = FVector2D::DistSquared(Position, Colony.GetNestLocation());
			if (DistSquared <= BestDistSquared)
			{
				OutGoal = Colony.GetNestLocation();
				bFound = true;
			}
			return bFound;
		}

		for (const FVector2D& Food : Colony.GetFoodLocations())
		{
			const float DistSquared = FVector2D::DistSquared(Position, Food);
			if (DistSquared <= BestDistSquared)
			{
				BestDistSquared = DistSquared;
				OutGoal = Food;
				bFound = true;
			}
		}
		return bFound;
	}

	/** Pick up food or drop it at the nest: switch state, turn around, restart the deposit falloff. */
	void ArriveAtTarget(FAntFragment& Ant, EAntState NewState)
	{
		Ant.State = NewState;
		Ant.Yaw = AntSteering::WrapYaw(Ant.Yaw + UE_PI);
		Ant.TimeSinceTarget = 0.f;
	}
}

//----------------------------------------------------------------------//
// UAntSteerProcessor
//----------------------------------------------------------------------//

UAntSteerProcessor::UAntSteerProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
}

void UAntSteerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FAntFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UAntSteerProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UWorld* World = Context.GetWorld();
	const UPheromoneGridSubsystem* GridSubsystem = World ? World->GetSubsystem<UPheromoneGridSubsystem>() : nullptr;
	const UColonySubsystem* Colony = World ? World->GetSubsystem<UColonySubsystem>() : nullptr;
	if (!GridSubsystem || !Colony || !Colony->HasNest())
	{
		return;
	}

	const FPheromoneGrid& Grid = GridSubsystem->GetGrid();
	const UAntSettings* Settings = GetDefault<UAntSettings>();
	const float DeltaTime = GetDeltaTime(Context);
	const float TurnRate = FMath::DegreesToRadians(Settings->TurnRate);
	const float MaxTurn = TurnRate * DeltaTime;
	const float SensorAngle = FMath::DegreesToRadians(Settings->SensorAngle);

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FAntFragment> Ants = Context.GetMutableFragmentView<FAntFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FAntFragment& Ant = Ants[EntityIt];
			const FVector2D Position(Transforms[EntityIt].GetTransform().GetLocation());

			FVector2D Goal;
			if (FindGoalInSight(Ant.State, Position, *Colony, Settings->SightRadius, Goal))
			{
				const FVector2D ToGoal = Goal - Position;
				Ant.Yaw = AntSteering::TurnToward(Ant.Yaw, FMath::Atan2(ToGoal.Y, ToGoal.X), MaxTurn);
				continue;
			}

			const EPheromoneChannel Channel = GetFollowedChannel(Ant.State);
			FVector2D Left, Ahead, Right;
			AntSteering::GetSensorPositions(Position, Ant.Yaw, SensorAngle, Settings->SensorDistance, Left, Ahead, Right);
			const float Turn = AntSteering::ChooseTurn(Grid.Sample(Left, Channel), Grid.Sample(Ahead, Channel), Grid.Sample(Right, Channel),
				Settings->MinSensorValue);

			Ant.Yaw = AntSteering::WrapYaw(Ant.Yaw + Turn * MaxTurn);
			Ant.Yaw = AntSteering::ApplyWander(Ant.Yaw, Settings->WanderStrength, TurnRate, DeltaTime, Ant.Random);
		}
	});
}

//----------------------------------------------------------------------//
// UAntMoveProcessor
//----------------------------------------------------------------------//

UAntMoveProcessor::UAntMoveProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteAfter.Add(UAntSteerProcessor::StaticClass()->GetFName());
	bRequiresGameThreadExecution = true;
}

void UAntMoveProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FAntFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
}

void UAntMoveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = Context.GetWorld();
	const UPheromoneGridSubsystem* GridSubsystem = World ? World->GetSubsystem<UPheromoneGridSubsystem>() : nullptr;
	UColonySubsystem* Colony = World ? World->GetSubsystem<UColonySubsystem>() : nullptr;
	if (!GridSubsystem || !Colony || !Colony->HasNest())
	{
		return;
	}

	const UAntSettings* Settings = GetDefault<UAntSettings>();
	const float DeltaTime = GetDeltaTime(Context);
	const float Step = Settings->Speed * DeltaTime;
	const float FoodRadiusSquared = FMath::Square(Settings->FoodRadius);
	const float NestRadiusSquared = FMath::Square(Settings->NestRadius);
	const FVector2D NestLocation = Colony->GetNestLocation();
	const TConstArrayView<FVector2D> FoodLocations = Colony->GetFoodLocations();

	const FPheromoneGrid& Grid = GridSubsystem->GetGrid();
	const FVector2D BoundsMin = Grid.GetOrigin() + FVector2D(Settings->BoundsMargin);
	const FVector2D BoundsMax = Grid.GetOrigin() + Grid.GetWorldSize() - FVector2D(Settings->BoundsMargin);

	int32 Delivered = 0;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FAntFragment> Ants = Context.GetMutableFragmentView<FAntFragment>();
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FAntFragment& Ant = Ants[EntityIt];
			FTransform& Transform = Transforms[EntityIt].GetMutableTransform();

			FVector2D Position = FVector2D(Transform.GetLocation()) + AntSteering::HeadingToDirection(Ant.Yaw) * Step;
			AntSteering::KeepInBounds(Position, Ant.Yaw, BoundsMin, BoundsMax);
			Ant.TimeSinceTarget += DeltaTime;

			if (Ant.State == EAntState::Searching)
			{
				for (const FVector2D& Food : FoodLocations)
				{
					if (FVector2D::DistSquared(Position, Food) <= FoodRadiusSquared)
					{
						ArriveAtTarget(Ant, EAntState::Returning);
						break;
					}
				}
			}
			else if (FVector2D::DistSquared(Position, NestLocation) <= NestRadiusSquared)
			{
				ArriveAtTarget(Ant, EAntState::Searching);
				++Delivered;
			}

			Transform.SetLocation(FVector(Position, 0.f));
			Transform.SetRotation(FQuat(FVector::UpVector, Ant.Yaw));
		}
	});

	if (Delivered > 0)
	{
		Colony->AddFoodDelivered(Delivered);
	}
}

//----------------------------------------------------------------------//
// UAntDepositProcessor
//----------------------------------------------------------------------//

UAntDepositProcessor::UAntDepositProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteAfter.Add(UAntMoveProcessor::StaticClass()->GetFName());
	bRequiresGameThreadExecution = true;
}

void UAntDepositProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FAntFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UAntDepositProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = Context.GetWorld();
	UPheromoneGridSubsystem* GridSubsystem = World ? World->GetSubsystem<UPheromoneGridSubsystem>() : nullptr;
	if (!GridSubsystem)
	{
		return;
	}

	const UAntSettings* Settings = GetDefault<UAntSettings>();
	const float DeltaTime = GetDeltaTime(Context);

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FAntFragment> Ants = Context.GetFragmentView<FAntFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FAntFragment& Ant = Ants[EntityIt];
			const float Rate = AntSteering::GetDepositRate(Settings->DepositAmount, Ant.TimeSinceTarget, Settings->DepositFalloffTime,
				Settings->MaxTrailTime);
			if (Rate <= 0.f)
			{
				continue;
			}
			GridSubsystem->Deposit(Transforms[EntityIt].GetTransform().GetLocation(), GetDepositedChannel(Ant.State), Rate * DeltaTime);
		}
	});
}

//----------------------------------------------------------------------//
// UAntRenderProcessor
//----------------------------------------------------------------------//

UAntRenderProcessor::UAntRenderProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteAfter.Add(UAntDepositProcessor::StaticClass()->GetFName());
	bRequiresGameThreadExecution = true;
}

void UAntRenderProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FAntFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UAntRenderProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UWorld* World = Context.GetWorld();
	const UColonySubsystem* Colony = World ? World->GetSubsystem<UColonySubsystem>() : nullptr;
	AAntRenderActor* RenderActor = Colony ? Colony->GetRenderActor() : nullptr;
	if (!RenderActor)
	{
		return;
	}

	const UAntSettings* Settings = GetDefault<UAntSettings>();
	const float Scale = Settings->DotSize / 100.f;
	// Flattened so dots read as discs from above without hiding the pheromone view.
	const FVector DotScale(Scale, Scale, Scale * 0.25f);
	const FVector Lift(0.f, 0.f, AAntRenderActor::HeightAboveGround);

	Transforms.Reset();
	CustomData.Reset();

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FAntFragment> Ants = Context.GetFragmentView<FAntFragment>();
		const TConstArrayView<FTransformFragment> AntTransforms = Context.GetFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FTransform& AntTransform = AntTransforms[EntityIt].GetTransform();
			Transforms.Emplace(AntTransform.GetRotation(), AntTransform.GetLocation() + Lift, DotScale);

			const FLinearColor& Color = Ants[EntityIt].State == EAntState::Returning ? Settings->CarryingColor : Settings->SearchingColor;
			CustomData.Append({ Color.R, Color.G, Color.B });
		}
	});

	RenderActor->UpdateInstances(Transforms, CustomData);
}
