#include "SwarmGameMode.h"
#include "HAL/IConsoleManager.h"
#include "UI/ColonyDebugHUD.h"

namespace
{
	IConsoleVariable* FindAntiAliasingCVar()
	{
		return IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
	}
}

ASwarmGameMode::ASwarmGameMode()
{
	HUDClass = AColonyDebugHUD::StaticClass();
}

void ASwarmGameMode::StartPlay()
{
	if (IConsoleVariable* CVar = FindAntiAliasingCVar())
	{
		PreviousAntiAliasingMethod = CVar->GetInt();
		CVar->Set(AntiAliasingMethod, ECVF_SetByCode);
	}

	Super::StartPlay();
}

void ASwarmGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	IConsoleVariable* CVar = FindAntiAliasingCVar();
	if (CVar && PreviousAntiAliasingMethod != INDEX_NONE)
	{
		CVar->Set(PreviousAntiAliasingMethod, ECVF_SetByCode);
	}

	Super::EndPlay(EndPlayReason);
}
