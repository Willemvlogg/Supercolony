#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ColonyMarkerActor.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

/** A coloured engine basic shape used as a placeholder for colony landmarks. No collision. */
UCLASS(Abstract)
class SUPERCOLONY_API AColonyMarkerActor : public AActor
{
	GENERATED_BODY()

public:
	AColonyMarkerActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Colony")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Colony")
	FLinearColor Color = FLinearColor::White;

private:
	/** Parent of the coloured material instance. Must have a vector parameter named "Color". */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;
};
