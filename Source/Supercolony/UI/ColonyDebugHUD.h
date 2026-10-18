#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ColonyDebugHUD.generated.h"

/** M1 stats as canvas text in the top-left corner: fps, ant count and food delivered. */
UCLASS()
class SUPERCOLONY_API AColonyDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	/** Frame time smoothed over roughly the last 20 frames, so the fps readout is stable enough to read. */
	float SmoothedDeltaTime = 0.f;
};
