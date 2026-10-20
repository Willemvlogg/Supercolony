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

	for (TActorIterator<AFoodSourceActor> It(&InWorld); It; ++It)
	{
		FoodLocations.Add(FVector2D(It->GetActorLocation()));
	}
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
		Ant.State = EAntState::Searching;
		Ant.TimeSinceTarget = 0.f;

		EntityManager.GetFragmentDataChecked<FTransformFragment>(Entities[Index]).SetTransform(FTransform(SpawnLocation));
	}

	AntCount = Entities.Num();
	Emergence.Reset(AntCount);
	SpawnTime = InWorld.GetTimeSeconds();
	UE_LOG(LogTemp, Log, TEXT("Colony: spawned %d ants at the nest, %d food sources."), AntCount, FoodLocations.Num());
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
