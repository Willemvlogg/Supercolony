#include "AntTrait.h"
#include "AntFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/EntityFragments.h"

void UAntTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment<FAntFragment>();
}
