#pragma once

#include "CoreMinimal.h"

struct FRandomStream;

/**
 * Steering maths for ants, kept free of Mass and world types so it can be unit tested directly.
 * Headings are yaw angles in radians, matching Unreal yaw: 0 faces +X and positive turns towards +Y,
 * which is a right turn seen from above.
 */
namespace AntSteering
{
	/** Wander is tuned at this step length; other frame rates scale it so the spread per second stays the same. */
	constexpr float WanderReferenceStep = 1.f / 60.f;

	SUPERCOLONY_API FVector2D HeadingToDirection(float Yaw);

	/** World positions of the left, ahead and right sensors, Distance in front of Position. */
	SUPERCOLONY_API void GetSensorPositions(const FVector2D& Position, float Yaw, float SensorAngle, float Distance,
		FVector2D& OutLeft, FVector2D& OutAhead, FVector2D& OutRight);

	/**
	 * Which way the sensors say to turn: -1 left, 0 straight, +1 right.
	 * Readings below MinValue count as zero, so faint trails are ignored.
	 * Straight when ahead is at least as strong as both sides, when all are zero, or when left and right tie.
	 */
	SUPERCOLONY_API float ChooseTurn(float Left, float Ahead, float Right, float MinValue = 0.f);

	/**
	 * Adds a random turn. Strength is a fraction of TurnRate (radians per second).
	 * The step scales with sqrt(DeltaTime), like a random walk, so the spread of headings after one second
	 * does not depend on the frame rate. At 60 fps a step is up to Strength * TurnRate * DeltaTime.
	 */
	SUPERCOLONY_API float ApplyWander(float Yaw, float Strength, float TurnRate, float DeltaTime, FRandomStream& Random);

	/** Turns Yaw towards TargetYaw the short way round, by at most MaxStep radians. */
	SUPERCOLONY_API float TurnToward(float Yaw, float TargetYaw, float MaxStep);

	/**
	 * Clamps Position into [Min, Max]. On hitting a wall while moving into it, reflects the heading off that wall.
	 * Returns true if the position was clamped.
	 */
	SUPERCOLONY_API bool KeepInBounds(FVector2D& Position, float& Yaw, const FVector2D& Min, const FVector2D& Max);

	/**
	 * Deposit per second: BaseRate * exp(-TimeSinceTarget / FalloffTime). Strongest right after leaving a target.
	 * Zero once TimeSinceTarget passes MaxTrailTime: a lost ant lays nothing.
	 */
	SUPERCOLONY_API float GetDepositRate(float BaseRate, float TimeSinceTarget, float FalloffTime, float MaxTrailTime = UE_BIG_NUMBER);

	/** Yaw wrapped into (-PI, PI]. */
	SUPERCOLONY_API float WrapYaw(float Yaw);
}
