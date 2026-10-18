#include "FoodSourceActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AFoodSourceActor::AFoodSourceActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Mesh->SetStaticMesh(SphereMesh.Object);
	Mesh->SetRelativeScale3D(FVector(2.f));

	Color = FLinearColor(0.1f, 0.9f, 0.2f);
}
