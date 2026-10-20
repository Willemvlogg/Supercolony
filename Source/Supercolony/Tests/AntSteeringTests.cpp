#include "Misc/AutomationTest.h"
#include "Agents/AntSteering.h"
#include "Colony/EmergenceGate.h"
#include "Colony/RollingEventCounter.h"
#include "Math/RandomStream.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AntSteeringTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	constexpr float TestTolerance = 1e-4f;
	const FVector2D BoundsMin(-1000.f, -1000.f);
	const FVector2D BoundsMax(1000.f, 1000.f);

	bool YawEquals(float A, float B)
	{
		return FMath::Abs(FMath::FindDeltaAngleRadians(A, B)) < TestTolerance;
	}

	/** Variance of the yaw spread after running many ants for one second at the given step length. */
	float WanderVarianceAfterOneSecond(float DeltaTime)
	{
		FRandomStream Random(1234);
		const int32 NumAnts = 4000;
		const int32 NumSteps = FMath::RoundToInt32(1.f / DeltaTime);
		double SumSquares = 0.0;
		for (int32 Ant = 0; Ant < NumAnts; ++Ant)
		{
			float Yaw = 0.f;
			for (int32 Step = 0; Step < NumSteps; ++Step)
			{
				Yaw = AntSteering::ApplyWander(Yaw, 0.3f, FMath::DegreesToRadians(240.f), DeltaTime, Random);
			}
			SumSquares += FMath::Square(Yaw);
		}
		return static_cast<float>(SumSquares / NumAnts);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntSensorPositionsTest, "Supercolony.Ants.SensorPositions", AntSteeringTests::TestFlags)
