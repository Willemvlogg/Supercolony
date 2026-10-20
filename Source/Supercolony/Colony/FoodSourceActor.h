#pragma once

#include "CoreMinimal.h"
#include "ColonyMarkerActor.h"
#include "FoodSourceActor.generated.h"

/**
 * Placeholder food source: a green sphere. Registers itself with UColonySubsystem on BeginPlay, so sources
 * placed or spawned while the game runs are found by the ants too.
 */
UCLASS()
class SUPERCOLONY_API AFoodSourceActor : public AColonyMarkerActor
{
	GENERATED_BODY()

public:
	AFoodSourceActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
