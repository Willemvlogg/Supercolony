#include "AntRenderActor.h"
#include "AntSettings.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AAntRenderActor::AAntRenderActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Dots = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Dots"));
	Dots->SetMobility(EComponentMobility::Movable);
	Dots->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dots->SetCanEverAffectNavigation(false);
	Dots->SetCastShadow(false);
	Dots->NumCustomDataFloats = NumCustomDataFloats;
	RootComponent = Dots;
}

void AAntRenderActor::BeginPlay()
{
	Super::BeginPlay();

	const UAntSettings* Settings = GetDefault<UAntSettings>();
	UStaticMesh* Mesh = Settings->DotMesh.LoadSynchronous();
	UMaterialInterface* Material = Settings->DotMaterial.LoadSynchronous();
	if (!Mesh || !Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ants: DotMesh or DotMaterial is not set or failed to load. See Project Settings > Game > Ants."));
	}

	Dots->SetStaticMesh(Mesh);
	Dots->SetMaterial(0, Material);
}

void AAntRenderActor::UpdateInstances(TConstArrayView<FTransform> Transforms, TConstArrayView<float> CustomData)
{
	check(CustomData.Num() == Transforms.Num() * NumCustomDataFloats);

	if (Dots->GetInstanceCount() != Transforms.Num())
	{
		Dots->ClearInstances();
		Dots->AddInstances(TArray<FTransform>(Transforms), /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	}

	if (Transforms.IsEmpty())
	{
		return;
	}

	// Only the last call marks the render state dirty, so the whole update is sent to the renderer once.
	Dots->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	Dots->SetCustomData(0, Transforms.Num() - 1, CustomData, /*bMarkRenderStateDirty*/ true);
}
