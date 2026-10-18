#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PheromoneGrid.h"
#include "PheromoneSettings.generated.h"

class UMaterialInterface;

/** Tuning for the pheromone grid. Shown under Project Settings > Game > Pheromones. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Pheromones"))
class SUPERCOLONY_API UPheromoneSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPheromoneSettings();

	float GetEvaporationRate(EPheromoneChannel Channel) const;

	/** Number of cells along each side. The grid is square and centred on the world origin. Applies on the next level load. */
	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "8", ClampMax = "2048"))
	int32 GridSize = 256;

	/** Width of one cell. Applies on the next level load. */
	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "1.0", Units = "Centimeters"))
	float CellSize = 50.f;

	/** Deposits are clamped to this, so heavy traffic cannot build runaway values. */
	UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "0.01"))
	float MaxCellValue = 10.f;

	/** How many times per second the grid evaporates and diffuses. */
	UPROPERTY(Config, EditAnywhere, Category = "Simulation", meta = (ClampMin = "1.0", ClampMax = "60.0", Units = "Hertz"))
	float StepRate = 10.f;

	/** Fraction each cell moves towards its neighbours' average, per second. Shared by all channels. */
	UPROPERTY(Config, EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.0"))
	float DiffusionRate = 0.1f;

	/** Evaporation per second: each second a cell keeps exp(-rate) of its value. */
	UPROPERTY(Config, EditAnywhere, Category = "Evaporation", meta = (ClampMin = "0.0"))
	float FoodEvaporationRate = 0.1f;

	UPROPERTY(Config, EditAnywhere, Category = "Evaporation", meta = (ClampMin = "0.0"))
	float HomeEvaporationRate = 0.03f;

	UPROPERTY(Config, EditAnywhere, Category = "Evaporation", meta = (ClampMin = "0.0"))
	float AlarmEvaporationRate = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Evaporation", meta = (ClampMin = "0.0"))
	float RallyEvaporationRate = 0.01f;

	UPROPERTY(Config, EditAnywhere, Category = "Evaporation", meta = (ClampMin = "0.0"))
	float AvoidEvaporationRate = 0.01f;

	/** Unlit material with a texture parameter named "Grid", used by Pheromones.Debug. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UMaterialInterface> DebugMaterial;
};