bool FAntSensorPositionsTest::RunTest(const FString& Parameters)
{
	FVector2D Left, Ahead, Right;
	AntSteering::GetSensorPositions(FVector2D(100.f, 50.f), 0.f, FMath::DegreesToRadians(90.f), 200.f, Left, Ahead, Right);
	TestTrue(TEXT("Ahead is Distance along the heading"), Ahead.Equals(FVector2D(300.f, 50.f), AntSteeringTests::TestTolerance));
	TestTrue(TEXT("Left is towards -Y when facing +X"), Left.Equals(FVector2D(100.f, -150.f), AntSteeringTests::TestTolerance));
	TestTrue(TEXT("Right is towards +Y when facing +X"), Right.Equals(FVector2D(100.f, 250.f), AntSteeringTests::TestTolerance));

	AntSteering::GetSensorPositions(FVector2D::ZeroVector, FMath::DegreesToRadians(90.f), FMath::DegreesToRadians(35.f), 200.f, Left, Ahead, Right);
	TestTrue(TEXT("Ahead follows a rotated heading"), Ahead.Equals(FVector2D(0.f, 200.f), AntSteeringTests::TestTolerance));
	TestEqual(TEXT("All sensors sit at Distance"), static_cast<float>(Left.Size()), 200.f, AntSteeringTests::TestTolerance);
	TestEqual(TEXT("All sensors sit at Distance"), static_cast<float>(Right.Size()), 200.f, AntSteeringTests::TestTolerance);
	TestTrue(TEXT("Facing +Y, the left sensor is towards +X"), Left.X > 0.f);
	TestTrue(TEXT("Facing +Y, the right sensor is towards -X"), Right.X < 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntChooseTurnTest, "Supercolony.Ants.ChooseTurn", AntSteeringTests::TestFlags)
bool FAntChooseTurnTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Strongest ahead goes straight"), AntSteering::ChooseTurn(1.f, 3.f, 2.f), 0.f);
	TestEqual(TEXT("Strongest left turns left"), AntSteering::ChooseTurn(3.f, 1.f, 2.f), -1.f);
	TestEqual(TEXT("Strongest right turns right"), AntSteering::ChooseTurn(1.f, 2.f, 3.f), 1.f);
	TestEqual(TEXT("All zero goes straight"), AntSteering::ChooseTurn(0.f, 0.f, 0.f), 0.f);
	TestEqual(TEXT("Ahead tied with a side goes straight"), AntSteering::ChooseTurn(2.f, 2.f, 1.f), 0.f);
	TestEqual(TEXT("Left and right tied above ahead goes straight"), AntSteering::ChooseTurn(2.f, 1.f, 2.f), 0.f);
	TestEqual(TEXT("Only one side has scent"), AntSteering::ChooseTurn(0.f, 0.f, 0.01f), 1.f);

	TestEqual(TEXT("Faint side below MinValue is ignored"), AntSteering::ChooseTurn(0.f, 0.f, 0.01f, 0.02f), 0.f);
	TestEqual(TEXT("Side at MinValue still counts"), AntSteering::ChooseTurn(0.f, 0.f, 0.02f, 0.02f), 1.f);
	TestEqual(TEXT("Strong side wins over a faint ahead"), AntSteering::ChooseTurn(0.5f, 0.01f, 0.f, 0.02f), -1.f);
	TestEqual(TEXT("Faint ahead no longer holds against a strong side"), AntSteering::ChooseTurn(0.f, 0.015f, 0.03f, 0.02f), 1.f);
	TestEqual(TEXT("All faint goes straight"), AntSteering::ChooseTurn(0.01f, 0.015f, 0.019f, 0.02f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntWanderTest, "Supercolony.Ants.Wander", AntSteeringTests::TestFlags)
bool FAntWanderTest::RunTest(const FString& Parameters)
{
	const float TurnRate = FMath::DegreesToRadians(240.f);
	const float DeltaTime = 1.f / 60.f;

	FRandomStream Random(42);
	TestEqual(TEXT("Zero strength never turns"), AntSteering::ApplyWander(0.5f, 0.f, TurnRate, DeltaTime, Random), 0.5f);

	FRandomStream RandomA(7);
	FRandomStream RandomB(7);
	TestEqual(TEXT("Same seed gives the same turn"),
		AntSteering::ApplyWander(0.f, 0.3f, TurnRate, DeltaTime, RandomA),
		AntSteering::ApplyWander(0.f, 0.3f, TurnRate, DeltaTime, RandomB));

	const float MaxStep = 0.3f * TurnRate * DeltaTime;
	bool bAllWithinStep = true;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		bAllWithinStep &= FMath::Abs(AntSteering::ApplyWander(0.f, 0.3f, TurnRate, DeltaTime, Random)) <= MaxStep + AntSteeringTests::TestTolerance;
	}
	TestTrue(TEXT("At 60 fps a step is at most Strength * TurnRate * DeltaTime"), bAllWithinStep);

	const float Variance30 = AntSteeringTests::WanderVarianceAfterOneSecond(1.f / 30.f);
	const float Variance120 = AntSteeringTests::WanderVarianceAfterOneSecond(1.f / 120.f);
	TestTrue(FString::Printf(TEXT("Spread after 1 s is frame-rate independent (30 fps %.4f vs 120 fps %.4f)"), Variance30, Variance120),
		FMath::Abs(Variance30 - Variance120) < 0.1f * Variance120);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntTurnTowardTest, "Supercolony.Ants.TurnToward", AntSteeringTests::TestFlags)
bool FAntTurnTowardTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Small gap closes fully"), AntSteeringTests::YawEquals(AntSteering::TurnToward(0.f, 0.1f, 0.5f), 0.1f));
	TestTrue(TEXT("Large gap is limited by MaxStep"), AntSteeringTests::YawEquals(AntSteering::TurnToward(0.f, 2.f, 0.5f), 0.5f));
	TestTrue(TEXT("Turns left for a target on the left"), AntSteeringTests::YawEquals(AntSteering::TurnToward(0.f, -2.f, 0.5f), -0.5f));

	const float NearPi = FMath::DegreesToRadians(170.f);
	TestTrue(TEXT("Takes the short way across +-180"),
		AntSteeringTests::YawEquals(AntSteering::TurnToward(NearPi, -NearPi, 0.5f), -NearPi));
	TestTrue(TEXT("Result stays wrapped"), FMath::Abs(AntSteering::TurnToward(NearPi, -NearPi + 0.1f, 1.f)) <= UE_PI);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntClampToBoundsTest, "Supercolony.Ants.ClampToBounds", AntSteeringTests::TestFlags)
