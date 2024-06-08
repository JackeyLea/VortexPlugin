#include "MechanismComponent.h"
#include "VortexIntegrationUtilities.h"
#include "VortexRuntime.h"
#include <Engine/Classes/GameFramework/Actor.h>
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

#include <string>
#include <vector>

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_TickComponent, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValue"), STAT_GetVHLValue, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueTransform"), STAT_GetVHLValueTransform, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueIMobile"), STAT_GetVHLValueIMobile, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueHeightfield"), STAT_GetVHLValueHeightfield, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueTiledHeightField"), STAT_GetVHLValueTiledHeightField, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueParticle"), STAT_GetVHLValueParticle, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueGraphicsMesh"), STAT_GetVHLValueGraphicsMesh, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("GetVHLValueMaterial"), STAT_GetVHLValueMaterial, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("SetVHLValue"), STAT_SetVHLValue, STATGROUP_VortexMechanism);
DECLARE_CYCLE_STAT(TEXT("SetVHLValueTransform"), STAT_SetVHLValueTransform, STATGROUP_VortexMechanism);

// Sets default values for this component's properties
UMechanismComponent::UMechanismComponent()
    : UActorComponent()
    , VortexObject(nullptr)
{
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bCanEverTick = true;

    bTickInEditor = true;
}

void UMechanismComponent::OnUnregister()
{
    if (HasBegunPlay())
    {
        if (VortexMechanism != nullptr)
        {
            if (LoadedMechanismKey.IsEmpty())
            {
                UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::OnUnregister(): LoadedMechanismKey should not be empty <%s>."), *VortexMechanism->MechanismFilepath.FilePath);
            }

            if (VortexObject == nullptr)
            {
                UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::OnUnregister(): The VortexObject should not be NULL <%s>."), *VortexMechanism->MechanismFilepath.FilePath);
            }
        }

        FVortexRuntimeModule::Get().UnregisterAllComponents(LoadedMechanismKey);
    }
    else
    {
        FVortexRuntimeModule::Get().UnregisterComponent(this);
    }

    Super::OnUnregister();
}

void UMechanismComponent::OnRegister()
{
    Super::OnRegister();

    if (!VortexMechanism || VortexMechanism->MechanismFilepath.FilePath.IsEmpty())
        return;

    if (GetWorld() == nullptr)
        return;

    if (GetWorld()->WorldType.GetValue() != EWorldType::Type::Editor)
        return;

    // Changing any UProperty causes an ReregisterComponent() via Unreal callbacks.
    // We rely on that to recompute the LoadedMechanismKey when the user picks a VortexMechanism UAsset
    FVortexRuntimeModule::Get().RegisterComponent(this);

    if (VortexMechanism && VortexMechanism->AutomatedMappingImport)
    {
        // get node count
        std::uint32_t MaterialNodeCount = 0;
        VortexGetGraphicsNodeHandles(VortexObject, nullptr, &MaterialNodeCount);
        // get node handles
        std::vector<VortexObjectHandle> GraphicsNodeArray(MaterialNodeCount, nullptr);
        VortexGetGraphicsNodeHandles(VortexObject, GraphicsNodeArray.data(), &MaterialNodeCount);

        for (VortexObjectHandle node : GraphicsNodeArray)
        {
            VortexGraphicNodeData nodeData = {};
            VortexGetGraphicNodeData(node, &nodeData);

            if (nodeData.hasConnection)
            {
                GraphicNodeObjectHandles.Add(node);

                //Don't use directly name from nodeData as it might have been renamed in Vortex, use the lookup table first using the ContentID
                //If the mapping is not found (happens with legacy MechanismComponent), just use noData.name directly
                FName nameToUse(nodeData.name);
                for (const FMechanismGraphicNodeMapping& mapping : VortexMechanism->GraphicNodeMappings)
                {
                    //if we have an existing mapping for this node, use the name from the mapping
                    uint64 mappingContentID[2] = { ((uint64)mapping.ContentID.id00 & 0xFFFFFFFF) | ((uint64)mapping.ContentID.id01) << 32,
                                                   ((uint64)mapping.ContentID.id10 & 0xFFFFFFFF) | ((uint64)mapping.ContentID.id11) << 32 };
                    if (mappingContentID[0] == nodeData.contentID[0] && mappingContentID[1] == nodeData.contentID[1])
                    {
                        //update the name in case it has changed
                        nameToUse = mapping.GraphicsNodeName;
                        break;
                    }
                }

                if (auto component = this->GetOwner()->GetDefaultSubobjectByName(nameToUse))
                {
                    GraphicNodeSceneComponentsTwins.Add(Cast<USceneComponent>(component));
                }
            }
        }
    }
}

void UMechanismComponent::PostRename(UObject* OldOuter, const FName OldName)
{
    Super::PostRename(OldOuter, OldName);

    if (IsRegistered())
    {
        // Renaming the component forces us to recompute the LoadedMechanismKey
        ReregisterComponent();
    }
}

void UMechanismComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
    SCOPE_CYCLE_COUNTER(STAT_TickComponent);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (AActor * Actor = GetOwner())
    {
        if (UWorld * World = GetWorld())
        {
            // Editor Components Only
            if (!HasBegunPlay() && World->WorldType.GetValue() == EWorldType::Editor)
            {
                if (VortexObject != nullptr)
                {
                    double translation[3];
                    double rotation[4];
                    VortexIntegrationUtilities::ConvertTransform(Actor->GetTransform(), translation, rotation);
                    VortexSetWorldTransform(VortexObject, translation, rotation);
                }
            }

            // Runtime & Editor Components
            if (HasBegunPlay() || World->WorldType.GetValue() == EWorldType::Editor)
            {
                if (VortexObject != nullptr)
                {
#if WITH_EDITOR
                    if (HasBegunPlay() && VortexGetApplicationMode() != kVortexModeSimulating)
                    {
                        if (GEngine)
                        {
                            //221518200524 is vortex as numbers. The first parameter of UEngine::AddOnScreenDebugMessage is a unique number used to identify the message in the screen.
                            //It prevents the same message from being printed multiple times.
                            GEngine->AddOnScreenDebugMessage(uint64(221518200524), 0.5, FColor::Red, FString("Vortex simulation is not started and a vortex mechanism is present in the level. Call 'Start Simulation' in a level blueprint to start the Vortex simulation."));
                        }
                    }
#endif
                    for (int i = 0; i < GraphicNodeSceneComponentsTwins.Num(); ++i)
                    {
                        double translation[3] = {};
                        double scale[3] = {};
                        double rotation[4] = {};
                        VortexGetParentTransform(GraphicNodeObjectHandles[i], translation, scale, rotation);

                        GraphicNodeSceneComponentsTwins[i]->SetWorldScale3D(FVector(scale[0], scale[1], scale[2]));
                        GraphicNodeSceneComponentsTwins[i]->SetWorldLocation(VortexIntegrationUtilities::ConvertTranslation(translation));
                        GraphicNodeSceneComponentsTwins[i]->SetWorldRotation(VortexIntegrationUtilities::ConvertRotation(rotation));
                    }
                    
                    for (FMechanismComponentMappingSection& section : ComponentMappings)
                    {
                        for (FMechanismComponentMapping& mapping : section.Mappings)
                        {
                            if (USceneComponent * Component = Cast<USceneComponent>(mapping.Component.GetComponent(Actor)))
                            {
                                if (Component != Actor->GetRootComponent())
                                {
                                    double Translation[3];
                                    double Rotation[4];
                                    if (VortexGetOutputMatrix(VortexObject, TCHAR_TO_UTF8(*section.VHLName), TCHAR_TO_UTF8(*mapping.TransformFieldName), Translation, Rotation))
                                    {
                                        FTransform transform = VortexIntegrationUtilities::ConvertTransform(Translation, Rotation);
                                        Component->SetWorldTransform(transform);
                                    }
                                    else
                                    {
                                        LogErrorForVHLFunction("TickComponent", VortexMechanism->MechanismFilepath.FilePath, VortexObject, section.VHLName, mapping.TransformFieldName, kVortexFieldTypeOutput, kVortexDataTypeMatrix);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void UMechanismComponent::BeginPlay()
{
    Super::BeginPlay();

    FVortexRuntimeModule::Get().RegisterComponent(this);
    FVortexRuntimeModule::Get().BeginPlay(this);

    if (VortexMechanism != nullptr)
    {
        if (LoadedMechanismKey.IsEmpty())
        {
            UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::BeginPlay(): LoadedMechanismKey should not be empty <%s>."), *VortexMechanism->MechanismFilepath.FilePath);
        }

        if (VortexObject == nullptr)
        {
            UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::BeginPlay(): The VortexObject should not be NULL <%s>."), *VortexMechanism->MechanismFilepath.FilePath);
        }

        if (VortexMechanism && VortexMechanism->AutomatedMappingImport)
        {
            // get node count
            std::uint32_t MaterialNodeCount = 0;
            VortexGetGraphicsNodeHandles(VortexObject, nullptr, &MaterialNodeCount);
            // get node handles
            std::vector<VortexObjectHandle> GraphicsNodeArray(MaterialNodeCount, nullptr);
            VortexGetGraphicsNodeHandles(VortexObject, GraphicsNodeArray.data(), &MaterialNodeCount);

            for (VortexObjectHandle node : GraphicsNodeArray)
            {
                VortexGraphicNodeData nodeData = {};
                VortexGetGraphicNodeData(node, &nodeData);

                if (nodeData.hasConnection)
                {
                    GraphicNodeObjectHandles.Add(node);

                    //Don't use directly name from nodeData as it might have been renamed in Vortex, use the lookup table first using the ContentID
                    //If the mapping is not found (happens with legacy MechanismComponent), just use noData.name directly
                    FName nameToUse(nodeData.name);
                    for (const FMechanismGraphicNodeMapping& mapping : VortexMechanism->GraphicNodeMappings)
                    {
                        //if we have an existing mapping for this node, use the name from the mapping
                        uint64 mappingContentID[2] = { ((uint64)mapping.ContentID.id00 & 0xFFFFFFFF) | ((uint64)mapping.ContentID.id01) << 32,
                                                       ((uint64)mapping.ContentID.id10 & 0xFFFFFFFF) | ((uint64)mapping.ContentID.id11) << 32 };
                        if (mappingContentID[0] == nodeData.contentID[0] && mappingContentID[1] == nodeData.contentID[1])
                        {
                            //update the name in case it has changed
                            nameToUse = mapping.GraphicsNodeName;
                            break;
                        }
                    }

                    if (auto component = this->GetOwner()->GetDefaultSubobjectByName(nameToUse))
                    {
                        GraphicNodeSceneComponentsTwins.Add(Cast<USceneComponent>(component));
                    }
                }
            }
        }
    }
}

void UMechanismComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsRegistered())
    {
        FVortexRuntimeModule::Get().EndPlay(this);
        if (VortexMechanism != nullptr)
        {
            if (LoadedMechanismKey.IsEmpty())
            {
                UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::EndPlay(): LoadedMechanismKey should not be empty <%s>."), *VortexMechanism->MechanismFilepath.FilePath);
            }

            if (VortexObject == nullptr)
            {
                UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::EndPlay(): The VortexObject should not be NULL <%s>."), *VortexMechanism->MechanismFilepath.FilePath);
            }
        }

        FVortexRuntimeModule::Get().UnregisterAllComponents(LoadedMechanismKey);
    }

    Super::EndPlay(EndPlayReason);
}

void UMechanismComponent::onComponentUnregistered()
{
    GraphicNodeObjectHandles.Empty();
    GraphicNodeSceneComponentsTwins.Empty();
}

void UMechanismComponent::GetVHLFieldAsBool(FString VHLName, FString FieldName, bool& bValue)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsBool", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    if (!VortexGetOutputBoolean(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &bValue))
    {
        LogErrorForVHLFunction("GetVHLFieldAsBool", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeBoolean);
    }
}

void UMechanismComponent::SetVHLFieldAsBool(FString VHLName, FString FieldName, bool bValue)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsBool", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }
    
    if (!VortexSetInputBoolean(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), bValue))
    {
        LogErrorForVHLFunction("SetVHLFieldAsBool", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeBoolean);
    }
}

void UMechanismComponent::GetVHLFieldAsInteger(FString VHLName, FString FieldName, int32& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsInteger", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }
    
    if (!VortexGetOutputInt(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &Value))
    {
        LogErrorForVHLFunction("GetVHLFieldAsInteger", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeInt);
    }
}

void UMechanismComponent::SetVHLFieldAsInteger(FString VHLName, FString FieldName, int32 Value)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsInteger", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }
    
    if (!VortexSetInputInt(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), Value))
    {
        LogErrorForVHLFunction("SetVHLFieldAsInteger", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeInt);
    }
}

void UMechanismComponent::GetVHLFieldAsFloat(FString VHLName, FString FieldName, float& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsFloat", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }
    
    double VxValue = 0.0;
    if (VortexGetOutputReal(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &VxValue))
    {
        Value = VxValue;
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsFloat", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeReal);
    }
}

void UMechanismComponent::SetVHLFieldAsFloat(FString VHLName, FString FieldName, float Value)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsFloat", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double VxValue = Value;
    if (!VortexSetInputReal(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        LogErrorForVHLFunction("SetVHLFieldAsFloat", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeReal);
    }
}

void UMechanismComponent::GetVHLFieldAsVector2(FString VHLName, FString FieldName, FVector2D& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsVector2", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double VxValue[2] = { 0.0, 0.0 };
    if (VortexGetOutputVector2(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        Value.X = VxValue[0];
        Value.Y = VxValue[1];
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsVector2", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeVector2);
    }
}

void UMechanismComponent::SetVHLFieldAsVector2(FString VHLName, FString FieldName, FVector2D Value)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsVector2", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double VxValue[2] = { Value.X, Value.Y };
    if (!VortexSetInputVector2(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        LogErrorForVHLFunction("SetVHLFieldAsVector2", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeVector2);
    }
}

void UMechanismComponent::GetVHLFieldAsVector3(FString VHLName, FString FieldName, FVector& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsVector3", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }
    
    double VxValue[3] = { 0.0, 0.0, 0.0 };
    if (VortexGetOutputVector3(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        Value.X = VxValue[0];
        Value.Y = VxValue[1];
        Value.Z = VxValue[2];
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsVector3", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeVector3);
    }
}

void UMechanismComponent::SetVHLFieldAsVector3(FString VHLName, FString FieldName, FVector Value)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsVector3", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double VxValue[3] = { Value.X, Value.Y, Value.Z };
    if (!VortexSetInputVector3(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        LogErrorForVHLFunction("SetVHLFieldAsVector3", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeVector3);
    }
}

void UMechanismComponent::GetVHLFieldAsVector4(FString VHLName, FString FieldName, FVector4& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsVector4", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double VxValue[4] = { 0.0, 0.0, 0.0, 0.0 };
    if (VortexGetOutputVector4(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        Value.X = VxValue[0];
        Value.Y = VxValue[1];
        Value.Z = VxValue[2];
        Value.W = VxValue[3];
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsVector4", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeVector4);
    }
}

void UMechanismComponent::SetVHLFieldAsVector4(FString VHLName, FString FieldName, FVector4 Value)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsVector4", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double VxValue[4] = { Value.X, Value.Y, Value.Z, Value.W };
    if (!VortexSetInputVector4(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), VxValue))
    {
        LogErrorForVHLFunction("SetVHLFieldAsVector4", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeVector4);
    }
}

void UMechanismComponent::GetVHLFieldAsString(FString VHLName, FString FieldName, FString& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsString", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    static const int kShortStringSize = 128;
    std::uint32_t stringSize = kShortStringSize;
    std::string VxValue; VxValue.resize(stringSize);
    if (VortexGetOutputString(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &VxValue.front(), &stringSize))
    {
        if (stringSize > kShortStringSize)
        {
            VxValue.resize(stringSize);
            VortexGetOutputString(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &VxValue.front(), &stringSize);
        }

        Value = UTF8_TO_TCHAR(VxValue.c_str());
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsString", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeString);
    }
}

void UMechanismComponent::SetVHLFieldAsString(FString VHLName, FString FieldName, FString Value)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValue);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsString", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    if (!VortexSetInputString(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), TCHAR_TO_UTF8(*Value)))
    {
        LogErrorForVHLFunction("SetVHLFieldAsString", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeString);
    }
}

void UMechanismComponent::GetVHLFieldAsTransform(FString VHLName, FString FieldName, FTransform& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueTransform);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsTransform", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double Translation[3];
    double Rotation[4];

    if (VortexGetOutputMatrix(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), Translation, Rotation))
    {
        Value = VortexIntegrationUtilities::ConvertTransform(Translation, Rotation);
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsTransform", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeMatrix);
    }
}

