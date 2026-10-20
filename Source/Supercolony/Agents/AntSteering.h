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

	/** What the three sensors say: which way to turn, and whether any of them picked up a trail at all. */
	struct FSensorReading
	{
		/** -1 left, 0 straight, +1 right. */
		float Turn = 0.f;
		bool bTrailSensed = false;
	};

	/**
	 * Reads the three sensors. Values below MinValue count as zero, so faint trails are ignored.
	 * Straight when ahead is at least as strong as both sides, when all are zero, or when left and right tie.
	 */
	SUPERCOLONY_API FSensorReading ReadSensors(float Left, float Ahead, float Right, float MinValue = 0.f);

	/** Just the turn from ReadSensors. */
	SUPERCOLONY_API float ChooseTurn(float Left, float Ahead, float Right, float MinValue = 0.f);

	/** Ants wander less while following a trail and more while exploring. */
	SUPERCOLONY_API float GetWanderStrength(bool bTrailSensed, float OnTrail, float OffTrail);

	/**
	 * Adds a random turn. Strength is a fraction of TurnRate (radians per second).
	 * The step scales with sqrt(DeltaTime), like a random walk, so the spread of headings after one second
	 * does not depend on the frame rate. At 60 fps a step is up to Strength * TurnRate * DeltaTime.
	 */
	SUPERCOLONY_API float ApplyWander(float Yaw, float Strength, float TurnRate, float DeltaTime, FRandomStream& Random);

	/** Turns Yaw towards TargetYaw the short way round, by at most MaxStep radians. */
	SUPERCOLONY_API float TurnToward(float Yaw, float TargetYaw, float MaxStep);

	/**
	 * Gentle pull back to the nest for a homing ant that senses no Home trail (a simple form of path integration).
	 * Turns towards the nest by at most Bias * TurnRate * DeltaTime, so wander still bends the path.
	 */
	SUPERCOLONY_API float ApplyHomingBias(float Yaw, const FVector2D& Position, const FVector2D& Nest, float Bias, float TurnRate, float DeltaTime);

	/**
	 * How strongly to steer away from the bounds, 0 to 1, and which way (unit vector pointing inwards).
	 * Grows linearly from 0 at AvoidDistance from a wall to 1 at the wall. In a corner both walls push, so the
	 * direction is diagonal. Returns 0 and leaves OutAway at zero away from all walls.
	 */
	SUPERCOLONY_API float GetEdgeAvoidance(const FVector2D& Position, const FVector2D& Min, const FVector2D& Max, float AvoidDistance,
		FVector2D& OutAway);

	/** Safety net behind edge avoidance: clamps Position into [Min, Max]. Returns true if it had to. */
	SUPERCOLONY_API bool ClampToBounds(FVector2D& Position, const FVector2D& Min, const FVector2D& Max);

	/** Per-ant speed multiplier in [1 - Variation, 1 + Variation]. */
	SUPERCOLONY_API float RandomSpeedScale(float Variation, FRandomStream& Random);

	/** True for roughly Fraction of calls; used once per ant at spawn to pick scouts. */
	SUPERCOLONY_API bool RollScout(float Fraction, FRandomStream& Random);

	/** A searching ant gives up and heads home once it has gone MaxSearchTime without finding food. */
	SUPERCOLONY_API bool ShouldGiveUpSearch(bool bSearching, float TimeSinceTarget, float MaxSearchTime);

	/**
	 * Deposit per second: BaseRate * exp(-TimeSinceTarget / FalloffTime). Strongest right after leaving a target.
	 * Zero once TimeSinceTarget passes MaxTrailTime: a lost ant lays nothing.
	 */
	SUPERCOLONY_API float GetDepositRate(float BaseRate, float TimeSinceTarget, float FalloffTime, float MaxTrailTime = UE_BIG_NUMBER);

	/** Yaw wrapped into (-PI, PI]. */
	SUPERCOLONY_API float WrapYaw(float Yaw);
}
