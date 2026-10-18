#include "SwarmGameMode.h"
#include "UI/ColonyDebugHUD.h"

ASwarmGameMode::ASwarmGameMode()
{
	HUDClass = AColonyDebugHUD::StaticClass();
}