void UMechanismComponent::SetVHLFieldAsTransform(FString VHLName, FString FieldName, FTransform Transform)
{
    SCOPE_CYCLE_COUNTER(STAT_SetVHLValueTransform);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("SetVHLFieldAsTransform", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double Translation[3];
    double Rotation[4];

    VortexIntegrationUtilities::ConvertTransform(Transform, Translation, Rotation);

    if (!VortexSetInputMatrix(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), Translation, Rotation))
    {
        LogErrorForVHLFunction("SetVHLFieldAsTransform", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeInput, kVortexDataTypeMatrix);
    }
}

void UMechanismComponent::GetVHLFieldAsIMobile(FString VHLName, FString FieldName, FTransform& Value)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueIMobile);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsIMobile", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    double Translation[3];
    double Rotation[4];

    if (VortexGetOutputIMobile(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), Translation, Rotation))
    {
        Value = VortexIntegrationUtilities::ConvertTransform(Translation, Rotation);
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsIMobile", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeExtensionPointer);
    }
}

void UMechanismComponent::GetVHLFieldAsHeightField(FString VHLName, FString FieldName,
    TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UV0, TArray<FProcMeshTangent>& Tangents)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueHeightfield);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsHeightField", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    VortexHeightField HeightField = {};

    if (VortexGetOutputHeightField(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &HeightField))
    {
        std::uint32_t VertexCount = HeightField.nbVerticesY * HeightField.nbVerticesX;
        if (VertexCount >= 3)
        {
            auto IndexOf = [&](int32 x, int32 y)
            {
                if (x < 0)
                    x = 0;
                if (y < 0)
                    y = 0;
                if (x >= int32(HeightField.nbVerticesX))
                    x = HeightField.nbVerticesX - 1;
                if (y >= int32(HeightField.nbVerticesY))
                    y = HeightField.nbVerticesY - 1;
                return y * HeightField.nbVerticesX + x;
            };

            double halfWidth = 0.5 * HeightField.cellSizeX * (HeightField.nbVerticesX - 1);
            double halfHeight = 0.5 * HeightField.cellSizeY * (HeightField.nbVerticesY - 1);

            Vertices.SetNum(VertexCount);
            UV0.SetNum(VertexCount);
            for (std::uint32_t y = 0, i = 0; y < HeightField.nbVerticesY; ++y)
            {
                for (std::uint32_t x = 0; x < HeightField.nbVerticesX; ++x)
                {
                    double pos[3] = {
                        HeightField.cellSizeX * x - halfWidth,
                        HeightField.cellSizeY * y - halfHeight,
                        HeightField.heights[IndexOf(x, y)] };
                    Vertices[i] = VortexIntegrationUtilities::ConvertTranslation(pos);
                    UV0[i++] = FVector2D(pos[0], pos[1]);
                }
            }

            // calculate smooth normals
            Normals.SetNum(VertexCount);
            Tangents.SetNum(VertexCount);
            for (int32 y = 0, i = 0; y < int32(HeightField.nbVerticesY); ++y)
            {
                for (int32 x = 0; x < int32(HeightField.nbVerticesX); ++x)
                {
                    FVector diag1 = Vertices[IndexOf(x + 1, y + 1)] - Vertices[IndexOf(x - 1, y - 1)];
                    FVector diag2 = Vertices[IndexOf(x + 1, y - 1)] - Vertices[IndexOf(x - 1, y + 1)];
                    FVector norm1 = FVector::CrossProduct(diag1, diag2).GetUnsafeNormal();
                    FVector line1 = Vertices[IndexOf(x + 0, y + 1)] - Vertices[IndexOf(x + 0, y - 1)];
                    FVector line2 = Vertices[IndexOf(x + 1, y + 0)] - Vertices[IndexOf(x - 1, y + 0)];
                    FVector norm2 = FVector::CrossProduct(line1, line2).GetUnsafeNormal();
                    FVector normal = 0.5f * (norm1 + norm2);
                    Normals[i] = normal;
                    Tangents[i++] = FProcMeshTangent{ FVector::CrossProduct(normal, FVector(1, 0, 0)), false };
                }
            }

            // build triangle index list
            Triangles.SetNum(6 * (HeightField.nbVerticesY - 1) * (HeightField.nbVerticesX - 1));
            for (std::uint32_t y = 0, i = 0; y < HeightField.nbVerticesY - 1; ++y)
            {
                for (std::uint32_t x = 0; x < HeightField.nbVerticesX - 1; ++x)
                {
                    /// mirror vs   no
                    /// |/|\|      |/|/|
                    /// |\|/|  vs  |/|/|
                    /// |/|\|      |/|/|
                    int mirror = y % 2 ^ x % 2;
                    Triangles[i++] = IndexOf(x + 1 - mirror, y);
                    Triangles[i++] = IndexOf(x + mirror, y + 1);
                    Triangles[i++] = IndexOf(x, y + mirror);

                    Triangles[i++] = IndexOf(x + 1, y + 1 - mirror);
                    Triangles[i++] = IndexOf(x + mirror, y + 1);
                    Triangles[i++] = IndexOf(x + 1 - mirror, y);
                }
            }
        }
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsHeightField", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeExtensionPointer);
    }
}

void UMechanismComponent::GetVHLFieldAsTiledHeightField(FString VHLName, FString FieldName, UProceduralMeshComponent* ProceduralMesh, bool bUpdate, bool& bOK)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueTiledHeightField);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsTiledHeightField", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    VortexTiledHeightField TiledHeightField = {};

    // get the tile count
    if (VortexGetOutputTiledHeightField(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &TiledHeightField, bUpdate))
    {
        // allocate the structure to receive the tiles
        TArray<VortexTiledHeightFieldTile> Tiles;
        Tiles.SetNum(TiledHeightField.tileCount);
        TiledHeightField.tiles = Tiles.GetData();
        TArray<VortexTiledHeightFieldTile> StitchTiles;
        StitchTiles.SetNum(TiledHeightField.stitchTileCount);
        TiledHeightField.stitchTiles = StitchTiles.GetData();

        // get the tiles
        VortexGetOutputTiledHeightField(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &TiledHeightField, bUpdate);

        int32 TileVerticesX = TiledHeightField.nbVerticesX;
        int32 TileVerticesY = TiledHeightField.nbVerticesY;
        int32 TileVertexCount = TileVerticesX * TileVerticesY;
        if (TileVertexCount < 3)
        {
            bOK = false;
            return;
        }
        int32 SectionVerticesX = TiledHeightField.nbVerticesX + 1;
        int32 SectionVerticesY = TiledHeightField.nbVerticesY + 1;
        int32 SectionVertexCount = SectionVerticesX * SectionVerticesY;
        int32 TileCountX = TiledHeightField.tileCountX;
        int32 TileCountY = TiledHeightField.tileCountY;

        auto IndexOfSectionCell = [SectionVerticesX, SectionVerticesY](int32 cellX, int32 cellY) -> int32
        {
            if (cellX < 0)
                cellX = 0;
            if (cellY < 0)
                cellY = 0;
            if (cellX >= SectionVerticesX)
                cellX = SectionVerticesX - 1;
            if (cellY >= SectionVerticesY)
                cellY = SectionVerticesY - 1;
            return cellY * SectionVerticesX + cellX;
        };

        auto IndexOfTileCell = [TileVerticesX, TileVerticesY](int32 cellX, int32 cellY) -> int32
        {
            if (cellX < 0)
                cellX = 0;
            if (cellY < 0)
                cellY = 0;
            if (cellX >= TileVerticesX)
                cellX = TileVerticesX - 1;
            if (cellY >= TileVerticesY)
                cellY = TileVerticesY - 1;
            return cellY * TileVerticesX + cellX;
        };

        auto IndexOfSectionOrTile = [TileCountX, TileCountY](int32 tileX, int32 tileY) -> int32
        {
            if (tileX < 0)
                tileX = 0;
            if (tileY < 0)
                tileY = 0;
            if (tileX >= TileCountX)
                tileX = TileCountX - 1;
            if (tileY >= TileCountY)
                tileY = TileCountY - 1;
            return tileX * TileCountY + tileY;
        };

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UV0;
        TArray<FProcMeshTangent> Tangents;
        TArray<FColor> Colors;
        for (const auto& Tile : Tiles)
        {
            auto GetHeight = [&](int32 tileX, int32 tileY, int32 cellX, int32 cellY) -> float
            {
                int32 StitchedTileX = tileX;
                int32 StitchedTileY = tileY;
                int32 StitchedCellX = cellX;
                int32 StitchedCellY = cellY;

                // handle tile stiching at tile creation by peeking connected tiles when on an edge cell
                if (StitchedCellX < 0)
                {
                    StitchedCellX += TileVerticesX;
                    StitchedTileX -= 1;
                }
                if (StitchedCellY < 0)
                {
                    StitchedCellY += TileVerticesY;
                    StitchedTileY -= 1;
                }
                if (StitchedCellX >= TileVerticesX)
                {
                    StitchedCellX -= TileVerticesX;
                    StitchedTileX += 1;
                }
                if (StitchedCellY >= TileVerticesY)
                {
                    StitchedCellY -= TileVerticesY;
                    StitchedTileY += 1;
                }

                if (bUpdate)
                {
                    if (StitchedTileX != tileX || StitchedTileY != tileY ||
                        StitchedCellX != cellX || StitchedCellY != cellY)
                    {
                        // for stitching during updates, find the connecting tile by searching for a matching Tile->tileX && Tile->tileY
                        // do NOT use the tile index when updating since not all tiles are present.
                        for (const auto& TileStitch : Tiles)
                        {
                            if (TileStitch.tileX == StitchedTileX && TileStitch.tileY == StitchedTileY)
                            {
                                return TileStitch.heights[IndexOfTileCell(StitchedCellX, StitchedCellY)];
                            }
                        }
                        // we need another pass of stitching against tiles that were not updated this frame
                        for (const auto& TileStitch : StitchTiles)
                        {
                            if (TileStitch.tileX == StitchedTileX && TileStitch.tileY == StitchedTileY)
                            {
                                return TileStitch.heights[IndexOfTileCell(StitchedCellX, StitchedCellY)];
                            }
                        }
                    }

                    // fallback
                    return Tile.heights[IndexOfTileCell(cellX, cellY)];
                }
                else
                {
                    return Tiles[IndexOfSectionOrTile(StitchedTileX, StitchedTileY)].heights[IndexOfTileCell(StitchedCellX, StitchedCellY)];
                }
            };

            auto GetLandUseID = [&](int32 tileX, int32 tileY, int32 cellX, int32 cellY) -> float
            {
                int32 StitchedTileX = tileX;
                int32 StitchedTileY = tileY;
                int32 StitchedCellX = cellX;
                int32 StitchedCellY = cellY;

                // handle tile stiching at tile creation by peeking connected tiles when on an edge cell
                if (StitchedCellX < 0)
                {
                    StitchedCellX += TileVerticesX;
                    StitchedTileX -= 1;
                }
                if (StitchedCellY < 0)
                {
                    StitchedCellY += TileVerticesY;
                    StitchedTileY -= 1;
                }
                if (StitchedCellX >= TileVerticesX)
                {
                    StitchedCellX -= TileVerticesX;
                    StitchedTileX += 1;
                }
                if (StitchedCellY >= TileVerticesY)
                {
                    StitchedCellY -= TileVerticesY;
                    StitchedTileY += 1;
                }

                if (bUpdate)
                {
                    if (StitchedTileX != tileX || StitchedTileY != tileY ||
                        StitchedCellX != cellX || StitchedCellY != cellY)
                    {
                        // for stitching during updates, find the connecting tile by searching for a matching Tile->tileX && Tile->tileY
                        // do NOT use the tile index when updating since not all tiles are present.
                        for (const auto& TileStitch : Tiles)
                        {
                            if (TileStitch.tileX == StitchedTileX && TileStitch.tileY == StitchedTileY)
                            {
                                return TileStitch.landUseIds[IndexOfTileCell(StitchedCellX, StitchedCellY)];
                            }
                        }
                        // we need another pass of stitching against tiles that were not updated this frame
                        for (const auto& TileStitch : StitchTiles)
                        {
                            if (TileStitch.tileX == StitchedTileX && TileStitch.tileY == StitchedTileY)
                            {
                                return TileStitch.landUseIds[IndexOfTileCell(StitchedCellX, StitchedCellY)];
                            }
                        }
                    }

                    // fallback
                    return Tile.landUseIds[IndexOfTileCell(cellX, cellY)];
                }
                else
                {
                    return Tiles[IndexOfSectionOrTile(StitchedTileX, StitchedTileY)].landUseIds[IndexOfTileCell(StitchedCellX, StitchedCellY)];
                }
            };

            Colors.SetNum(SectionVertexCount);
            Vertices.SetNum(SectionVertexCount);
            UV0.SetNum(SectionVertexCount);
            for (int32 y = 0, i = 0; y < SectionVerticesY; ++y)
            {
                for (int32 x = 0; x < SectionVerticesX; ++x)
                {
                    double pos[3] = {
                        TiledHeightField.cellSizeX * x + Tile.tilePositionX,
                        TiledHeightField.cellSizeY * y + Tile.tilePositionY,
                        GetHeight(Tile.tileX, Tile.tileY, x, y) };
                    Vertices[i] = VortexIntegrationUtilities::ConvertTranslation(pos);
                    UV0[i] = FVector2D(pos[0], pos[1]);
                    Colors[i++] = GetLandUseID(Tile.tileX, Tile.tileY, x, y) == 0 ? FColor(255, 255, 255) : FColor(0, 0, 0); // color mask to be used by mixing in material
                }
            }

            // calculate smooth normals
            Normals.SetNum(SectionVertexCount);
            Tangents.SetNum(SectionVertexCount);
            for (int32 y = 0, i = 0; y < SectionVerticesY; ++y)
            {
                for (int32 x = 0; x < SectionVerticesX; ++x)
                {
                    // normal smoothing is not fuly stitched, we stop smoothing at the current tile's border
                    FVector diag1 = Vertices[IndexOfSectionCell(x + 1, y + 1)] - Vertices[IndexOfSectionCell(x - 1, y - 1)];
                    FVector diag2 = Vertices[IndexOfSectionCell(x + 1, y - 1)] - Vertices[IndexOfSectionCell(x - 1, y + 1)];
                    FVector norm1 = FVector::CrossProduct(diag1, diag2).GetUnsafeNormal();
                    FVector line1 = Vertices[IndexOfSectionCell(x + 0, y + 1)] - Vertices[IndexOfSectionCell(x + 0, y - 1)];
                    FVector line2 = Vertices[IndexOfSectionCell(x + 1, y + 0)] - Vertices[IndexOfSectionCell(x - 1, y + 0)];
                    FVector norm2 = FVector::CrossProduct(line1, line2).GetUnsafeNormal();
                    FVector normal = 0.5f * (norm1 + norm2);
                    Normals[i] = normal;
                    Tangents[i++] = FProcMeshTangent{ FVector::CrossProduct(normal, FVector(1, 0, 0)), false };
                }
            }

            if (!bUpdate)
            {
                // build triangle index list
                Triangles.SetNum(6 * (SectionVerticesY - 1) * (SectionVerticesX - 1));
                for (int32 y = 0, i = 0; y < SectionVerticesY - 1; ++y)
                {
                    for (int32 x = 0; x < SectionVerticesX - 1; ++x)
                    {
                        /// mirror vs   no
                        /// |/|\|      |/|/|
                        /// |\|/|  vs  |/|/|
                        /// |/|\|      |/|/|
                        int mirror = y % 2 ^ x % 2;
                        Triangles[i++] = IndexOfSectionCell(x + 1 - mirror, y);
                        Triangles[i++] = IndexOfSectionCell(x + mirror, y + 1);
                        Triangles[i++] = IndexOfSectionCell(x, y + mirror);

                        Triangles[i++] = IndexOfSectionCell(x + 1, y + 1 - mirror);
                        Triangles[i++] = IndexOfSectionCell(x + mirror, y + 1);
                        Triangles[i++] = IndexOfSectionCell(x + 1 - mirror, y);
                    }
                }
            }

            int32 TileIndex = IndexOfSectionOrTile(Tile.tileX, Tile.tileY);
            if (bUpdate)
            {
                ProceduralMesh->UpdateMeshSection(TileIndex, Vertices, Normals, UV0, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), Colors, Tangents);
            }
            else
            {
                ProceduralMesh->CreateMeshSection(TileIndex, Vertices, Triangles, Normals, UV0, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), Colors, Tangents, false);
            }

            bOK = true;
        }
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsTiledHeightField", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeExtensionPointer);
    }
}

