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

	/** Ants leaving the nest per second after the level starts. 0 lets them all out at once. */
	UPROPERTY(Config, EditAnywhere, Category = "Spawning", meta = (ClampMin = "0.0"))
	float EmergeRate = 50.f;

	/** Entity config used to spawn ants. Only needs UAntTrait. */
	UPROPERTY(Config, EditAnywhere, Category = "Spawning")
	TSoftObjectPtr<UMassEntityConfigAsset> AntEntityConfig;

	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float Speed = 400.f;

	/** Each ant gets its own speed within Speed * (1 +- SpeedVariation). 0.2 means +-20%. */
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float SpeedVariation = 0.2f;

	/** Fastest an ant turns when following sensors or heading for something in sight. Wander is added on top. */
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0", Units = "DegreesPerSecond"))
	float TurnRate = 240.f;

	/**
	 * Share of ants that are scouts. While searching, scouts ignore Food trails and always wander with WanderOffTrail,
	 * so the colony keeps exploring after strong trails form. A scout becomes a normal ant once it reaches food.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Behaviour", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoutFraction = 0.1f;

	/** Random turning, as a fraction of TurnRate, while a sensor picks up the trail being followed. */
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0"))
	float WanderOnTrail = 0.15f;

	/** Random turning, as a fraction of TurnRate, while no trail is sensed and the ant is exploring. */
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0"))
	float WanderOffTrail = 0.5f;

	/** Angle of the left and right sensors either side of the heading. */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "Degrees"))
	float SensorAngle = 35.f;

	/** How far in front of the ant the three sensors sample the grid. */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float SensorDistance = 200.f;

	/**
	 * Sensor readings below this count as zero, so an ant on a faint, stale trail wanders instead of following it.
	 * Stops old trails from trapping ants in loops. For scale: one fresh ant crossing a 50 cm cell lays about 0.125.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0"))
	float MinSensorValue = 0.02f;

	/** Within this distance of its goal (food or nest), an ant ignores pheromones and heads straight for it. */
	UPROPERTY(Config, EditAnywhere, Category = "Sensing", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float SightRadius = 300.f;

	/** Pheromone laid per second right after leaving the nest or food. Fades with DepositFalloffTime. */
	UPROPERTY(Config, EditAnywhere, Category = "Pheromones", meta = (ClampMin = "0.0"))
	float DepositAmount = 1.f;

	/** Seconds for the deposit rate to fall to about 37% (1/e) of DepositAmount. */
	UPROPERTY(Config, EditAnywhere, Category = "Pheromones", meta = (ClampMin = "0.1", Units = "Seconds"))
	float DepositFalloffTime = 20.f;

	/** An ant that has not reached the nest or food for this long is lost and stops laying pheromone altogether. */
	UPROPERTY(Config, EditAnywhere, Category = "Pheromones", meta = (ClampMin = "0.0", Units = "Seconds"))
	float MaxTrailTime = 60.f;

	/** A searching ant that has not found food for this long gives up and heads home (Homing), then tries again. */
	UPROPERTY(Config, EditAnywhere, Category = "Behaviour", meta = (ClampMin = "0.0", Units = "Seconds"))
	float MaxSearchTime = 45.f;

	/**
	 * When a homing ant senses no Home trail, it turns towards the nest at this fraction of TurnRate.
	 * Off-trail wander is still added, so the path home is a gentle curve rather than a straight line.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Behaviour", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HomingBias = 0.3f;

	/** An ant this close to the nest centre has arrived. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float NestRadius = 150.f;

	/** An ant this close to a food source centre has arrived. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float FoodRadius = 150.f;

	/** Ants stay at least this far inside the pheromone grid edge. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float BoundsMargin = 50.f;

	/** Within this distance of the bounds, ants start turning away; the closer they get, the harder they turn. */
	UPROPERTY(Config, EditAnywhere, Category = "Targets", meta = (ClampMin = "0.0", Units = "Centimeters"))
	float EdgeAvoidDistance = 300.f;

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

	/** Searching scouts: close to SearchingColor so the swarm still reads as one, but tinted so scouts can be spotted. */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	FLinearColor ScoutColor = FLinearColor(0.35f, 0.05f, 0.4f);

	/** Ants that gave up searching and are heading home empty-handed. */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	FLinearColor HomingColor = FLinearColor(0.2f, 0.35f, 0.7f);
};
