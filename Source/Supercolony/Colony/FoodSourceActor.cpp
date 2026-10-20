#include "FoodSourceActor.h"
#include "ColonySubsystem.h"
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

void AFoodSourceActor::BeginPlay()
{
	Super::BeginPlay();

	if (UColonySubsystem* Colony = GetWorld()->GetSubsystem<UColonySubsystem>())
	{
		Colony->RegisterFoodSource(*this);
	}
}

void AFoodSourceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UColonySubsystem* Colony = GetWorld()->GetSubsystem<UColonySubsystem>())
	{
		Colony->UnregisterFoodSource(*this);
	}

	Super::EndPlay(EndPlayReason);
}
