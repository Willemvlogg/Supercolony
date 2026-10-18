#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PheromoneGrid.h"
#include "PheromoneGridSubsystem.generated.h"

/**
 * Owns the pheromone grid for a game or PIE world. Steps evaporation and diffusion at a fixed rate
 * (UPheromoneSettings::StepRate) rather than every frame, so the cost stays predictable.
 */
UCLASS()
class SUPERCOLONY_API UPheromoneGridSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "Pheromones")
	void Deposit(FVector WorldLocation, EPheromoneChannel Channel, float Amount);

	UFUNCTION(BlueprintPure, Category = "Pheromones")
	float Sample(FVector WorldLocation, EPheromoneChannel Channel) const;

	/** Unit direction (Z = 0) of the strongest increase, or zero if the area is flat. */
	UFUNCTION(BlueprintPure, Category = "Pheromones")
	FVector GetGradient(FVector WorldLocation, EPheromoneChannel Channel) const;

	const FPheromoneGrid& GetGrid() const { return Grid; }

	/** Increments after every evaporation and diffusion step. The debug view uses it to know when to redraw. */
	uint32 GetStepCount() const { return StepCount; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void Step(float StepSeconds);

	FPheromoneGrid Grid;
	float TimeSinceLastStep = 0.f;
	uint32 StepCount = 0;
};
