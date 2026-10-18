#include "RollingEventCounter.h"

void FRollingEventCounter::Add(double Time, int32 Count)
{
	// Several deliveries in the same frame share one entry.
	if (Entries.Num() > 0 && Entries.Last().Time == Time)
	{
		Entries.Last().Count += Count;
	}
	else
	{
		Entries.Add({ Time, Count });
	}
	RunningTotal += Count;
}

int32 FRollingEventCounter::GetCount(double Now)
{
	Prune(Now);
	return RunningTotal;
}

void FRollingEventCounter::Reset()
{
	Entries.Reset();
	RunningTotal = 0;
}

void FRollingEventCounter::Prune(double Now)
{
	const double Cutoff = Now - WindowSeconds;
	int32 NumExpired = 0;
	while (NumExpired < Entries.Num() && Entries[NumExpired].Time <= Cutoff)
	{
		RunningTotal -= Entries[NumExpired].Count;
		++NumExpired;
	}
	if (NumExpired > 0)
	{
		Entries.RemoveAt(0, NumExpired, EAllowShrinking::No);
	}
}
