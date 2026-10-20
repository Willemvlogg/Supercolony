#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AntSettings.generated.h"

class UMassEntityConfigAsset;
class UMaterialInterface;
class UStaticMesh;

/** Tuning for ant agents. Shown under Project Settings > Game > Ants. Distances are in centimetres. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Ants"))
class SUPERCOLONY_API UAntSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAntSettings();

	/** Ants spawned at the nest when the level starts. */
	UPROPERTY(Config, EditAnywhere, Category = "Spawning", meta = (ClampMin = "0", ClampMax = "100000"))
	int32 AntCount = 500;

	/** Entity config used to spawn ants. Only needs UAntTrait. */
	UPROPERTY(Config, EditAnywhere, Category = "Spawning")
	TSoftObjectPtr<UMassEntityConfigAsset> AntEntityConfig;

	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float Speed = 400.f;

	/** Fastest an ant turns when following sensors or heading for something in sight. Wander is added on top. */
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0", Units = "DegreesPerSecond"))
	float TurnRate = 240.f;

	/** Random turning as a fraction of TurnRate. 0 walks straight lines, 1 is as strong as a sensor turn. */
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0"))
	float WanderStrength = 0.3f;

	/** Angle of the left and right sensors either side of the heading. */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "Degrees"))
	float SensorAngle = 35.f;

	/** How far in front of the ant the three sensors sample the grid. */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float SensorDistance = 200.f;

	/** Within this distance of its goal (food or nest), an ant ignores pheromones and heads straight for it. */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float SightRadius = 300.f;

	/** Pheromone laid per second right after leaving the nest or food. Fades with DepositFalloffTime. */
	UPROPERTY(Config, EditAnywhere, Category = "Pheromones", meta = (ClampMin = "0.0"))
	float DepositAmount = 1.f;

	/** Seconds for the deposit rate to fall to about 37% (1/e) of DepositAmount. */
	UPROPERTY(Config, EditAnywhere, Category = "Pheromones", meta = (ClampMin = "0.1", Units = "Seconds"))
	float DepositFalloffTime = 20.f;

	/** An ant this close to the nest centre has arrived. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float NestRadius = 150.f;

	/** An ant this close to a food source centre has arrived. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float FoodRadius = 150.f;

	/** Ants stay at least this far inside the pheromone grid edge. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float BoundsMargin = 50.f;

	/** Mesh drawn for each ant, scaled to DotSize. Assumed to be 100 cm across, like the engine basic shapes. */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	TSoftObjectPtr<UStaticMesh> DotMesh;

	/** Unlit material that reads its colour from per-instance custom data 0-2 (RGB). */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	TSoftObjectPtr<UMaterialInterface> DotMaterial;

	UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "1.0", Units = "Centimeters"))
	float DotSize = 40.f;

	/** Dark so it reads against the light default floor and the pheromone overlay. */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	FLinearColor SearchingColor = FLinearColor(0.02f, 0.02f, 0.02f);

	UPROPERTY(Config, EditAnywhere, Category = "Display")
	FLinearColor CarryingColor = FLinearColor(1.f, 0.35f, 0.f);
};
