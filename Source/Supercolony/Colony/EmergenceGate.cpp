#include "EmergenceGate.h"

void FEmergenceGate::Reset(int32 InTotal)
{
	Total = FMath::Max(InTotal, 0);
	Released = 0;
	Pending = 0.f;
}

void FEmergenceGate::Advance(float Rate, float DeltaTime)
{
	if (Released >= Total)
	{
		return;
	}
	if (Rate <= 0.f)
	{
		Released = Total;
		return;
	}

	Pending += Rate * FMath::Max(DeltaTime, 0.f);
	const int32 Whole = FMath::FloorToInt32(Pending);
	Pending -= Whole;
	Released = FMath::Min(Total, Released + Whole);
}
