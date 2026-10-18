#include "ColonyMarkerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The engine's BasicShapeMaterial exposes its colour under this parameter name. */
	const FName ColorParameter(TEXT("Color"));
}

AColonyMarkerActor::AColonyMarkerActor()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Mesh;
}

void AColonyMarkerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(ColorParameter, Color);
	}
}
