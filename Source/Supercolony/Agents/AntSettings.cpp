#include "AntSettings.h"
#include "MassEntityConfigAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

UAntSettings::UAntSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Ants");

	AntEntityConfig = TSoftObjectPtr<UMassEntityConfigAsset>(FSoftObjectPath(TEXT("/Game/Agents/DA_Ant.DA_Ant")));
	DotMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	DotMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Agents/M_AntDot.M_AntDot")));
}
