#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "PheromoneGrid.generated.h"

UENUM(BlueprintType)
enum class EPheromoneChannel : uint8
{
	Food,
	Home,
	Alarm,
	Rally,
	Avoid,

	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EPheromoneChannel, EPheromoneChannel::Count);

/**
 * A 2D grid of pheromone values, one layer per channel, laid flat over the XY plane.
 * Plain C++ with no world or UObject dependencies so it can be unit tested directly.
 * Cell (0, 0) starts at Origin; cells grow along +X and +Y.
 */
struct SUPERCOLONY_API FPheromoneGrid
{
	static constexpr int32 NumChannels = static_cast<int32>(EPheromoneChannel::Count);

	/** Diffusion fraction per step is capped here; above 0.5 a checkerboard pattern would flip instead of smoothing. */
	static constexpr float MaxDiffusionAlpha = 0.5f;

	void Init(int32 InWidth, int32 InHeight, float InCellSize, const FVector2D& InOrigin, float InMaxCellValue);

	/** Returns false if the location is outside the grid. */
	bool WorldToCell(const FVector2D& WorldLocation, FIntPoint& OutCell) const;
	FVector2D CellToWorld(const FIntPoint& Cell) const;

	/** Adds Amount to the cell under WorldLocation, clamped to [0, MaxCellValue]. Ignored outside the grid. */
	void Deposit(const FVector2D& WorldLocation, EPheromoneChannel Channel, float Amount);

	/** Value of the cell under WorldLocation, or 0 outside the grid. */
	float Sample(const FVector2D& WorldLocation, EPheromoneChannel Channel) const;

	/** Unit direction of the strongest increase around WorldLocation, or zero if the area is flat or outside the grid. */
	FVector2D GetGradient(const FVector2D& WorldLocation, EPheromoneChannel Channel) const;

	/** Scales every cell by exp(-Rate * DeltaTime), so the result does not depend on how the time is split into steps. */
	void Evaporate(EPheromoneChannel Channel, float Rate, float DeltaTime);

	/**
	 * Moves each cell towards the average of its 4 neighbours by Rate * DeltaTime (capped at MaxDiffusionAlpha).
	 * Reads the previous values and writes a separate buffer, then swaps. Edges reflect, so the total is conserved.
	 */
	void Diffuse(EPheromoneChannel Channel, float Rate, float DeltaTime);

	float GetCellValue(const FIntPoint& Cell, EPheromoneChannel Channel) const;
	float GetTotal(EPheromoneChannel Channel) const;

	int32 GetWidth() const { return Width; }
	int32 GetHeight() const { return Height; }
	float GetCellSize() const { return CellSize; }
	FVector2D GetOrigin() const { return Origin; }
	float GetMaxCellValue() const { return MaxCellValue; }
	FVector2D GetWorldSize() const { return FVector2D(Width, Height) * CellSize; }

private:
	bool IsValidCell(int32 X, int32 Y) const { return X >= 0 && X < Width && Y >= 0 && Y < Height; }
	int32 ToIndex(int32 X, int32 Y) const { return Y * Width + X; }

	/** Value at (X, Y) with coordinates clamped to the grid, so edge cells see themselves as their missing neighbour. */
	float GetClampedValue(const TArray<float>& Values, int32 X, int32 Y) const;

	int32 Width = 0;
	int32 Height = 0;
	float CellSize = 1.f;
	FVector2D Origin = FVector2D::ZeroVector;
	float MaxCellValue = 1.f;

	TArray<float> Layers[NumChannels];

	/** Write buffer for Diffuse, swapped with the layer being diffused. */
	TArray<float> DiffusionBuffer;
};
