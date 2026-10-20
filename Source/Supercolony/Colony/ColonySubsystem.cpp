#include "ColonySubsystem.h"
#include "EngineUtils.h"
#include "FoodSourceActor.h"
#include "NestActor.h"
#include "Agents/AntFragments.h"
#include "Agents/AntRenderActor.h"
#include "Agents/AntSettings.h"
#include "Agents/AntSteering.h"
#include "MassEntityConfigAsset.h"
#include "MassEntitySubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "Mass/EntityFragments.h"

void UColonySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	FindTargets(InWorld);
	if (!bHasNest)
	{
		UE_LOG(LogTemp, Warning, TEXT("Colony: no NestActor in the level, so no ants were spawned."));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	RenderActor = InWorld.SpawnActor<AAntRenderActor>(SpawnParams);

	SpawnAnts(InWorld);
}

void UColonySubsystem::FindTargets(UWorld& InWorld)
{
	for (TActorIterator<ANestActor> It(&InWorld); It; ++It)
	{
		if (bHasNest)
		{
			UE_LOG(LogTemp, Warning, TEXT("Colony: more than one NestActor; using the first, ignoring %s."), *It->GetName());
			continue;
		}
		NestLocation = FVector2D(It->GetActorLocation());
		bHasNest = true;
	}
}

void UColonySubsystem::RegisterFoodSource(AFoodSourceActor& Source)
{
	if (FoodSources.Contains(&Source))
	{
		return;
	}

	const FVector2D Location(Source.GetActorLocation());
	FoodLocations.Add(Location);
	FoodSources.Add(&Source);

	FFoodSourceStats& Stats = FoodStats.AddDefaulted_GetRef();
	Stats.Label = Source.GetActorNameOrLabel();
	Stats.AppearedTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogTemp, Log, TEXT("Colony: food source %s at (%.1f, %.1f) m."), *Stats.Label, Location.X / 100.f, Location.Y / 100.f);
}

void UColonySubsystem::UnregisterFoodSource(AFoodSourceActor& Source)
{
	const int32 Index = FoodSources.IndexOfByKey(&Source);
	if (Index != INDEX_NONE)
	{
		FoodLocations.RemoveAt(Index);
		FoodSources.RemoveAt(Index);
		FoodStats.RemoveAt(Index);
	}
}

void UColonySubsystem::AddFoodPickups(int32 FoodIndex, int32 Amount)
{
	if (!FoodStats.IsValidIndex(FoodIndex) || Amount <= 0)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	FFoodSourceStats& Stats = FoodStats[FoodIndex];
	if (Stats.FirstPickupTime < 0.0)
	{
		Stats.FirstPickupTime = Now;
		UE_LOG(LogTemp, Log, TEXT("Colony: first pickup from %s, %.1f s after it appeared."), *Stats.Label, Now - Stats.AppearedTime);
	}
	Stats.Pickups += Amount;
	Stats.PickupCounter.Add(Now, Amount);
}

void UColonySubsystem::SpawnAnts(UWorld& InWorld)
{
	const UAntSettings* Settings = GetDefault<UAntSettings>();
	const UMassEntityConfigAsset* EntityConfig = Settings->AntEntityConfig.LoadSynchronous();
	if (!EntityConfig)
	{
		UE_LOG(LogTemp, Warning, TEXT("Colony: AntEntityConfig is not set or failed to load. See Project Settings > Game > Ants."));
		return;
	}

	const FMassEntityTemplate& Template = EntityConfig->GetOrCreateEntityTemplate(InWorld);
	UMassSpawnerSubsystem* Spawner = InWorld.GetSubsystem<UMassSpawnerSubsystem>();
	UMassEntitySubsystem* EntitySubsystem = InWorld.GetSubsystem<UMassEntitySubsystem>();
	if (!Template.IsValid() || !Spawner || !EntitySubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Colony: could not build the ant template or find the Mass subsystems."));
		return;
	}

	TArray<FMassEntityHandle> Entities;
	{
		// Creation finishes when the context is released, so fragment values are set after this scope.
		TSharedPtr<FMassEntityManager::FEntityCreationContext> CreationContext = Spawner->SpawnEntities(Template, Settings->AntCount, Entities);
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	const FVector SpawnLocation(NestLocation, 0.f);
	const int32 BaseSeed = FMath::Rand();
	for (int32 Index = 0; Index < Entities.Num(); ++Index)
	{
		FAntFragment& Ant = EntityManager.GetFragmentDataChecked<FAntFragment>(Entities[Index]);
		Ant.Random.Initialize(BaseSeed + Index);
		Ant.Yaw = Ant.Random.FRandRange(-UE_PI, UE_PI);
		Ant.SpeedScale = AntSteering::RandomSpeedScale(Settings->SpeedVariation, Ant.Random);
		Ant.EmergeIndex = Index;
		Ant.bScout = AntSteering::RollScout(Settings->ScoutFraction, Ant.Random);
		Ant.State = EAntState::Searching;
		Ant.TimeSinceTarget = 0.f;

		EntityManager.GetFragmentDataChecked<FTransformFragment>(Entities[Index]).SetTransform(FTransform(SpawnLocation));
	}

	AntCount = Entities.Num();
	Emergence.Reset(AntCount);
	SpawnTime = InWorld.GetTimeSeconds();
	UE_LOG(LogTemp, Log, TEXT("Colony: spawned %d ants at the nest."), AntCount);
}

void UColonySubsystem::AdvanceEmergence(float DeltaTime)
{
	Emergence.Advance(GetDefault<UAntSettings>()->EmergeRate, DeltaTime);
}

void UColonySubsystem::AddFoodDelivered(int32 Amount)
{
	FoodDelivered += Amount;
	FoodCounter.Add(GetWorld()->GetTimeSeconds(), Amount);
}

int32 UColonySubsystem::GetFoodLastMinute()
{
	return FoodCounter.GetCount(GetWorld()->GetTimeSeconds());
}

float UColonySubsystem::GetSecondsSinceSpawn() const
{
	return static_cast<float>(GetWorld()->GetTimeSeconds() - SpawnTime);
}

bool UColonySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
