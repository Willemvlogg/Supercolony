#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EmergenceGate.h"
#include "RollingEventCounter.h"
#include "ColonySubsystem.generated.h"

class AAntRenderActor;
class AFoodSourceActor;

/** Per food source counters, shown on the HUD so you can see when a trail to a source has formed. */
struct FFoodSourceStats
{
	FString Label;
	double AppearedTime = 0.0;
	/** Negative until the first ant picks food up here. */
	double FirstPickupTime = -1.0;
	int32 Pickups = 0;
	FRollingEventCounter PickupCounter{ 60.0 };
	/** Negative until pickups in the last minute first reach UColonySubsystem::TrailPickupsPerMinute. */
	double TrailTime = -1.0;
};

/**
 * The player's colony for a game or PIE world. On begin play it finds the nest and spawns UAntSettings::AntCount
 * ants there. Food sources register themselves (also when spawned later), and it counts the food brought home.
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

	void RegisterFoodSource(AFoodSourceActor& Source);
	void UnregisterFoodSource(AFoodSourceActor& Source);

	/** A food source counts as having a trail once this many pickups happen there within one minute. */
	static constexpr int32 TrailPickupsPerMinute = 20;

	/** Called by the ant movement processor when ants pick up food. FoodIndex matches GetFoodLocations(). */
	void AddFoodPickups(int32 FoodIndex, int32 Amount);

	/** Same order as GetFoodLocations(). Mutable because the per-minute counters drop old entries when read. */
	TArray<FFoodSourceStats>& GetFoodSourceStats() { return FoodStats; }

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

	/** Ants that have left the nest so far. */
	UFUNCTION(BlueprintPure, Category = "Colony")
	int32 GetAntsOut() const { return Emergence.GetReleased(); }

	/** Lets more ants out at UAntSettings::EmergeRate. Called once per frame by the ant movement processor. */
	void AdvanceEmergence(float DeltaTime);

	const FEmergenceGate& GetEmergence() const { return Emergence; }

	AAntRenderActor* GetRenderActor() const { return RenderActor; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void FindTargets(UWorld& InWorld);
	void SpawnAnts(UWorld& InWorld);

	bool bHasNest = false;
	FVector2D NestLocation = FVector2D::ZeroVector;
	TArray<FVector2D> FoodLocations;
	TArray<TWeakObjectPtr<AFoodSourceActor>> FoodSources;
	TArray<FFoodSourceStats> FoodStats;

	int32 AntCount = 0;
	int32 FoodDelivered = 0;
	FRollingEventCounter FoodCounter{ 60.0 };
	FEmergenceGate Emergence;
	double SpawnTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<AAntRenderActor> RenderActor;
};
