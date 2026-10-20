#include "ColonyDebugHUD.h"
#include "Colony/ColonySubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Misc/App.h"

namespace
{
	constexpr float SmoothingFactor = 0.05f;
	constexpr float TextLeft = 20.f;
	constexpr float TextTop = 20.f;
	constexpr float LineHeight = 20.f;
}

void AColonyDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	const float DeltaTime = static_cast<float>(FApp::GetDeltaTime());
	SmoothedDeltaTime = SmoothedDeltaTime > 0.f ? FMath::Lerp(SmoothedDeltaTime, DeltaTime, SmoothingFactor) : DeltaTime;

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("FPS: %.0f (%.1f ms)"), SmoothedDeltaTime > 0.f ? 1.f / SmoothedDeltaTime : 0.f, SmoothedDeltaTime * 1000.f));

	if (UColonySubsystem* Colony = GetWorld()->GetSubsystem<UColonySubsystem>())
	{
		Lines.Add(FString::Printf(TEXT("Ants: %d out / %d"), Colony->GetAntsOut(), Colony->GetAntCount()));
		Lines.Add(FString::Printf(TEXT("Food delivered: %d"), Colony->GetFoodDelivered()));

		const float Elapsed = Colony->GetSecondsSinceSpawn();
		Lines.Add(Elapsed < 60.f
			? FString::Printf(TEXT("Food / min: %d (first %.0f s)"), Colony->GetFoodLastMinute(), Elapsed)
			: FString::Printf(TEXT("Food / min: %d"), Colony->GetFoodLastMinute()));

		// One line per food source, so you can see when a trail to a new source has formed.
		const double Now = GetWorld()->GetTimeSeconds();
		for (FFoodSourceStats& Stats : Colony->GetFoodSourceStats())
		{
			const FString FirstPickup = Stats.FirstPickupTime >= 0.0
				? FString::Printf(TEXT("first pickup %.0f s after it appeared"), Stats.FirstPickupTime - Stats.AppearedTime)
				: FString::Printf(TEXT("no pickups yet (%.0f s)"), Now - Stats.AppearedTime);
			const FString Trail = Stats.TrailTime >= 0.0
				? FString::Printf(TEXT(", trail after %.0f s"), Stats.TrailTime - Stats.AppearedTime)
				: FString();
			Lines.Add(FString::Printf(TEXT("  %s: %d / min, %s%s"), *Stats.Label, Stats.PickupCounter.GetCount(Now), *FirstPickup, *Trail));
		}
	}

	UFont* Font = GEngine->GetMediumFont();
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		DrawText(Lines[Index], FLinearColor::White, TextLeft, TextTop + Index * LineHeight, Font);
	}
}
