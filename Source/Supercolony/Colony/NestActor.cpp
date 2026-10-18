#include "NestActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ANestActor::ANestActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Mesh->SetStaticMesh(CylinderMesh.Object);
	Mesh->SetRelativeScale3D(FVector(3.f, 3.f, 0.5f));

	Color = FLinearColor(0.1f, 0.3f, 1.f);
}
