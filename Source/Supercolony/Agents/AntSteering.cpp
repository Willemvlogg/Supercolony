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

	float ChooseTurn(float Left, float Ahead, float Right)
	{
		if (Ahead >= Left && Ahead >= Right)
		{
			return 0.f;
		}
		if (Left == Right)
		{
			return 0.f;
		}
		return Right > Left ? 1.f : -1.f;
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

	bool KeepInBounds(FVector2D& Position, float& Yaw, const FVector2D& Min, const FVector2D& Max)
	{
		const FVector2D Direction = HeadingToDirection(Yaw);
		bool bClamped = false;

		if (Position.X <= Min.X || Position.X >= Max.X)
		{
			const bool bMovingOut = Position.X <= Min.X ? Direction.X < 0.f : Direction.X > 0.f;
			if (bMovingOut)
			{
				Yaw = WrapYaw(UE_PI - Yaw);
			}
			Position.X = FMath::Clamp(Position.X, Min.X, Max.X);
			bClamped = true;
		}

		if (Position.Y <= Min.Y || Position.Y >= Max.Y)
		{
			const bool bMovingOut = Position.Y <= Min.Y ? Direction.Y < 0.f : Direction.Y > 0.f;
			if (bMovingOut)
			{
				Yaw = WrapYaw(-Yaw);
			}
			Position.Y = FMath::Clamp(Position.Y, Min.Y, Max.Y);
			bClamped = true;
		}

		return bClamped;
	}

	float GetDepositRate(float BaseRate, float TimeSinceTarget, float FalloffTime)
	{
		return BaseRate * FMath::Exp(-FMath::Max(TimeSinceTarget, 0.f) / FalloffTime);
	}

	float WrapYaw(float Yaw)
	{
		return FMath::UnwindRadians(Yaw);
	}
}
