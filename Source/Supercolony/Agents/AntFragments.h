#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Math/RandomStream.h"
#include "AntFragments.generated.h"

UENUM()
enum class EAntState : uint8
{
	/** No food: lays Home, follows Food. */
	Searching,
	/** Carrying food: lays Food, follows Home back to the nest. */
	Returning,
};

/** Per-ant state. Position lives in FTransformFragment; this holds everything the ant behaviour needs on top. */
USTRUCT()
struct SUPERCOLONY_API FAntFragment : public FMassFragment
{
	GENERATED_BODY()

	/** Heading as a yaw angle in radians, see AntSteering. */
	float Yaw = 0.f;

	/** Seconds since the ant last left the nest or a food source. Drives the deposit falloff. */
	float TimeSinceTarget = 0.f;

	EAntState State = EAntState::Searching;

	/** Each ant has its own stream so wander does not depend on processing order. */
	FRandomStream Random;
};
