#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "AntProcessors.generated.h"

/*
 * Ant behaviour, one step per frame, in this order:
 *   Steer   - pick a new heading: straight at a goal in sight, otherwise sensors plus wander.
 *   Move    - walk forward, stay in bounds, pick up or drop food on arrival.
 *   Deposit - lay pheromone, fading with time since the last target.
 *   Render  - copy positions and state colours into the instanced dots.
 */

UCLASS()
class SUPERCOLONY_API UAntSteerProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UAntSteerProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/** Runs on the game thread because it adds to the colony's food counter. */
UCLASS()
class SUPERCOLONY_API UAntMoveProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UAntMoveProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/** Runs on the game thread because it writes to the pheromone grid. */
UCLASS()
class SUPERCOLONY_API UAntDepositProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UAntDepositProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/** Runs on the game thread because it updates the instanced mesh component on AAntRenderActor. */
UCLASS()
class SUPERCOLONY_API UAntRenderProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UAntRenderProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;

	/** Kept between frames to avoid reallocating. */
	TArray<FTransform> Transforms;
	TArray<float> CustomData;
};
