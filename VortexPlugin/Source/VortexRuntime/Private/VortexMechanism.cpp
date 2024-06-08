#include "VortexMechanism.h"
#include "Runtime/Core/Public/Misc/Paths.h"

UVortexMechanism::UVortexMechanism(const FObjectInitializer& ObjectInitializer)
    :Super(ObjectInitializer)
    , AutomatedMappingImport(false)
{
}

void UVortexMechanism::PostLoad()
{
    if (!filepath_DEPRECATED.IsEmpty())
    {
        MechanismFilepath.FilePath = filepath_DEPRECATED;
        FPaths::MakePathRelativeTo(MechanismFilepath.FilePath, *FPaths::ProjectContentDir());
        filepath_DEPRECATED.Empty();
    }
    Super::PostLoad();
}
#if WITH_EDITOR
void UVortexMechanism::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
    FName PropertyName = (e.MemberProperty != NULL) ? e.MemberProperty->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UVortexMechanism, MechanismFilepath))
    {
        FPaths::MakePathRelativeTo(MechanismFilepath.FilePath, *FPaths::ProjectContentDir());
    }
    Super::PostEditChangeProperty(e);
}
#endif