void UMechanismComponent::GetVHLFieldAsParticles(FString VHLName, FString FieldName, TArray<FVector>& Positions, TArray<float>& Radii, TArray<int32>& IDs)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueParticle);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsParticles", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    VortexParticles particles = {};

    if (VortexGetOutputParticles(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &particles))
    {
        Positions.SetNum(particles.count);
        Radii.SetNum(particles.count);
        IDs.SetNum(particles.count);

        for (std::uint32_t i = 0; i < particles.count; ++i)
        {
            double pos[3] = { particles.positions[i * 3], particles.positions[i * 3 + 1], particles.positions[i * 3 + 2] };
            Positions[i] = VortexIntegrationUtilities::ConvertTranslation(pos);
            Radii[i] = particles.radii[i];
            IDs[i] = particles.ids[i];
        }
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsParticles", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeExtensionPointer);
    }
}

void UMechanismComponent::GetVHLFieldAsGraphicsMesh(FString VHLName, FString FieldName,
    TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UV0, TArray<FVector2D>& UV1, TArray<FVector2D>& UV2, TArray<FProcMeshTangent>& Tangents)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueGraphicsMesh);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsGraphicsMesh", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    static const std::uint32_t kMeshDataCount = 4;
    VortexMeshData meshDataArray[kMeshDataCount] = {};
    std::uint32_t meshDataCount = kMeshDataCount;

    if (VortexGetOutputGraphicsMesh(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), meshDataArray, &meshDataCount))
    {
        for (std::uint32_t j = 0; j < meshDataCount && j < kMeshDataCount; ++j)
        {
            const VortexMeshData& MeshData = meshDataArray[j];

            Triangles.SetNum(Triangles.Num() + MeshData.indexCount);
            const std::uint8_t* indices = reinterpret_cast<const std::uint8_t*>(MeshData.indexes);
            switch (MeshData.indexSize)
            {
            case 4:
                for (std::uint32_t i = 0; i < MeshData.indexCount; ++i)
                {
                    std::uint32_t index = *reinterpret_cast<const std::uint32_t*>(&indices[i * 4]);
                    Triangles[i] = index;
                }
                break;
            case 2:
                for (std::uint32_t i = 0; i < MeshData.indexCount; ++i)
                {
                    std::uint32_t index = *reinterpret_cast<const std::uint16_t*>(&indices[i * 2]);
                    Triangles[i] = index;
                }
                break;
            case 1:
                for (std::uint32_t i = 0; i < MeshData.indexCount; ++i)
                {
                    std::uint32_t index = *reinterpret_cast<const std::uint8_t*>(&indices[i * 1]);
                    Triangles[i] = index;
                }
                break;
            default:
                break;
            }

            Vertices.SetNum(Vertices.Num() + MeshData.vertexCount);
            for (std::uint32_t i = 0; i < MeshData.vertexCount; ++i)
            {
                double pos[3] = { MeshData.vertices[i * 3], MeshData.vertices[i * 3 + 1], MeshData.vertices[i * 3 + 2] };
                Vertices[i] = VortexIntegrationUtilities::ConvertTranslation(pos);
            }

            UV0.SetNum(UV0.Num() + MeshData.uvs[0].uvCount);
            for (std::uint32_t i = 0; i < MeshData.uvs[0].uvCount; ++i)
            {
                UV0[i] = FVector2D(MeshData.uvs[0].uvs[i * MeshData.uvs[0].uvSize], MeshData.uvs[0].uvs[i * MeshData.uvs[0].uvSize + 1]);
            }

            UV1.SetNum(UV1.Num() + MeshData.uvs[1].uvCount);
            for (std::uint32_t i = 0; i < MeshData.uvs[1].uvCount; ++i)
            {
                UV1[i] = FVector2D(MeshData.uvs[1].uvs[i * MeshData.uvs[1].uvSize], MeshData.uvs[1].uvs[i * MeshData.uvs[1].uvSize + 1]);
            }

            UV2.SetNum(UV2.Num() + MeshData.uvs[2].uvCount);
            for (std::uint32_t i = 0; i < MeshData.uvs[2].uvCount; ++i)
            {
                UV2[i] = FVector2D(MeshData.uvs[2].uvs[i * MeshData.uvs[2].uvSize], MeshData.uvs[2].uvs[i * MeshData.uvs[2].uvSize + 1]);
            }

            Normals.SetNum(Normals.Num() + MeshData.normalCount);
            for (std::uint32_t i = 0; i < MeshData.normalCount; ++i)
            {
                double dir[3] = { MeshData.normals[i * 3], MeshData.normals[i * 3 + 1], MeshData.normals[i * 3 + 2] };
                Normals[i] = VortexIntegrationUtilities::ConvertDirection(dir);
            }

            Tangents.SetNum(Tangents.Num() + MeshData.tangentCount);
            for (std::uint32_t i = 0; i < MeshData.tangentCount; ++i)
            {
                double dir[3] = { MeshData.tangents[i * 3], MeshData.tangents[i * 3 + 1], MeshData.tangents[i * 3 + 2] };
                Tangents[i] = FProcMeshTangent(VortexIntegrationUtilities::ConvertDirection(dir), false);
            }
        }
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsGraphicsMesh", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeExtensionPointer);
    }
}

