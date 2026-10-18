#include "PheromoneGridSubsystem.h"
#include "PheromoneSettings.h"

namespace
{
	/** After a long hitch, run at most this many steps in one frame and drop the rest instead of stalling further. */
	constexpr int32 MaxStepsPerFrame = 4;
}

void UPheromoneGridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UPheromoneSettings* Settings = GetDefault<UPheromoneSettings>();
	const FVector2D WorldSize(Settings->GridSize * Settings->CellSize);
	Grid.Init(Settings->GridSize, Settings->GridSize, Settings->CellSize, -0.5f * WorldSize, Settings->MaxCellValue);
}

void UPheromoneGridSubsystem::Tick(float DeltaTime)
{
	// Settings are read every frame so rates can be tuned in Project Settings while PIE is running.
	const float StepSeconds = 1.f / GetDefault<UPheromoneSettings>()->StepRate;

	TimeSinceLastStep += DeltaTime;
	int32 StepsThisFrame = 0;
	while (TimeSinceLastStep >= StepSeconds && StepsThisFrame < MaxStepsPerFrame)
	{
		Step(StepSeconds);
		TimeSinceLastStep -= StepSeconds;
		++StepsThisFrame;
	}
	TimeSinceLastStep = FMath::Min(TimeSinceLastStep, StepSeconds);
}

void UPheromoneGridSubsystem::Step(float StepSeconds)
{
	const UPheromoneSettings* Settings = GetDefault<UPheromoneSettings>();
	for (const EPheromoneChannel Channel : TEnumRange<EPheromoneChannel>())
	{
		Grid.Evaporate(Channel, Settings->GetEvaporationRate(Channel), StepSeconds);
		Grid.Diffuse(Channel, Settings->DiffusionRate, StepSeconds);
	}
	++StepCount;
}

TStatId UPheromoneGridSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPheromoneGridSubsystem, STATGROUP_Tickables);
}

bool UPheromoneGridSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UPheromoneGridSubsystem::Deposit(FVector WorldLocation, EPheromoneChannel Channel, float Amount)
{
	Grid.Deposit(FVector2D(WorldLocation), Channel, Amount);
}

float UPheromoneGridSubsystem::Sample(FVector WorldLocation, EPheromoneChannel Channel) const
{
	return Grid.Sample(FVector2D(WorldLocation), Channel);
}

FVector UPheromoneGridSubsystem::GetGradient(FVector WorldLocation, EPheromoneChannel Channel) const
{
	return FVector(Grid.GetGradient(FVector2D(WorldLocation), Channel), 0.f);
}
