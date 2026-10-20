#include "Misc/AutomationTest.h"
#include "Pheromones/PheromoneGrid.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PheromoneGridTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	constexpr float TestCellSize = 100.f;
	constexpr float TestMaxValue = 10.f;
	constexpr float TestTolerance = 1e-4f;

	/** 10 x 10 cells of 1 m, with cell (0, 0) starting at the world origin. */
	FPheromoneGrid MakeGrid()
	{
		FPheromoneGrid Grid;
		Grid.Init(10, 10, TestCellSize, FVector2D::ZeroVector, TestMaxValue);
		return Grid;
	}

	void DepositInCell(FPheromoneGrid& Grid, const FIntPoint& Cell, EPheromoneChannel Channel, float Amount)
	{
		Grid.Deposit(Grid.CellToWorld(Cell), Channel, Amount);
	}
}

using namespace PheromoneGridTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridWorldToCellTest, "Supercolony.Pheromones.WorldToCell", TestFlags)
bool FPheromoneGridWorldToCellTest::RunTest(const FString& Parameters)
{
	const FPheromoneGrid Grid = MakeGrid();
	FIntPoint Cell;

	TestTrue(TEXT("Inside the first cell"), Grid.WorldToCell(FVector2D(50.f, 50.f), Cell));
	TestTrue(TEXT("First cell"), Cell == FIntPoint(0, 0));

	TestTrue(TEXT("Inside the last cell"), Grid.WorldToCell(FVector2D(999.f, 999.f), Cell));
	TestTrue(TEXT("Last cell"), Cell == FIntPoint(9, 9));

	TestTrue(TEXT("Exactly on a cell boundary belongs to the next cell"), Grid.WorldToCell(FVector2D(100.f, 0.f), Cell));
	TestTrue(TEXT("Boundary cell"), Cell == FIntPoint(1, 0));

	TestFalse(TEXT("Just past the far edge"), Grid.WorldToCell(FVector2D(1000.f, 500.f), Cell));
	TestFalse(TEXT("Just before the near edge"), Grid.WorldToCell(FVector2D(-0.01f, 500.f), Cell));

	TestTrue(TEXT("CellToWorld returns the cell centre"), Grid.CellToWorld(FIntPoint(3, 4)).Equals(FVector2D(350.f, 450.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridDepositTest, "Supercolony.Pheromones.Deposit", TestFlags)
bool FPheromoneGridDepositTest::RunTest(const FString& Parameters)
{
	FPheromoneGrid Grid = MakeGrid();

	Grid.Deposit(FVector2D(150.f, 250.f), EPheromoneChannel::Food, 2.f);
	TestEqual(TEXT("Deposit lands in the cell under the location"), Grid.GetCellValue(FIntPoint(1, 2), EPheromoneChannel::Food), 2.f);
	TestEqual(TEXT("Nothing else changes"), Grid.GetTotal(EPheromoneChannel::Food), 2.f);

	Grid.Deposit(FVector2D(199.f, 201.f), EPheromoneChannel::Food, 1.5f);
	// Soft cap: 2 + 1.5 * (1 - 2 / 10) = 3.2.
	TestEqual(TEXT("A second deposit in the same cell adds with diminishing returns"),
		Grid.Sample(FVector2D(150.f, 250.f), EPheromoneChannel::Food), 3.2f, TestTolerance);

	TestEqual(TEXT("Other channels are untouched"), Grid.GetTotal(EPheromoneChannel::Home), 0.f);

	Grid.Deposit(FVector2D(-50.f, 250.f), EPheromoneChannel::Food, 5.f);
	TestEqual(TEXT("Deposits outside the grid are ignored"), Grid.GetTotal(EPheromoneChannel::Food), 3.2f, TestTolerance);
	TestEqual(TEXT("Sampling outside the grid returns 0"), Grid.Sample(FVector2D(-50.f, 250.f), EPheromoneChannel::Food), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridSoftCapTest, "Supercolony.Pheromones.SoftCap", TestFlags)
bool FPheromoneGridSoftCapTest::RunTest(const FString& Parameters)
{
	FPheromoneGrid Grid = MakeGrid();
	const FIntPoint Cell(4, 4);

	DepositInCell(Grid, Cell, EPheromoneChannel::Home, 8.f);
	TestEqual(TEXT("An empty cell takes the full amount"), Grid.GetCellValue(Cell, EPheromoneChannel::Home), 8.f, TestTolerance);
	DepositInCell(Grid, Cell, EPheromoneChannel::Home, 8.f);
	TestEqual(TEXT("A second deposit adds Amount * (1 - current / max)"), Grid.GetCellValue(Cell, EPheromoneChannel::Home), 9.6f, TestTolerance);

	bool bNeverAboveMax = true;
	for (int32 Step = 0; Step < 1000; ++Step)
	{
		DepositInCell(Grid, Cell, EPheromoneChannel::Home, 5.f);
		bNeverAboveMax &= Grid.GetCellValue(Cell, EPheromoneChannel::Home) <= TestMaxValue;
	}
	TestTrue(TEXT("Many deposits never exceed MaxCellValue"), bNeverAboveMax);
	TestTrue(TEXT("Many deposits approach MaxCellValue"), Grid.GetCellValue(Cell, EPheromoneChannel::Home) > TestMaxValue - 0.01f);

	const FIntPoint Empty(1, 1);
	DepositInCell(Grid, Empty, EPheromoneChannel::Food, 1000.f);
	TestEqual(TEXT("A single huge deposit never exceeds the max"), Grid.GetCellValue(Empty, EPheromoneChannel::Food), TestMaxValue);

	const FIntPoint Sparse(2, 2);
	const FIntPoint Busy(7, 7);
	DepositInCell(Grid, Sparse, EPheromoneChannel::Alarm, 2.f);
	DepositInCell(Grid, Busy, EPheromoneChannel::Alarm, 8.f);
	DepositInCell(Grid, Sparse, EPheromoneChannel::Alarm, 1.f);
	DepositInCell(Grid, Busy, EPheromoneChannel::Alarm, 1.f);
	const float SparseGain = Grid.GetCellValue(Sparse, EPheromoneChannel::Alarm) - 2.f;
	const float BusyGain = Grid.GetCellValue(Busy, EPheromoneChannel::Alarm) - 8.f;
	TestEqual(TEXT("Emptier cell gains 1 * (1 - 0.2)"), SparseGain, 0.8f, TestTolerance);
	TestEqual(TEXT("Fuller cell gains 1 * (1 - 0.8)"), BusyGain, 0.2f, TestTolerance);
	TestTrue(TEXT("A fuller cell gains less than an emptier one"), BusyGain < SparseGain);

	DepositInCell(Grid, Busy, EPheromoneChannel::Alarm, -3.f);
	TestEqual(TEXT("Negative amounts subtract directly"), Grid.GetCellValue(Busy, EPheromoneChannel::Alarm), 5.2f, TestTolerance);
	DepositInCell(Grid, Busy, EPheromoneChannel::Alarm, -1000.f);
	TestEqual(TEXT("Values never go below zero"), Grid.GetCellValue(Busy, EPheromoneChannel::Alarm), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridEvaporationTest, "Supercolony.Pheromones.Evaporation", TestFlags)
bool FPheromoneGridEvaporationTest::RunTest(const FString& Parameters)
{
	FPheromoneGrid OneStep = MakeGrid();
	FPheromoneGrid TwoSteps = MakeGrid();
	const FIntPoint Cell(2, 7);
	DepositInCell(OneStep, Cell, EPheromoneChannel::Food, 10.f);
	DepositInCell(TwoSteps, Cell, EPheromoneChannel::Food, 10.f);
	DepositInCell(OneStep, Cell, EPheromoneChannel::Home, 10.f);

	OneStep.Evaporate(EPheromoneChannel::Food, 0.5f, 2.f);
	TestEqual(TEXT("Value after evaporation is 10 * exp(-rate * t)"),
		OneStep.GetCellValue(Cell, EPheromoneChannel::Food), 10.f * FMath::Exp(-1.f), TestTolerance);

	TwoSteps.Evaporate(EPheromoneChannel::Food, 0.5f, 1.f);
	TwoSteps.Evaporate(EPheromoneChannel::Food, 0.5f, 1.f);
	TestEqual(TEXT("Two 1 s steps equal one 2 s step"),
		TwoSteps.GetCellValue(Cell, EPheromoneChannel::Food), OneStep.GetCellValue(Cell, EPheromoneChannel::Food), TestTolerance);

	TestEqual(TEXT("Other channels do not evaporate"), OneStep.GetCellValue(Cell, EPheromoneChannel::Home), 10.f);

	OneStep.Evaporate(EPheromoneChannel::Home, 0.f, 5.f);
	TestEqual(TEXT("A rate of zero keeps the value"), OneStep.GetCellValue(Cell, EPheromoneChannel::Home), 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridDiffusionTest, "Supercolony.Pheromones.Diffusion", TestFlags)
bool FPheromoneGridDiffusionTest::RunTest(const FString& Parameters)
{
	FPheromoneGrid Grid = MakeGrid();
	const FIntPoint Centre(5, 5);
	DepositInCell(Grid, Centre, EPheromoneChannel::Food, 10.f);

	// Alpha = 1.0 * 0.1 = 0.1: the centre gives 10% of its value, split equally between 4 neighbours.
	Grid.Diffuse(EPheromoneChannel::Food, 1.f, 0.1f);
	TestEqual(TEXT("Centre loses Alpha of its value"), Grid.GetCellValue(Centre, EPheromoneChannel::Food), 9.f, TestTolerance);
	TestEqual(TEXT("Each neighbour gets a quarter of it"), Grid.GetCellValue(FIntPoint(6, 5), EPheromoneChannel::Food), 0.25f, TestTolerance);
	TestEqual(TEXT("Diagonal cells get nothing in one step"), Grid.GetCellValue(FIntPoint(6, 6), EPheromoneChannel::Food), 0.f, TestTolerance);
	TestEqual(TEXT("One step conserves the total"), Grid.GetTotal(EPheromoneChannel::Food), 10.f, TestTolerance);

	for (int32 Step = 0; Step < 200; ++Step)
	{
		Grid.Diffuse(EPheromoneChannel::Food, 1.f, 0.1f);
	}
	TestEqual(TEXT("Many steps conserve the total"), Grid.GetTotal(EPheromoneChannel::Food), 10.f, 1e-3f);
	TestTrue(TEXT("Values spread to the far corner"), Grid.GetCellValue(FIntPoint(0, 0), EPheromoneChannel::Food) > 0.f);

	FPheromoneGrid CornerGrid = MakeGrid();
	DepositInCell(CornerGrid, FIntPoint(0, 0), EPheromoneChannel::Food, 10.f);
	CornerGrid.Diffuse(EPheromoneChannel::Food, 1.f, 0.1f);
	TestEqual(TEXT("Edges reflect, so a corner deposit is conserved"), CornerGrid.GetTotal(EPheromoneChannel::Food), 10.f, TestTolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridDiffusionStabilityTest, "Supercolony.Pheromones.DiffusionStability", TestFlags)
bool FPheromoneGridDiffusionStabilityTest::RunTest(const FString& Parameters)
{
	// A checkerboard is the pattern most likely to oscillate. Read and write buffers are separate,
	// so every cell is updated from the same previous step.
	FPheromoneGrid Grid = MakeGrid();
	for (int32 Y = 0; Y < Grid.GetHeight(); ++Y)
	{
		for (int32 X = 0; X < Grid.GetWidth(); ++X)
		{
			if ((X + Y) % 2 == 0)
			{
				DepositInCell(Grid, FIntPoint(X, Y), EPheromoneChannel::Food, 1.f);
			}
		}
	}

	// Rate * DeltaTime = 100 is far above the cap, so Alpha is clamped to MaxDiffusionAlpha (0.5).
	Grid.Diffuse(EPheromoneChannel::Food, 100.f, 1.f);

	// Interior cells see two 1s and two 0s around them, so they all land on 0.5. Edge cells reflect onto
	// themselves and settle elsewhere, so they are only checked for not flipping past the opposite colour.
	bool bInteriorHalf = true;
	bool bNoCellFlipped = true;
	for (int32 Y = 0; Y < Grid.GetHeight(); ++Y)
	{
		for (int32 X = 0; X < Grid.GetWidth(); ++X)
		{
			const float Value = Grid.GetCellValue(FIntPoint(X, Y), EPheromoneChannel::Food);
			const bool bWasFull = (X + Y) % 2 == 0;
			bNoCellFlipped &= bWasFull ? Value >= 0.5f - TestTolerance : Value <= 0.5f + TestTolerance;

			const bool bInterior = X > 0 && Y > 0 && X < Grid.GetWidth() - 1 && Y < Grid.GetHeight() - 1;
			if (bInterior)
			{
				bInteriorHalf &= FMath::IsNearlyEqual(Value, 0.5f, TestTolerance);
			}
		}
	}
	TestTrue(TEXT("Interior of a checkerboard smooths to 0.5"), bInteriorHalf);
	TestTrue(TEXT("No cell flips past the opposite colour"), bNoCellFlipped);
	TestEqual(TEXT("Total is conserved at the maximum rate"), Grid.GetTotal(EPheromoneChannel::Food), 50.f, TestTolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPheromoneGridGradientTest, "Supercolony.Pheromones.Gradient", TestFlags)
bool FPheromoneGridGradientTest::RunTest(const FString& Parameters)
{
	FPheromoneGrid Grid = MakeGrid();
	const FVector2D Probe = Grid.CellToWorld(FIntPoint(5, 5));

	TestTrue(TEXT("Flat field has no gradient"), Grid.GetGradient(Probe, EPheromoneChannel::Food).IsZero());

	DepositInCell(Grid, FIntPoint(6, 5), EPheromoneChannel::Food, 4.f);
	TestTrue(TEXT("Points towards a deposit on +X"), Grid.GetGradient(Probe, EPheromoneChannel::Food).Equals(FVector2D(1.f, 0.f), TestTolerance));

	DepositInCell(Grid, FIntPoint(5, 6), EPheromoneChannel::Food, 4.f);
	const FVector2D Diagonal = Grid.GetGradient(Probe, EPheromoneChannel::Food);
	TestTrue(TEXT("Equal deposits on +X and +Y point diagonally"), Diagonal.Equals(FVector2D(1.f, 1.f).GetSafeNormal(), TestTolerance));
	TestEqual(TEXT("Gradient is a unit vector"), static_cast<float>(Diagonal.Size()), 1.f, TestTolerance);

	TestTrue(TEXT("Other channels stay flat"), Grid.GetGradient(Probe, EPheromoneChannel::Home).IsZero());
	TestTrue(TEXT("Outside the grid returns zero"), Grid.GetGradient(FVector2D(-500.f, 0.f), EPheromoneChannel::Food).IsZero());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
