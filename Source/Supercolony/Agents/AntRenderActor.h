#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AntRenderActor.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Holds the single instanced mesh that draws every ant as a dot. Spawned by UColonySubsystem;
 * UAntRenderProcessor rewrites all instance transforms and colours each frame.
 */
UCLASS(NotPlaceable, Transient)
class SUPERCOLONY_API AAntRenderActor : public AActor
{
	GENERATED_BODY()

public:
	AAntRenderActor();

	/** Per-instance custom data floats: RGB colour. */
	static constexpr int32 NumCustomDataFloats = 3;

	/** Height of the dot centres above Z = 0, just above the pheromone debug plane. */
	static constexpr float HeightAboveGround = 5.f;

	virtual void BeginPlay() override;

	/** Replaces every instance with these transforms and colours (NumCustomDataFloats per instance). */
	void UpdateInstances(TConstArrayView<FTransform> Transforms, TConstArrayView<float> CustomData);

private:
	UPROPERTY(VisibleAnywhere, Category = "Ants")
	TObjectPtr<UInstancedStaticMeshComponent> Dots;
};