void UMechanismComponent::GetVHLFieldAsGraphicsMaterial(FString VHLName, FString FieldName,
    UTexture2D*& EmissionColor0, UTexture2D*& EmissionColor1, UTexture2D*& EmissionColor2,
    UTexture2D*& OcclusionColor0, UTexture2D*& OcclusionColor1, UTexture2D*& OcclusionColor2,
    UTexture2D*& AlbedoColor0, UTexture2D*& AlbedoColor1, UTexture2D*& AlbedoColor2,
    UTexture2D*& SpecularColor0, UTexture2D*& SpecularColor1, UTexture2D*& SpecularColor2,
    UTexture2D*& GlossColor0, UTexture2D*& GlossColor1, UTexture2D*& GlossColor2,
    UTexture2D*& MetalnessColor0, UTexture2D*& MetalnessColor1, UTexture2D*& MetalnessColor2,
    UTexture2D*& RoughnessColor0, UTexture2D*& RoughnessColor1, UTexture2D*& RoughnessColor2,
    UTexture2D*& NormalColor0, UTexture2D*& NormalColor1, UTexture2D*& NormalColor2,
    UTexture2D*& HeightMapColor0, UTexture2D*& HeightMapColor1, UTexture2D*& HeightMapColor2)
{
    SCOPE_CYCLE_COUNTER(STAT_GetVHLValueMaterial);
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
        return;

    if (!PreValidateVHLFunction("GetVHLFieldAsGraphicsMaterial", VortexMechanism->MechanismFilepath.FilePath, VHLName, FieldName))
    {
        return;
    }

    VortexMaterialData material = {};

    if (VortexGetOutputGraphicsMaterial(VortexObject, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), &material))
    {
        auto assign = [&](const VortexMaterialLayerData& layerData, UTexture2D*& texture)
        {
            if (layerData.textureHandle != nullptr)
            {
                VortexTextureData TextureData = {};
                if (VortexGetGraphicsTextureData(layerData.textureHandle, &TextureData))
                {
                    if (TextureData.sizeX > 0 && TextureData.sizeY > 0)
                    {
                        EPixelFormat format = PF_R8G8B8A8;
                        switch (TextureData.format)
                        {
                        case kVortexTextureFormatR8G8B8A8:
                            format = PF_R8G8B8A8;
                            break;
                        case kVortexTextureFormatBC1RGB:
                            format = PF_DXT1;
                            break;
                        case kVortexTextureFormatBC1RGBA:
                            format = PF_DXT1;
                            break;
                        case kVortexTextureFormatBC2:
                            format = PF_DXT3;
                            break;
                        case kVortexTextureFormatBC3:
                            format = PF_DXT5;
                            break;
                        }
                        texture = UTexture2D::CreateTransient(TextureData.sizeX, TextureData.sizeY, format);
                        FTexture2DMipMap& Mip = texture->PlatformData->Mips[0];
                        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
                        FMemory::Memcpy(Data, TextureData.bytes, TextureData.byteCount);
                        Mip.BulkData.Unlock();

                        texture->PlatformData->SetNumSlices(1);
                        texture->NeverStream = true;
                        texture->UpdateResource();
                    }
                }
            }
        };

        assign(material.emissionLayers[0], EmissionColor0);
        assign(material.emissionLayers[1], EmissionColor1);
        assign(material.emissionLayers[2], EmissionColor2);

        assign(material.occlusionLayers[0], OcclusionColor0);
        assign(material.occlusionLayers[1], OcclusionColor1);
        assign(material.occlusionLayers[2], OcclusionColor2);

        assign(material.albedoLayers[0], AlbedoColor0);
        assign(material.albedoLayers[1], AlbedoColor1);
        assign(material.albedoLayers[2], AlbedoColor2);

        assign(material.specularLayers[0], SpecularColor0);
        assign(material.specularLayers[1], SpecularColor1);
        assign(material.specularLayers[2], SpecularColor2);

        assign(material.glossLayers[0], GlossColor0);
        assign(material.glossLayers[1], GlossColor1);
        assign(material.glossLayers[2], GlossColor2);

        assign(material.metalnessLayers[0], MetalnessColor0);
        assign(material.metalnessLayers[1], MetalnessColor1);
        assign(material.metalnessLayers[2], MetalnessColor2);

        assign(material.roughnessLayers[0], RoughnessColor0);
        assign(material.roughnessLayers[1], RoughnessColor1);
        assign(material.roughnessLayers[2], RoughnessColor2);

        assign(material.normalLayers[0], NormalColor0);
        assign(material.normalLayers[1], NormalColor1);
        assign(material.normalLayers[2], NormalColor2);

        assign(material.heightMapLayers[0], HeightMapColor0);
        assign(material.heightMapLayers[1], HeightMapColor1);
        assign(material.heightMapLayers[2], HeightMapColor2);
    }
    else
    {
        LogErrorForVHLFunction("GetVHLFieldAsGraphicsMaterial", VortexMechanism->MechanismFilepath.FilePath, VortexObject, VHLName, FieldName, kVortexFieldTypeOutput, kVortexDataTypeExtensionPointer);
    }
}

