#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SwarmGameMode.generated.h"

/** Game mode for the M1 swarm test map: a free camera and the colony stats HUD. */
UCLASS()
class SUPERCOLONY_API ASwarmGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASwarmGameMode();
};
