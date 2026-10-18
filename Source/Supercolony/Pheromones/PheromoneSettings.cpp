#include "PheromoneSettings.h"
#include "Materials/MaterialInterface.h"

UPheromoneSettings::UPheromoneSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Pheromones");

	DebugMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Debug/M_PheromoneDebug.M_PheromoneDebug")));
}

float UPheromoneSettings::GetEvaporationRate(EPheromoneChannel Channel) const
{
	switch (Channel)
	{
	case EPheromoneChannel::Food:  return FoodEvaporationRate;
	case EPheromoneChannel::Home:  return HomeEvaporationRate;
	case EPheromoneChannel::Alarm: return AlarmEvaporationRate;
	case EPheromoneChannel::Rally: return RallyEvaporationRate;
	case EPheromoneChannel::Avoid: return AvoidEvaporationRate;
	default:                       return 0.f;
	}
}
