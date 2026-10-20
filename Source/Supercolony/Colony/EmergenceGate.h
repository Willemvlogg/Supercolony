#pragma once

#include "CoreMinimal.h"

/**
 * Lets ants out of the nest a few at a time. Ants are numbered 0..Total-1 when spawned; ant N is out once
 * GetReleased() > N. Advancing by Rate * DeltaTime each frame means a rate change applies from the next frame on.
 * Plain C++ so it can be unit tested directly.
 */
struct SUPERCOLONY_API FEmergenceGate
{
	void Reset(int32 InTotal);

	/** Releases Rate ants per second. A rate of 0 or less releases everyone at once. */
	void Advance(float Rate, float DeltaTime);

	int32 GetReleased() const { return Released; }
	int32 GetTotal() const { return Total; }
	bool IsReleased(int32 EmergeIndex) const { return EmergeIndex < Released; }

private:
	int32 Total = 0;
	int32 Released = 0;

	/** Fractional ants carried over between frames, so low rates at high frame rates still release. */
	float Pending = 0.f;
};
