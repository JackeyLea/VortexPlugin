#include "VortexSettings.h"
#include "VortexRuntime.h"
#include "Runtime/Core/Public/Misc/Paths.h"

#if WITH_EDITOR
#include "Internationalization/Internationalization.h"
#include "Misc/MessageDialog.h"
#endif

#define LOCTEXT_NAMESPACE "UVortexSettings"

FMaterialMapping::FMaterialMapping()
    : PhysicalMaterial(nullptr)
    , VortexMaterialName("")
{
}

UVortexSettings::UVortexSettings()
    : UseDefaultApplicationSetup(true)
    , UseDefaultMaterialTable(true)
    , EnableLandscapeCollisionDetection(true)
    , EnableMeshSimpleCollisionDetection(true)
    , EnableMeshComplexCollisionDetection(true)
    , TerrainPagingTileSizeXY(50.0)
    , TerrainPagingLookAheadTime(1.0)
    , TerrainPagingSafetyBandSize(1.0)
    , IsMaterialMappingErrorBeingShown(false)
{
}

void UVortexSettings::PostInitProperties()
{
    if (!ApplicationSetup.FilePath.IsEmpty())
    {
        UseDefaultApplicationSetup = false;
    }

    if (!MaterialTableFilepath.FilePath.IsEmpty())
    {
        UseDefaultMaterialTable = false;
    }

    Super::PostInitProperties();
}

#if WITH_EDITOR
void UVortexSettings::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
    bool isFileUnderProjectContentDir = true;
    const FString absoluteProjectContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

    FName PropertyName = (e.MemberProperty != nullptr) ? e.MemberProperty->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UVortexSettings, UseDefaultApplicationSetup))
    {
        if (UseDefaultApplicationSetup)
        {
            ApplicationSetup.FilePath.Empty();
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UVortexSettings, UseDefaultMaterialTable))
    {
        if (UseDefaultMaterialTable)
        {
            MaterialTableFilepath.FilePath.Empty();
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UVortexSettings, ApplicationSetup))
    {
        if (!ApplicationSetup.FilePath.IsEmpty())
        {
            // Check if the path is relative to the project content directory
            FString absolutePickedPath = FPaths::ConvertRelativePathToFull(ApplicationSetup.FilePath);
            if (absolutePickedPath.StartsWith(absoluteProjectContentDir))
            {
                FPaths::MakePathRelativeTo(ApplicationSetup.FilePath, *absoluteProjectContentDir);
            }
            else
            {
                ApplicationSetup.FilePath.Empty();
                isFileUnderProjectContentDir = false;
            }
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UVortexSettings, MaterialTableFilepath))
    {
        if (!MaterialTableFilepath.FilePath.IsEmpty())
        {
            // Check if the path is relative to the project content directory
            FString absolutePickedPath = FPaths::ConvertRelativePathToFull(MaterialTableFilepath.FilePath);
            if (absolutePickedPath.StartsWith(absoluteProjectContentDir))
            {
                FPaths::MakePathRelativeTo(MaterialTableFilepath.FilePath, *absoluteProjectContentDir);
            }
            else
            {
                MaterialTableFilepath.FilePath.Empty();
                isFileUnderProjectContentDir = false;
            }
        }
    }

    if (!isFileUnderProjectContentDir)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("Error_InvalidRootPath", "The chosen file must be within {0}."), FText::FromString(absoluteProjectContentDir)));
    }

    Super::PostEditChangeProperty(e);
}


void UVortexSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
    FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(FMaterialMapping, VortexMaterialName))
    {
        // This flag prevents us from showing 2 error popups in a row when the user presses the "Return" key.
        // This triggers 2 consecutive calls to this function.
        if (!IsMaterialMappingErrorBeingShown)
        {
            if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
            {
                const int32 ValueSetIndex = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_CHECKED(UVortexSettings, MaterialMappings).ToString());
                if (ValueSetIndex != INDEX_NONE && ValueSetIndex < MaterialMappings.Num())
                {
                    FMaterialMapping& ModifiedMaterialMapping = MaterialMappings[ValueSetIndex];
                    auto& AvailableVortexMaterials = FVortexRuntimeModule::Get().GetAvailableVortexMaterials();
                    if (!ModifiedMaterialMapping.VortexMaterialName.IsEmpty() && AvailableVortexMaterials.Find(ModifiedMaterialMapping.VortexMaterialName) == nullptr)
                    {
                        FString FormattedListOfMaterials = "";
                        for (auto& MaterialName : AvailableVortexMaterials)
                        {
                            FormattedListOfMaterials += "- ";
                            FormattedListOfMaterials += MaterialName;
                            FormattedListOfMaterials += "\n";
                        }

                        FString VortexMaterialName = ModifiedMaterialMapping.VortexMaterialName;
                        ModifiedMaterialMapping.VortexMaterialName = "";

                        IsMaterialMappingErrorBeingShown = true;
                        FMessageDialog::Open(EAppMsgType::Ok,
                            FText::Format(LOCTEXT("Error_InvalidMaterialName", "Material \"{0}\" does not exist in the currently loaded material table. Please select a material from the following list:\n\n{1}"),
                                FText::FromString(VortexMaterialName), FText::FromString(FormattedListOfMaterials)));
                        IsMaterialMappingErrorBeingShown = false;

                        ModifiedMaterialMapping.VortexMaterialName = "";
                    }
                }
            }
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(FMaterialMapping, PhysicalMaterial))
    {
        if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
        {
            const int32 ValueSetIndex = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_CHECKED(UVortexSettings, MaterialMappings).ToString());
            if (ValueSetIndex != INDEX_NONE && ValueSetIndex < MaterialMappings.Num())
            {
                FMaterialMapping& ModifiedMaterialMapping = MaterialMappings[ValueSetIndex];
                if (ModifiedMaterialMapping.PhysicalMaterial != nullptr)
                {
                    for (int32 Index = 0; Index < MaterialMappings.Num(); ++Index)
                    {
                        if (Index != ValueSetIndex)
                        {
                            if (MaterialMappings[Index].PhysicalMaterial == ModifiedMaterialMapping.PhysicalMaterial)
                            {
                                FMessageDialog::Open(EAppMsgType::Ok,
                                    FText::Format(LOCTEXT("Error_DuplicatedPhysicalMaterial", "Physical Material \"{0}\" is already mapped to a Vortex Material. Please select another Physical Material."),
                                                  FText::FromString(MaterialMappings[Index].PhysicalMaterial->GetName())));
                                ModifiedMaterialMapping.PhysicalMaterial = nullptr;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif
