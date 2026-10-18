#pragma once

#include "CoreMinimal.h"

/**
 * Counts events inside a sliding time window, e.g. food delivered in the last 60 seconds.
 * Events must be added in time order. Plain C++ so it can be unit tested directly.
 */
struct SUPERCOLONY_API FRollingEventCounter
{
	explicit FRollingEventCounter(double InWindowSeconds = 60.0) : WindowSeconds(InWindowSeconds) {}

	void Add(double Time, int32 Count = 1);

	/** Events with a time in (Now - Window, Now]. Drops anything older. */
	int32 GetCount(double Now);

	void Reset();

	double GetWindowSeconds() const { return WindowSeconds; }

private:
	struct FEntry
	{
		double Time;
		int32 Count;
	};

	void Prune(double Now);

	double WindowSeconds;
	TArray<FEntry> Entries;
	int32 RunningTotal = 0;
};