bool UMechanismComponent::PreValidateVHLFunction(const FString& FunctionName, const FString& FilePath, const FString& VHLName, const FString& FieldName)
{
    bool CanCallVHLFunction = true;

    FString ActorName = "";
    if (AActor * Actor = GetOwner())
    {
        ActorName = Actor->GetName();
    }

    if (!HasBegunPlay())
    {
        CanCallVHLFunction = false;
        UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::%s(): Actor \"%s\", Mechanism \"%s\": Interface \"%s\" and field \"%s\". The component has not begun play yet. Please make sure to call this function only after the \"BeginPlay\" event has been triggered."), *FunctionName, *ActorName, *FilePath, *VHLName, *FieldName);
    }
    else if (VortexObject == nullptr)
    {
        CanCallVHLFunction = false;
        UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::%s(): Actor \"%s\", Mechanism \"%s\": Interface \"%s\" and field \"%s\". The associated Vortex Mechanism is not loaded. An unknown error occured."), *FunctionName, *ActorName, *FilePath, *VHLName, *FieldName);
    }

    return CanCallVHLFunction;
}

void UMechanismComponent::LogErrorForVHLFunction(const FString& FunctionName, const FString& FilePath, VortexObjectHandle objectHandle, const FString& VHLName, const FString& FieldName, VortexFieldType fieldType, VortexDataType dataType)
{
    FString ActorName = "";
    if (AActor * Actor = GetOwner())
    {
        ActorName = Actor->GetName();
    }

    auto status = VortexGetFieldStatus(objectHandle, TCHAR_TO_UTF8(*VHLName), TCHAR_TO_UTF8(*FieldName), fieldType, dataType);

    const FString fieldTypeStr = fieldType == kVortexFieldTypeInput ? "Input" : "Output";

    switch (status)
    {
    case kVortexFieldStatusNotFound:
        UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::%s(): Actor \"%s\", Mechanism \"%s\", Interface \"%s\": %s \"%s\" does not exist!"), *FunctionName, *ActorName, *FilePath, *VHLName, *fieldTypeStr, *FieldName);
        break;
    case kVortexFieldStatusWrongDataType:
        UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::%s(): Actor \"%s\", Mechanism \"%s\", Interface \"%s\": %s \"%s\" does not have the right type!"), *FunctionName, *ActorName, *FilePath, *VHLName, *fieldTypeStr, *FieldName);
        break;
    case kVortexFieldStatusInterfaceNotFound:
        UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::%s(): Actor \"%s\", Mechanism \"%s\": Interface \"%s\" could not be found!"), *FunctionName, *ActorName, *FilePath, *VHLName);
        break;
    case kVortexFieldStatusObjectNotFound:
        UE_LOG(LogVortex, Error, TEXT("UMechanismComponent::%s(): Actor \"%s\", Mechanism \"%s\": Invalid Vortex object!"), *FunctionName, *ActorName, *FilePath);
        break;
    default:
        break;
    }
}

void UMechanismComponentMappingBlueprint::SetComponent(const FMechanismComponentMapping& MappingRef, FMechanismComponentMapping& Mapping, USceneComponent* SceneComponent)
{
    Mapping.TransformFieldName = MappingRef.TransformFieldName;
    Mapping.Component.OtherActor = SceneComponent->GetOwner();
    Mapping.Component.ComponentProperty = *SceneComponent->GetName();
}

void UMechanismComponentMappingBlueprint::Set(const FMechanismComponentMapping& MappingRef, FMechanismComponentMapping& Mapping, AActor* Actor, FName ComponentProperty)
{
    Mapping.TransformFieldName = MappingRef.TransformFieldName;
    Mapping.Component.OtherActor = Actor;
    Mapping.Component.ComponentProperty = ComponentProperty;
}
