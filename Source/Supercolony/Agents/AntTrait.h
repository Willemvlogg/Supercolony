#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "AntTrait.generated.h"

/** Makes an entity an ant: adds FTransformFragment and FAntFragment, which the ant processors run on. */
UCLASS(meta = (DisplayName = "Ant"))
class SUPERCOLONY_API UAntTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
