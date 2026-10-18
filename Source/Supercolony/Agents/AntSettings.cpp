#include "AntSettings.h"
#include "MassEntityConfigAsset.h"

UAntSettings::UAntSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Ants");

	AntEntityConfig = TSoftObjectPtr<UMassEntityConfigAsset>(FSoftObjectPath(TEXT("/Game/Agents/DA_Ant.DA_Ant")));
}
