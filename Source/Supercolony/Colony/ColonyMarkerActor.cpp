#include "ColonyMarkerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** BasicShapeMaterial exposes its colour under this parameter name. */
	const FName ColorParameter(TEXT("Color"));
}

AColonyMarkerActor::AColonyMarkerActor()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Mesh;

	// The engine basic shape meshes ship with DefaultMaterial, which has no Color parameter.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = ShapeMaterial.Object;
}

void AColonyMarkerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Always build the instance from BaseMaterial rather than the mesh's current material,
	// so a stale override saved in the level is replaced instead of reused.
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Material->SetVectorParameterValue(ColorParameter, Color);
		Mesh->SetMaterial(0, Material);
	}
}
