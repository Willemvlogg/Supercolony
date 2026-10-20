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

	/** Searching lays Home, Returning lays Food, Homing lays nothing. Returns false when the ant lays nothing. */
	bool GetDepositedChannel(EAntState State, EPheromoneChannel& OutChannel)
	{
		switch (State)
		{
		case EAntState::Searching: OutChannel = EPheromoneChannel::Home; return true;
		case EAntState::Returning: OutChannel = EPheromoneChannel::Food; return true;
		default:                   return false;
		}
	}

	/** Where ants may walk: the pheromone grid minus BoundsMargin on every side. */
	void GetAntBounds(const FPheromoneGrid& Grid, const UAntSettings& Settings, FVector2D& OutMin, FVector2D& OutMax)
	{
		OutMin = Grid.GetOrigin() + FVector2D(Settings.BoundsMargin);
		OutMax = Grid.GetOrigin() + Grid.GetWorldSize() - FVector2D(Settings.BoundsMargin);
	}

	/** The nearest food source (Searching) or the nest (Returning, Homing), if within SightRadius. */
	bool FindGoalInSight(EAntState State, const FVector2D& Position, const UColonySubsystem& Colony, float SightRadius, FVector2D& OutGoal)
	{
		float BestDistSquared = FMath::Square(SightRadius);
		bool bFound = false;

		if (State != EAntState::Searching)
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

	/** Pick up food or come home: switch state, turn around, restart the deposit falloff. */
	void ArriveAtTarget(FAntFragment& Ant, EAntState NewState)
	{
		Ant.State = NewState;
		Ant.Yaw = AntSteering::WrapYaw(Ant.Yaw + UE_PI);
		Ant.TimeSinceTarget = 0.f;
		Ant.DepositBoost = 1.f;
	}

	/** Whether any of the ant's three sensors picks up a Food trail at Position. */
	bool SensesFoodTrail(const FPheromoneGrid& Grid, const FVector2D& Position, float Yaw, const UAntSettings& Settings)
	{
		FVector2D Left, Ahead, Right;
		AntSteering::GetSensorPositions(Position, Yaw, FMath::DegreesToRadians(Settings.SensorAngle), Settings.SensorDistance, Left, Ahead, Right);
		return AntSteering::ReadSensors(Grid.Sample(Left, EPheromoneChannel::Food), Grid.Sample(Ahead, EPheromoneChannel::Food),
			Grid.Sample(Right, EPheromoneChannel::Food), Settings.MinSensorValue).bTrailSensed;
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
	const FVector2D NestLocation = Colony->GetNestLocation();
	const FEmergenceGate& Emergence = Colony->GetEmergence();

	FVector2D BoundsMin, BoundsMax;
	GetAntBounds(Grid, *Settings, BoundsMin, BoundsMax);

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FAntFragment> Ants = Context.GetMutableFragmentView<FAntFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FAntFragment& Ant = Ants[EntityIt];
			if (!Emergence.IsReleased(Ant.EmergeIndex))
			{
				continue;
			}
			const FVector2D Position(Transforms[EntityIt].GetTransform().GetLocation());

			FVector2D Goal;
			if (FindGoalInSight(Ant.State, Position, *Colony, Settings->SightRadius, Goal))
			{
				const FVector2D ToGoal = Goal - Position;
				Ant.Yaw = AntSteering::TurnToward(Ant.Yaw, FMath::Atan2(ToGoal.Y, ToGoal.X), MaxTurn);
			}
			else if (Ant.bScout && Ant.State == EAntState::Searching)
			{
				// Scouts explore: no trail following, always the off-trail wander.
				Ant.Yaw = AntSteering::ApplyWander(Ant.Yaw, Settings->WanderOffTrail, TurnRate, DeltaTime, Ant.Random);
			}
			else
			{
				const EPheromoneChannel Channel = GetFollowedChannel(Ant.State);
				FVector2D Left, Ahead, Right;
				AntSteering::GetSensorPositions(Position, Ant.Yaw, SensorAngle, Settings->SensorDistance, Left, Ahead, Right);
				const AntSteering::FSensorReading Reading = AntSteering::ReadSensors(
					Grid.Sample(Left, Channel), Grid.Sample(Ahead, Channel), Grid.Sample(Right, Channel), Settings->MinSensorValue);

				Ant.Yaw = AntSteering::WrapYaw(Ant.Yaw + Reading.Turn * MaxTurn);
				if (Ant.State == EAntState::Homing && !Reading.bTrailSensed)
				{
					Ant.Yaw = AntSteering::ApplyHomingBias(Ant.Yaw, Position, NestLocation, Settings->HomingBias, TurnRate, DeltaTime);
				}

				const float Wander = AntSteering::GetWanderStrength(Reading.bTrailSensed, Settings->WanderOnTrail, Settings->WanderOffTrail);
				Ant.Yaw = AntSteering::ApplyWander(Ant.Yaw, Wander, TurnRate, DeltaTime, Ant.Random);
			}

			FVector2D Away;
			const float EdgeStrength = AntSteering::GetEdgeAvoidance(Position, BoundsMin, BoundsMax, Settings->EdgeAvoidDistance, Away);
			if (EdgeStrength > 0.f)
			{
				Ant.Yaw = AntSteering::TurnToward(Ant.Yaw, FMath::Atan2(Away.Y, Away.X), EdgeStrength * MaxTurn);
			}
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
	FVector2D BoundsMin, BoundsMax;
	GetAntBounds(Grid, *Settings, BoundsMin, BoundsMax);

	// This processor runs once per frame on the game thread, so it is the one that lets more ants out.
	Colony->AdvanceEmergence(DeltaTime);
	const FEmergenceGate& Emergence = Colony->GetEmergence();

	int32 Delivered = 0;
	TArray<int32, TInlineAllocator<8>> PickupsPerFood;
	PickupsPerFood.SetNumZeroed(FoodLocations.Num());

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FAntFragment> Ants = Context.GetMutableFragmentView<FAntFragment>();
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FAntFragment& Ant = Ants[EntityIt];
			if (!Emergence.IsReleased(Ant.EmergeIndex))
			{
				continue;
			}
			FTransform& Transform = Transforms[EntityIt].GetMutableTransform();

			FVector2D Position = FVector2D(Transform.GetLocation()) + AntSteering::HeadingToDirection(Ant.Yaw) * Step * Ant.SpeedScale;
			AntSteering::ClampToBounds(Position, BoundsMin, BoundsMax);
			Ant.TimeSinceTarget += DeltaTime;

			if (AntSteering::ShouldGiveUpSearch(Ant.State == EAntState::Searching, Ant.TimeSinceTarget, Settings->MaxSearchTime))
			{
				// No turn-around: the Home trail or the homing bias brings it round gradually.
				Ant.State = EAntState::Homing;
			}

			// Searching and homing ants both pick up food they walk into.
			if (Ant.State != EAntState::Returning)
			{
				for (int32 FoodIndex = 0; FoodIndex < FoodLocations.Num(); ++FoodIndex)
				{
					if (FVector2D::DistSquared(Position, FoodLocations[FoodIndex]) <= FoodRadiusSquared)
					{
						// Read the sensors before turning around: no Food trail here means a new find worth recruiting to.
						const bool bFoodTrailSensed = SensesFoodTrail(Grid, Position, Ant.Yaw, *Settings);
						ArriveAtTarget(Ant, EAntState::Returning);
						Ant.DepositBoost = AntSteering::GetRecruitBoost(bFoodTrailSensed, Settings->RecruitBoost);
						++PickupsPerFood[FoodIndex];
						break;
					}
				}
			}

			// Returning ants drop food at the nest; homing ants just come in and set out again.
			if (Ant.State != EAntState::Searching && FVector2D::DistSquared(Position, NestLocation) <= NestRadiusSquared)
			{
				if (Ant.State == EAntState::Returning)
				{
					++Delivered;
				}
				ArriveAtTarget(Ant, EAntState::Searching);
			}

			Transform.SetLocation(FVector(Position, 0.f));
			Transform.SetRotation(FQuat(FVector::UpVector, Ant.Yaw));
		}
	});

	if (Delivered > 0)
	{
		Colony->AddFoodDelivered(Delivered);
	}
	for (int32 FoodIndex = 0; FoodIndex < PickupsPerFood.Num(); ++FoodIndex)
	{
		Colony->AddFoodPickups(FoodIndex, PickupsPerFood[FoodIndex]);
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
	const UColonySubsystem* Colony = World ? World->GetSubsystem<UColonySubsystem>() : nullptr;
	if (!GridSubsystem || !Colony)
	{
		return;
	}

	const UAntSettings* Settings = GetDefault<UAntSettings>();
	const float DeltaTime = GetDeltaTime(Context);
	const FEmergenceGate& Emergence = Colony->GetEmergence();

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FAntFragment> Ants = Context.GetFragmentView<FAntFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FAntFragment& Ant = Ants[EntityIt];
			EPheromoneChannel Channel;
			if (!Emergence.IsReleased(Ant.EmergeIndex) || !GetDepositedChannel(Ant.State, Channel))
			{
				continue;
			}
			const float Rate = AntSteering::GetDepositRate(Settings->DepositAmount, Ant.TimeSinceTarget, Settings->DepositFalloffTime,
				Settings->MaxTrailTime);
			if (Rate <= 0.f)
			{
				continue;
			}
			GridSubsystem->Deposit(Transforms[EntityIt].GetTransform().GetLocation(), Channel, Rate * Ant.DepositBoost * DeltaTime);
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
			// Ants still inside the nest sit at its centre, hidden by the nest marker.
			const FTransform& AntTransform = AntTransforms[EntityIt].GetTransform();
			Transforms.Emplace(AntTransform.GetRotation(), AntTransform.GetLocation() + Lift, DotScale);

			const FAntFragment& Ant = Ants[EntityIt];
			const FLinearColor& Color = Ant.State == EAntState::Returning ? Settings->CarryingColor
				: Ant.State == EAntState::Homing ? Settings->HomingColor
				: Ant.bScout ? Settings->ScoutColor
				: Settings->SearchingColor;
			CustomData.Append({ Color.R, Color.G, Color.B });
		}
	});

	RenderActor->UpdateInstances(Transforms, CustomData);
}
