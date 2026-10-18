#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RollingEventCounter.h"
#include "ColonySubsystem.generated.h"

class AAntRenderActor;

/**
 * The player's colony for a game or PIE world. On begin play it records where the nest and food sources are,
 * spawns UAntSettings::AntCount ants at the nest, and from then on counts the food they bring home.
 */
UCLASS()
class SUPERCOLONY_API UColonySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	bool HasNest() const { return bHasNest; }
	FVector2D GetNestLocation() const { return NestLocation; }
	TConstArrayView<FVector2D> GetFoodLocations() const { return FoodLocations; }

	/** Called by the ant movement processor when ants drop food at the nest. */
	void AddFoodDelivered(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Colony")
	int32 GetFoodDelivered() const { return FoodDelivered; }

	/** Food delivered in the last 60 seconds. */
	UFUNCTION(BlueprintPure, Category = "Colony")
	int32 GetFoodLastMinute();

	/** Seconds since the ants were spawned, so the HUD can say when the per-minute window is not full yet. */
	float GetSecondsSinceSpawn() const;

	UFUNCTION(BlueprintPure, Category = "Colony")
	int32 GetAntCount() const { return AntCount; }

	AAntRenderActor* GetRenderActor() const { return RenderActor; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void FindTargets(UWorld& InWorld);
	void SpawnAnts(UWorld& InWorld);

	bool bHasNest = false;
	FVector2D NestLocation = FVector2D::ZeroVector;
	TArray<FVector2D> FoodLocations;

	int32 AntCount = 0;
	int32 FoodDelivered = 0;
	FRollingEventCounter FoodCounter{ 60.0 };
	double SpawnTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<AAntRenderActor> RenderActor;
};
