#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PheromoneDebugActor.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;

/**
 * A flat plane over the pheromone grid showing Food (green) and Home (blue).
 * Spawned and removed with the console command Pheromones.Debug. Redraws only after a grid step.
 */
UCLASS(NotPlaceable, Transient)
class SUPERCOLONY_API APheromoneDebugActor : public AActor
{
	GENERATED_BODY()

public:
	APheromoneDebugActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Height above Z = 0 so the plane sits just on top of the floor. */
	static constexpr float HeightAboveGround = 2.f;

private:
	void RedrawTexture();

	UPROPERTY(VisibleAnywhere, Category = "Pheromones")
	TObjectPtr<UStaticMeshComponent> Plane;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> GridTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlaneMaterial;

	uint32 LastDrawnStep = MAX_uint32;
};
