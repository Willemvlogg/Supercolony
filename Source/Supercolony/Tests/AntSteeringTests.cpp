#include "Misc/AutomationTest.h"
#include "Agents/AntSteering.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntKeepInBoundsTest, "Supercolony.Ants.KeepInBounds", AntSteeringTests::TestFlags)
bool FAntKeepInBoundsTest::RunTest(const FString& Parameters)
{
	FVector2D Position(0.f, 0.f);
	float Yaw = 0.3f;
	TestFalse(TEXT("Inside the bounds nothing changes"), AntSteering::KeepInBounds(Position, Yaw, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax));
	TestEqual(TEXT("Heading unchanged inside"), Yaw, 0.3f);

	struct FWallCase
	{
		const TCHAR* Name;
		FVector2D Position;
		float YawDegrees;
		FVector2D ExpectedPosition;
		float ExpectedYawDegrees;
	};
	const FWallCase Cases[] =
	{
		{ TEXT("+X wall"), FVector2D(1010.f, 0.f), 30.f, FVector2D(1000.f, 0.f), 150.f },
		{ TEXT("-X wall"), FVector2D(-1010.f, 0.f), 150.f, FVector2D(-1000.f, 0.f), 30.f },
		{ TEXT("+Y wall"), FVector2D(0.f, 1010.f), 60.f, FVector2D(0.f, 1000.f), -60.f },
		{ TEXT("-Y wall"), FVector2D(0.f, -1010.f), -60.f, FVector2D(0.f, -1000.f), 60.f },
		{ TEXT("+X+Y corner"), FVector2D(1010.f, 1010.f), 45.f, FVector2D(1000.f, 1000.f), -135.f },
		{ TEXT("-X-Y corner"), FVector2D(-1010.f, -1010.f), -135.f, FVector2D(-1000.f, -1000.f), 45.f },
	};

	for (const FWallCase& Case : Cases)
	{
		Position = Case.Position;
		Yaw = FMath::DegreesToRadians(Case.YawDegrees);
		TestTrue(FString::Printf(TEXT("%s: clamped"), Case.Name), AntSteering::KeepInBounds(Position, Yaw, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax));
		TestTrue(FString::Printf(TEXT("%s: position"), Case.Name), Position.Equals(Case.ExpectedPosition, AntSteeringTests::TestTolerance));
		TestTrue(FString::Printf(TEXT("%s: heading reflected"), Case.Name), AntSteeringTests::YawEquals(Yaw, FMath::DegreesToRadians(Case.ExpectedYawDegrees)));
	}

	Position = FVector2D(1010.f, 0.f);
	Yaw = FMath::DegreesToRadians(150.f);
	AntSteering::KeepInBounds(Position, Yaw, AntSteeringTests::BoundsMin, AntSteeringTests::BoundsMax);
	TestTrue(TEXT("Already heading back in: heading kept"), AntSteeringTests::YawEquals(Yaw, FMath::DegreesToRadians(150.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAntDepositRateTest, "Supercolony.Ants.DepositRate", AntSteeringTests::TestFlags)
bool FAntDepositRateTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Full rate on leaving a target"), AntSteering::GetDepositRate(1.f, 0.f, 20.f), 1.f, AntSteeringTests::TestTolerance);
	TestEqual(TEXT("1/e after one falloff time"), AntSteering::GetDepositRate(1.f, 20.f, 20.f), FMath::Exp(-1.f), AntSteeringTests::TestTolerance);
	TestEqual(TEXT("Scales with the base rate"), AntSteering::GetDepositRate(2.f, 20.f, 20.f), 2.f * FMath::Exp(-1.f), AntSteeringTests::TestTolerance);
	TestEqual(TEXT("Negative time counts as zero"), AntSteering::GetDepositRate(1.f, -5.f, 20.f), 1.f, AntSteeringTests::TestTolerance);

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

#endif
