#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SwarmGameMode.generated.h"

/**
 * Game mode for the M1 swarm test map: a free camera, the colony stats HUD, and cheap anti-aliasing.
 * Lumen and shadows are turned off in the map itself (post process volume and light); anti-aliasing is a
 * renderer setting rather than a post process one, so this game mode applies it while the map plays.
 */
UCLASS(Config = Game)
class SUPERCOLONY_API ASwarmGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASwarmGameMode();

	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** Value for r.AntiAliasingMethod while playing: 0 none, 1 FXAA, 2 TAA, 3 MSAA (forward only), 4 TSR. Restored afterwards. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Rendering", meta = (ClampMin = "0", ClampMax = "4"))
	int32 AntiAliasingMethod = 1;

private:
	/** r.AntiAliasingMethod before StartPlay, so the editor gets its own setting back. */
	int32 PreviousAntiAliasingMethod = INDEX_NONE;
};
