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
	/** Gave up searching: lays nothing, follows Home (or pulls gently towards the nest) and tries again. */
	Homing,
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

	/** Multiplies UAntSettings::Speed, so ants do not all move in lockstep. */
	float SpeedScale = 1.f;

	/** Order in which this ant leaves the nest; it stays inside until UColonySubsystem has released this many. */
	int32 EmergeIndex = 0;

	EAntState State = EAntState::Searching;

	/** Multiplies the Food laid on the current trip home: UAntSettings::RecruitBoost after finding an unmarked source. */
	float DepositBoost = 1.f;

	/** Scouts ignore Food trails while searching. Assigned at spawn and kept for life. */
	bool bScout = false;

	/** Each ant has its own stream so wander does not depend on processing order. */
	FRandomStream Random;
};
