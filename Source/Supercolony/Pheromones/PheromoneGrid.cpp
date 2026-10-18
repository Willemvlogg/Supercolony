#include "PheromoneGrid.h"

void FPheromoneGrid::Init(int32 InWidth, int32 InHeight, float InCellSize, const FVector2D& InOrigin, float InMaxCellValue)
{
	check(InWidth > 0 && InHeight > 0 && InCellSize > 0.f && InMaxCellValue > 0.f);

	Width = InWidth;
	Height = InHeight;
	CellSize = InCellSize;
	Origin = InOrigin;
	MaxCellValue = InMaxCellValue;

	const int32 NumCells = Width * Height;
	for (TArray<float>& Layer : Layers)
	{
		Layer.Init(0.f, NumCells);
	}
	DiffusionBuffer.Init(0.f, NumCells);
}

bool FPheromoneGrid::WorldToCell(const FVector2D& WorldLocation, FIntPoint& OutCell) const
{
	const FVector2D Local = (WorldLocation - Origin) / CellSize;
	OutCell = FIntPoint(FMath::FloorToInt32(Local.X), FMath::FloorToInt32(Local.Y));
	return IsValidCell(OutCell.X, OutCell.Y);
}

FVector2D FPheromoneGrid::CellToWorld(const FIntPoint& Cell) const
{
	return Origin + (FVector2D(Cell) + FVector2D(0.5f)) * CellSize;
}

void FPheromoneGrid::Deposit(const FVector2D& WorldLocation, EPheromoneChannel Channel, float Amount)
{
	FIntPoint Cell;
	if (!WorldToCell(WorldLocation, Cell))
	{
		return;
	}

	float& Value = Layers[static_cast<int32>(Channel)][ToIndex(Cell.X, Cell.Y)];
	Value = FMath::Clamp(Value + Amount, 0.f, MaxCellValue);
}

float FPheromoneGrid::Sample(const FVector2D& WorldLocation, EPheromoneChannel Channel) const
{
	FIntPoint Cell;
	return WorldToCell(WorldLocation, Cell) ? GetCellValue(Cell, Channel) : 0.f;
}

FVector2D FPheromoneGrid::GetGradient(const FVector2D& WorldLocation, EPheromoneChannel Channel) const
{
	FIntPoint Cell;
	if (!WorldToCell(WorldLocation, Cell))
	{
		return FVector2D::ZeroVector;
	}

	const TArray<float>& Values = Layers[static_cast<int32>(Channel)];
	const float DeltaX = GetClampedValue(Values, Cell.X + 1, Cell.Y) - GetClampedValue(Values, Cell.X - 1, Cell.Y);
	const float DeltaY = GetClampedValue(Values, Cell.X, Cell.Y + 1) - GetClampedValue(Values, Cell.X, Cell.Y - 1);
	return FVector2D(DeltaX, DeltaY).GetSafeNormal();
}

void FPheromoneGrid::Evaporate(EPheromoneChannel Channel, float Rate, float DeltaTime)
{
	const float Factor = FMath::Exp(-Rate * DeltaTime);
	for (float& Value : Layers[static_cast<int32>(Channel)])
	{
		Value *= Factor;
	}
}

void FPheromoneGrid::Diffuse(EPheromoneChannel Channel, float Rate, float DeltaTime)
{
	const float Alpha = FMath::Clamp(Rate * DeltaTime, 0.f, MaxDiffusionAlpha);
	if (Alpha <= 0.f)
	{
		return;
	}

	TArray<float>& Values = Layers[static_cast<int32>(Channel)];
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const float Current = Values[ToIndex(X, Y)];
			const float NeighbourAverage = 0.25f * (
				GetClampedValue(Values, X + 1, Y) +
				GetClampedValue(Values, X - 1, Y) +
				GetClampedValue(Values, X, Y + 1) +
				GetClampedValue(Values, X, Y - 1));
			DiffusionBuffer[ToIndex(X, Y)] = Current + Alpha * (NeighbourAverage - Current);
		}
	}
	Swap(Values, DiffusionBuffer);
}

float FPheromoneGrid::GetCellValue(const FIntPoint& Cell, EPheromoneChannel Channel) const
{
	return IsValidCell(Cell.X, Cell.Y) ? Layers[static_cast<int32>(Channel)][ToIndex(Cell.X, Cell.Y)] : 0.f;
}

float FPheromoneGrid::GetTotal(EPheromoneChannel Channel) const
{
	float Total = 0.f;
	for (const float Value : Layers[static_cast<int32>(Channel)])
	{
		Total += Value;
	}
	return Total;
}

float FPheromoneGrid::GetClampedValue(const TArray<float>& Values, int32 X, int32 Y) const
{
	return Values[ToIndex(FMath::Clamp(X, 0, Width - 1), FMath::Clamp(Y, 0, Height - 1))];
}
