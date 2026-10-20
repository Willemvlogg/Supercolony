#include "AntSteering.h"
#include "Math/RandomStream.h"

namespace AntSteering
{
	FVector2D HeadingToDirection(float Yaw)
	{
		float Sin, Cos;
		FMath::SinCos(&Sin, &Cos, Yaw);
		return FVector2D(Cos, Sin);
	}

	void GetSensorPositions(const FVector2D& Position, float Yaw, float SensorAngle, float Distance,
		FVector2D& OutLeft, FVector2D& OutAhead, FVector2D& OutRight)
	{
		OutLeft = Position + HeadingToDirection(Yaw - SensorAngle) * Distance;
		OutAhead = Position + HeadingToDirection(Yaw) * Distance;
		OutRight = Position + HeadingToDirection(Yaw + SensorAngle) * Distance;
	}

	FSensorReading ReadSensors(float Left, float Ahead, float Right, float MinValue)
	{
		Left = Left >= MinValue ? Left : 0.f;
		Ahead = Ahead >= MinValue ? Ahead : 0.f;
		Right = Right >= MinValue ? Right : 0.f;

		FSensorReading Reading;
		Reading.bTrailSensed = Left > 0.f || Ahead > 0.f || Right > 0.f;
		if (Ahead >= Left && Ahead >= Right)
		{
			return Reading;
		}
		if (Left == Right)
		{
			return Reading;
		}
		Reading.Turn = Right > Left ? 1.f : -1.f;
		return Reading;
	}

	float ChooseTurn(float Left, float Ahead, float Right, float MinValue)
	{
		return ReadSensors(Left, Ahead, Right, MinValue).Turn;
	}

	float GetWanderStrength(bool bTrailSensed, float OnTrail, float OffTrail)
	{
		return bTrailSensed ? OnTrail : OffTrail;
	}

	float ApplyWander(float Yaw, float Strength, float TurnRate, float DeltaTime, FRandomStream& Random)
	{
		const float MaxStep = Strength * TurnRate * FMath::Sqrt(FMath::Max(DeltaTime, 0.f) * WanderReferenceStep);
		return WrapYaw(Yaw + Random.FRandRange(-1.f, 1.f) * MaxStep);
	}

	float TurnToward(float Yaw, float TargetYaw, float MaxStep)
	{
		const float Delta = FMath::FindDeltaAngleRadians(Yaw, TargetYaw);
		return WrapYaw(Yaw + FMath::Clamp(Delta, -MaxStep, MaxStep));
	}

	float ApplyHomingBias(float Yaw, const FVector2D& Position, const FVector2D& Nest, float Bias, float TurnRate, float DeltaTime)
	{
		const FVector2D ToNest = Nest - Position;
		if (Bias <= 0.f || ToNest.IsNearlyZero())
		{
			return Yaw;
		}
		return TurnToward(Yaw, FMath::Atan2(ToNest.Y, ToNest.X), Bias * TurnRate * DeltaTime);
	}

	float GetEdgeAvoidance(const FVector2D& Position, const FVector2D& Min, const FVector2D& Max, float AvoidDistance,
		FVector2D& OutAway)
	{
		OutAway = FVector2D::ZeroVector;
		if (AvoidDistance <= 0.f)
		{
			return 0.f;
		}

		// Each wall pushes inwards with 1 - distance / AvoidDistance, clamped to [0, 1].
		auto Push = [AvoidDistance](float DistanceToWall)
		{
			return FMath::Clamp(1.f - DistanceToWall / AvoidDistance, 0.f, 1.f);
		};
		const FVector2D Away(
			Push(Position.X - Min.X) - Push(Max.X - Position.X),
			Push(Position.Y - Min.Y) - Push(Max.Y - Position.Y));

		const float Strength = FMath::Min(1.f, FMath::Max(FMath::Abs(Away.X), FMath::Abs(Away.Y)));
		if (Strength <= 0.f)
		{
			return 0.f;
		}
		OutAway = Away.GetSafeNormal();
		return Strength;
	}

	bool ClampToBounds(FVector2D& Position, const FVector2D& Min, const FVector2D& Max)
	{
		const FVector2D Clamped(FMath::Clamp(Position.X, Min.X, Max.X), FMath::Clamp(Position.Y, Min.Y, Max.Y));
		const bool bClamped = Clamped != Position;
		Position = Clamped;
		return bClamped;
	}

	float RandomSpeedScale(float Variation, FRandomStream& Random)
	{
		return 1.f + Random.FRandRange(-Variation, Variation);
	}

	float GetRecruitBoost(bool bFoodTrailSensed, float RecruitBoost)
	{
		return bFoodTrailSensed ? 1.f : FMath::Max(RecruitBoost, 1.f);
	}

	bool RollScout(float Fraction, FRandomStream& Random)
	{
		return Random.FRand() < Fraction;
	}

	bool ShouldGiveUpSearch(bool bSearching, float TimeSinceTarget, float MaxSearchTime)
	{
		return bSearching && TimeSinceTarget > MaxSearchTime;
	}

	float GetDepositRate(float BaseRate, float TimeSinceTarget, float FalloffTime, float MaxTrailTime)
	{
		if (TimeSinceTarget > MaxTrailTime)
		{
			return 0.f;
		}
		return BaseRate * FMath::Exp(-FMath::Max(TimeSinceTarget, 0.f) / FalloffTime);
	}

	float WrapYaw(float Yaw)
	{
		return FMath::UnwindRadians(Yaw);
	}
}