bool FAntClampToBoundsTest::RunTest(const FString& Parameters)
{
	FVector2D Position(10.f, -20.f);
	TestFalse(TEXT("Inside the bounds nothing changes"), AntSteering::ClampToBounds(Position, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax));
	TestTrue(TEXT("Inside position kept"), Position.Equals(FVector2D(10.f, -20.f)));

	Position = FVector2D(1010.f, 0.f);
	TestTrue(TEXT("Past +X is clamped"), AntSteering::ClampToBounds(Position, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax));
	TestTrue(TEXT("+X clamp position"), Position.Equals(FVector2D(1000.f, 0.f)));

	Position = FVector2D(-1010.f, -1010.f);
	TestTrue(TEXT("Past a corner is clamped"), AntSteering::ClampToBounds(Position, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax));
	TestTrue(TEXT("Corner clamp position"), Position.Equals(FVector2D(-1000.f, -1000.f)));

	Position = FVector2D(1000.f, 1000.f);
	TestFalse(TEXT("Exactly on the bounds is not a clamp"), AntSteering::ClampToBounds(Position, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntDepositRateTest, "Supercolony.Ants.DepositRate", AntSteeringTests::TestFlags)
bool FAntDepositRateTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Full rate on leaving a target"), AntSteering::GetDepositRate(1.f, 0.f, 20.f), 1.f, AntSteeringTests::TestTolerance);
	TestEqual(TEXT("1/e after one falloff time"), AntSteering::GetDepositRate(1.f, 20.f, 20.f), FMath::Exp(-1.f), AntSteeringTests::TestTolerance);
	TestEqual(TEXT("Scales with the base rate"), AntSteering::GetDepositRate(2.f, 20.f, 20.f), 2.f * FMath::Exp(-1.f), AntSteeringTests::TestTolerance);
	TestEqual(TEXT("Negative time counts as zero"), AntSteering::GetDepositRate(1.f, -5.f, 20.f), 1.f, AntSteeringTests::TestTolerance);

	TestEqual(TEXT("Before MaxTrailTime the falloff applies"), AntSteering::GetDepositRate(1.f, 60.f, 20.f, 60.f), FMath::Exp(-3.f), AntSteeringTests::TestTolerance);
	TestEqual(TEXT("Past MaxTrailTime a lost ant lays nothing"), AntSteering::GetDepositRate(1.f, 60.01f, 20.f, 60.f), 0.f);
	TestEqual(TEXT("MaxTrailTime of zero only allows the first instant"), AntSteering::GetDepositRate(1.f, 0.1f, 20.f, 0.f), 0.f);

	bool bDecreasing = true;
	float Previous = AntSteering::GetDepositRate(1.f, 0.f, 20.f);
	for (float Time = 1.f; Time <= 120.f; Time += 1.f)
	{
		const float Current = AntSteering::GetDepositRate(1.f, Time, 20.f);
		bDecreasing &= Current < Previous && Current > 0.f;
		Previous = Current;
	}
	TestTrue(TEXT("Always decreasing and positive"), bDecreasing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRollingEventCounterTest, "Supercolony.Colony.RollingEventCounter", AntSteeringTests::TestFlags)
bool FRollingEventCounterTest::RunTest(const FString& Parameters)
{
	FRollingEventCounter Counter(60.0);
	TestEqual(TEXT("Starts empty"), Counter.GetCount(0.0), 0);

	Counter.Add(1.0);
	Counter.Add(1.0, 2);
	Counter.Add(30.0);
	TestEqual(TEXT("Counts every event in the window"), Counter.GetCount(30.0), 4);
	TestEqual(TEXT("Events just inside the window are kept"), Counter.GetCount(60.9), 4);
	TestEqual(TEXT("Events exactly one window old drop out"), Counter.GetCount(61.0), 1);
	TestEqual(TEXT("Everything drops out eventually"), Counter.GetCount(200.0), 0);

	Counter.Add(201.0, 5);
	Counter.Reset();
	TestEqual(TEXT("Reset clears the count"), Counter.GetCount(201.0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntEdgeAvoidanceTest, "Supercolony.Ants.EdgeAvoidance", AntSteeringTests::TestFlags)
bool FAntEdgeAvoidanceTest::RunTest(const FString& Parameters)
{
	using namespace AntSteeringTests;
	constexpr float AvoidDistance = 300.f;
	FVector2D Away;

	TestEqual(TEXT("No push in the middle"), AntSteering::GetEdgeAvoidance(FVector2D::ZeroVector, BoundsMin, BoundsMax, AvoidDistance, Away), 0.f);
	TestTrue(TEXT("No direction in the middle"), Away.IsZero());

	TestEqual(TEXT("No push exactly at AvoidDistance"), AntSteering::GetEdgeAvoidance(FVector2D(700.f, 0.f), BoundsMin, BoundsMax, AvoidDistance, Away), 0.f);

	TestEqual(TEXT("Half push half way in"), AntSteering::GetEdgeAvoidance(FVector2D(850.f, 0.f), BoundsMin, BoundsMax, AvoidDistance, Away), 0.5f, TestTolerance);
	TestTrue(TEXT("+X wall pushes towards -X"), Away.Equals(FVector2D(-1.f, 0.f), TestTolerance));

	TestEqual(TEXT("Full push at the wall"), AntSteering::GetEdgeAvoidance(FVector2D(0.f, -1000.f), BoundsMin, BoundsMax, AvoidDistance, Away), 1.f, TestTolerance);
	TestTrue(TEXT("-Y wall pushes towards +Y"), Away.Equals(FVector2D(0.f, 1.f), TestTolerance));

	AntSteering::GetEdgeAvoidance(FVector2D(-900.f, 0.f), BoundsMin, BoundsMax, AvoidDistance, Away);
	TestTrue(TEXT("-X wall pushes towards +X"), Away.Equals(FVector2D(1.f, 0.f), TestTolerance));
	AntSteering::GetEdgeAvoidance(FVector2D(0.f, 900.f), BoundsMin, BoundsMax, AvoidDistance, Away);
	TestTrue(TEXT("+Y wall pushes towards -Y"), Away.Equals(FVector2D(0.f, -1.f), TestTolerance));

	TestEqual(TEXT("Corner push is full strength"), AntSteering::GetEdgeAvoidance(FVector2D(1000.f, 1000.f), BoundsMin, BoundsMax, AvoidDistance, Away), 1.f, TestTolerance);
	TestTrue(TEXT("Corner pushes diagonally inwards"), Away.Equals(FVector2D(-UE_INV_SQRT_2, -UE_INV_SQRT_2), TestTolerance));

	TestEqual(TEXT("AvoidDistance of zero disables it"), AntSteering::GetEdgeAvoidance(FVector2D(1000.f, 0.f), BoundsMin, BoundsMax, 0.f, Away), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntTrailSensingTest, "Supercolony.Ants.TrailSensing", AntSteeringTests::TestFlags)
bool FAntTrailSensingTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Nothing sensed when all zero"), AntSteering::ReadSensors(0.f, 0.f, 0.f, 0.02f).bTrailSensed);
	TestFalse(TEXT("Nothing sensed when all below MinValue"), AntSteering::ReadSensors(0.01f, 0.019f, 0.f, 0.02f).bTrailSensed);
	TestTrue(TEXT("One sensor at MinValue counts"), AntSteering::ReadSensors(0.f, 0.02f, 0.f, 0.02f).bTrailSensed);
	TestTrue(TEXT("Trail straight ahead is sensed but does not turn"), AntSteering::ReadSensors(0.f, 1.f, 0.f, 0.02f).bTrailSensed);
	TestEqual(TEXT("ReadSensors turn matches ChooseTurn"), AntSteering::ReadSensors(0.1f, 0.f, 0.5f, 0.02f).Turn, AntSteering::ChooseTurn(0.1f, 0.f, 0.5f, 0.02f));

	TestEqual(TEXT("On trail uses the on-trail wander"), AntSteering::GetWanderStrength(true, 0.15f, 0.5f), 0.15f);
	TestEqual(TEXT("Off trail uses the off-trail wander"), AntSteering::GetWanderStrength(false, 0.15f, 0.5f), 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntHomingTest, "Supercolony.Ants.Homing", AntSteeringTests::TestFlags)
bool FAntHomingTest::RunTest(const FString& Parameters)
{
	using namespace AntSteeringTests;
	const float TurnRate = FMath::DegreesToRadians(240.f);
	const float DeltaTime = 1.f / 60.f;
	const float MaxStep = 0.3f * TurnRate * DeltaTime;

	// Ant at +X facing +Y; the nest at the origin is directly behind-left (yaw 180).
	const FVector2D Position(1000.f, 0.f);
	const float Facing = FMath::DegreesToRadians(90.f);
	const float Biased = AntSteering::ApplyHomingBias(Facing, Position, FVector2D::ZeroVector, 0.3f, TurnRate, DeltaTime);
	TestTrue(TEXT("Turns towards the nest by Bias * TurnRate * DeltaTime"), YawEquals(Biased, Facing + MaxStep));

	const float AlmostThere = FMath::DegreesToRadians(179.9f);
	TestTrue(TEXT("Small gaps close fully"), YawEquals(AntSteering::ApplyHomingBias(AlmostThere, Position, FVector2D::ZeroVector, 0.3f, TurnRate, DeltaTime), UE_PI));
	TestEqual(TEXT("Zero bias does nothing"), AntSteering::ApplyHomingBias(Facing, Position, FVector2D::ZeroVector, 0.f, TurnRate, DeltaTime), Facing);
	TestEqual(TEXT("At the nest does nothing"), AntSteering::ApplyHomingBias(Facing, FVector2D::ZeroVector, FVector2D::ZeroVector, 0.3f, TurnRate, DeltaTime), Facing);

	TestFalse(TEXT("Searching ant keeps going before MaxSearchTime"), AntSteering::ShouldGiveUpSearch(true, 44.9f, 45.f));
	TestFalse(TEXT("Exactly MaxSearchTime is not yet giving up"), AntSteering::ShouldGiveUpSearch(true, 45.f, 45.f));
	TestTrue(TEXT("Searching ant gives up after MaxSearchTime"), AntSteering::ShouldGiveUpSearch(true, 45.1f, 45.f));
	TestFalse(TEXT("Returning or homing ants never give up"), AntSteering::ShouldGiveUpSearch(false, 1000.f, 45.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntSpeedScaleTest, "Supercolony.Ants.SpeedScale", AntSteeringTests::TestFlags)
bool FAntSpeedScaleTest::RunTest(const FString& Parameters)
{
	FRandomStream Random(99);
	float Lowest = 2.f;
	float Highest = 0.f;
	for (int32 Index = 0; Index < 10000; ++Index)
	{
		const float Scale = AntSteering::RandomSpeedScale(0.2f, Random);
		Lowest = FMath::Min(Lowest, Scale);
		Highest = FMath::Max(Highest, Scale);
	}
	TestTrue(TEXT("Never below 1 - Variation"), Lowest >= 0.8f);
	TestTrue(TEXT("Never above 1 + Variation"), Highest <= 1.2f);
	TestTrue(TEXT("Uses most of the range"), Lowest < 0.81f && Highest > 1.19f);

	FRandomStream RandomA(5);
	FRandomStream RandomB(5);
	TestEqual(TEXT("Same seed gives the same speed"), AntSteering::RandomSpeedScale(0.2f, RandomA), AntSteering::RandomSpeedScale(0.2f, RandomB));
	TestEqual(TEXT("Zero variation is exactly 1"), AntSteering::RandomSpeedScale(0.f, Random), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmergenceGateTest, "Supercolony.Colony.EmergenceGate", AntSteeringTests::TestFlags)
bool FEmergenceGateTest::RunTest(const FString& Parameters)
{
	FEmergenceGate Gate;
	Gate.Reset(120);
	TestEqual(TEXT("Nobody out before the first advance"), Gate.GetReleased(), 0);
	TestFalse(TEXT("Ant 0 still inside"), Gate.IsReleased(0));

	Gate.Advance(50.f, 1.f);
	TestEqual(TEXT("50 per second for 1 s releases 50"), Gate.GetReleased(), 50);
	TestTrue(TEXT("Ant 49 is out"), Gate.IsReleased(49));
	TestFalse(TEXT("Ant 50 is still inside"), Gate.IsReleased(50));

	// Half an ant per frame: nothing would ever come out unless the fraction carries over.
	for (int32 Frame = 0; Frame < 100; ++Frame)
	{
		Gate.Advance(50.f, 0.01f);
	}
	TestEqual(TEXT("Fractions carry over between small frames"), Gate.GetReleased(), 100);

	Gate.Advance(10.f, 1.f);
	TestEqual(TEXT("A rate change applies from the next advance"), Gate.GetReleased(), 110);

	Gate.Advance(50.f, 10.f);
	TestEqual(TEXT("Never more than the total"), Gate.GetReleased(), 120);

	Gate.Reset(30);
	Gate.Advance(0.f, 0.01f);
	TestEqual(TEXT("Rate 0 releases everyone at once"), Gate.GetReleased(), 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntScoutAssignmentTest, "Supercolony.Ants.ScoutAssignment", AntSteeringTests::TestFlags)
bool FAntScoutAssignmentTest::RunTest(const FString& Parameters)
{
	// Mirrors spawning: each ant has its own stream, seeded from a base seed plus its index.
	auto CountScouts = [](int32 BaseSeed, int32 NumAnts, float Fraction)
	{
		int32 Scouts = 0;
		for (int32 Index = 0; Index < NumAnts; ++Index)
		{
			FRandomStream Random(BaseSeed + Index);
			Scouts += AntSteering::RollScout(Fraction, Random) ? 1 : 0;
		}
		return Scouts;
	};

	constexpr int32 NumAnts = 5000;
	for (const float Fraction : { 0.1f, 0.25f, 0.5f })
	{
		// Binomial standard deviation for 5,000 ants at 10% is about 0.4%; 2% is a safe tolerance.
		const float Share = static_cast<float>(CountScouts(1234, NumAnts, Fraction)) / NumAnts;
		TestTrue(FString::Printf(TEXT("Scout share %.3f is within 0.02 of %.2f"), Share, Fraction), FMath::Abs(Share - Fraction) < 0.02f);
	}

	TestEqual(TEXT("Same seed gives the same scouts"), CountScouts(77, NumAnts, 0.1f), CountScouts(77, NumAnts, 0.1f));

	bool bSameAnts = true;
	for (int32 Index = 0; Index < 200; ++Index)
	{
		FRandomStream RandomA(500 + Index);
		FRandomStream RandomB(500 + Index);
		bSameAnts &= AntSteering::RollScout(0.1f, RandomA) == AntSteering::RollScout(0.1f, RandomB);
	}
	TestTrue(TEXT("Same seed picks the same individual ants"), bSameAnts);

	TestEqual(TEXT("Fraction 0 gives no scouts"), CountScouts(1, 1000, 0.f), 0);
	TestEqual(TEXT("Fraction 1 makes every ant a scout"), CountScouts(1, 1000, 1.f), 1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntRecruitBoostTest, "Supercolony.Ants.RecruitBoost", AntSteeringTests::TestFlags)
bool FAntRecruitBoostTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No Food trail at the pickup: boosted trip home"), AntSteering::GetRecruitBoost(false, 3.f), 3.f);
	TestEqual(TEXT("Existing Food trail: normal deposit"), AntSteering::GetRecruitBoost(true, 3.f), 1.f);
	TestEqual(TEXT("A boost below 1 never weakens the trail"), AntSteering::GetRecruitBoost(false, 0.5f), 1.f);

	// The decision uses the same sensing as steering: all three sensors below MinSensorValue means no trail.
	TestEqual(TEXT("Faint Food below MinSensorValue counts as unmarked"),
		AntSteering::GetRecruitBoost(AntSteering::ReadSensors(0.01f, 0.015f, 0.f, 0.02f).bTrailSensed, 3.f), 3.f);
	TestEqual(TEXT("One sensor on a trail is enough to skip the boost"),
		AntSteering::GetRecruitBoost(AntSteering::ReadSensors(0.f, 0.f, 0.05f, 0.02f).bTrailSensed, 3.f), 1.f);
	return true;
}

#endif
