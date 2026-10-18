#include "PheromoneDebugActor.h"
#include "PheromoneGridSubsystem.h"
#include "PheromoneSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName GridTextureParameter(TEXT("Grid"));

	/** The engine plane mesh is 100 x 100 cm. */
	constexpr float PlaneMeshSize = 100.f;

	/** Maps a cell value to 0-255. The square root keeps faint trails visible next to strong ones. */
	uint8 ToBrightness(float Value, float MaxValue)
	{
		return static_cast<uint8>(FMath::Sqrt(FMath::Clamp(Value / MaxValue, 0.f, 1.f)) * 255.f);
	}
}

APheromoneDebugActor::APheromoneDebugActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetCastShadow(false);
	RootComponent = Plane;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	Plane->SetStaticMesh(PlaneMesh.Object);
}

void APheromoneDebugActor::BeginPlay()
{
	Super::BeginPlay();

	const UPheromoneGridSubsystem* Subsystem = GetWorld()->GetSubsystem<UPheromoneGridSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	const FPheromoneGrid& Grid = Subsystem->GetGrid();
	const FVector2D GridCentre = Grid.GetOrigin() + 0.5f * Grid.GetWorldSize();
	SetActorLocation(FVector(GridCentre, HeightAboveGround));
	SetActorScale3D(FVector(Grid.GetWorldSize() / PlaneMeshSize, 1.f));

	GridTexture = UTexture2D::CreateTransient(Grid.GetWidth(), Grid.GetHeight(), PF_B8G8R8A8);
	GridTexture->Filter = TF_Nearest;
	GridTexture->SRGB = false;
	GridTexture->UpdateResource();

	UMaterialInterface* BaseMaterial = GetDefault<UPheromoneSettings>()->DebugMaterial.LoadSynchronous();
	if (BaseMaterial)
	{
		PlaneMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		PlaneMaterial->SetTextureParameterValue(GridTextureParameter, GridTexture);
		Plane->SetMaterial(0, PlaneMaterial);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Pheromones.Debug: DebugMaterial is not set or failed to load. See Project Settings > Game > Pheromones."));
	}
}

void APheromoneDebugActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UPheromoneGridSubsystem* Subsystem = GetWorld()->GetSubsystem<UPheromoneGridSubsystem>();
	if (GridTexture && Subsystem && Subsystem->GetStepCount() != LastDrawnStep)
	{
		LastDrawnStep = Subsystem->GetStepCount();
		RedrawTexture();
	}
}

void APheromoneDebugActor::RedrawTexture()
{
	const FPheromoneGrid& Grid = GetWorld()->GetSubsystem<UPheromoneGridSubsystem>()->GetGrid();
	const int32 Width = Grid.GetWidth();
	const int32 Height = Grid.GetHeight();
	const float MaxValue = Grid.GetMaxCellValue();

	// The render thread copies this buffer later, so it is heap-allocated and freed in the cleanup callback.
	FColor* Pixels = static_cast<FColor*>(FMemory::Malloc(Width * Height * sizeof(FColor)));
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint Cell(X, Y);
			const uint8 Food = ToBrightness(Grid.GetCellValue(Cell, EPheromoneChannel::Food), MaxValue);
			const uint8 Home = ToBrightness(Grid.GetCellValue(Cell, EPheromoneChannel::Home), MaxValue);
			Pixels[Y * Width + X] = FColor(0, Food, Home, 255);
		}
	}

	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Width, Height);
	GridTexture->UpdateTextureRegions(0, 1, Region, Width * sizeof(FColor), sizeof(FColor), reinterpret_cast<uint8*>(Pixels),
		[](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
		{
			FMemory::Free(SrcData);
			delete Regions;
		});
}

namespace
{
	APheromoneDebugActor* FindDebugActor(UWorld* World)
	{
		TActorIterator<APheromoneDebugActor> It(World);
		return It ? *It : nullptr;
	}

	/** Pheromones.Debug [0|1]: shows or hides the debug plane. With no argument it toggles. */
	void HandleDebugCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->GetSubsystem<UPheromoneGridSubsystem>())
		{
			UE_LOG(LogTemp, Warning, TEXT("Pheromones.Debug only works in a game or PIE world."));
			return;
		}

		APheromoneDebugActor* Existing = FindDebugActor(World);
		const bool bShow = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : Existing == nullptr;

		if (bShow && !Existing)
		{
			World->SpawnActor<APheromoneDebugActor>();
		}
		else if (!bShow && Existing)
		{
			Existing->Destroy();
		}
	}

	/** Pheromones.Deposit <Channel> <X> <Y> <Amount>: adds pheromone at a world location, for testing the view. */
	void HandleDepositCommand(const TArray<FString>& Args, UWorld* World)
	{
		UPheromoneGridSubsystem* Subsystem = World ? World->GetSubsystem<UPheromoneGridSubsystem>() : nullptr;
		const UEnum* ChannelEnum = StaticEnum<EPheromoneChannel>();
		const int64 ChannelValue = Args.Num() == 4 ? ChannelEnum->GetValueByNameString(Args[0]) : INDEX_NONE;
		if (!Subsystem || ChannelValue == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Usage in a game or PIE world: Pheromones.Deposit <Food|Home|Alarm|Rally|Avoid> <X> <Y> <Amount>"));
			return;
		}

		const FVector Location(FCString::Atof(*Args[1]), FCString::Atof(*Args[2]), 0.f);
		Subsystem->Deposit(Location, static_cast<EPheromoneChannel>(ChannelValue), FCString::Atof(*Args[3]));
	}

	FAutoConsoleCommandWithWorldAndArgs DebugCommand(
		TEXT("Pheromones.Debug"),
		TEXT("Show the Food (green) and Home (blue) channels on a plane. Usage: Pheromones.Debug [0|1]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleDebugCommand));

	FAutoConsoleCommandWithWorldAndArgs DepositCommand(
		TEXT("Pheromones.Deposit"),
		TEXT("Add pheromone at a world location. Usage: Pheromones.Deposit <Channel> <X> <Y> <Amount>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleDepositCommand));
}
