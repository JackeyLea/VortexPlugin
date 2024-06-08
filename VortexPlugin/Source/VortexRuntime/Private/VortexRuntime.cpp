#include "VortexRuntime.h"
#include "VortexTerrain.h"

#include "VortexApplicationBlueprintLib.h"
#include "VortexIntegrationUtilities.h"
#include "VortexSettings.h"
#include "WindowsUtilities.h"
#include "Engine/Classes/Engine/Engine.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "VortexIntegration/Version.h"
#include "VortexIntegration/GraphicsIntegration.h"
#include "MechanismComponent.h"
#include "VortexLidarActor.h"
#include "VortexLidarActorComponent.h"
#include "VortexDepthCameraActor.h"
#include "VortexDepthCameraActorComponent.h"
#include "VortexColorCameraActor.h"
#include "VortexColorCameraActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR
#include "Editor.h" // for FEditorDelegates
#endif

#include <string>

#define LOCTEXT_NAMESPACE "FVortexRuntimeModule"

DEFINE_LOG_CATEGORY(LogVortex);

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_ModuleTick, STATGROUP_VortexRuntimeModule);

namespace
{
    void VortexLidarCallback(GraphicNotificationType type, GraphicsLidarInfo* graphicsLidar)
    {
        if (type == GraphicNotificationType_Add) 
        {
            FVortexRuntimeModule::Get().AssociateLidarToRegisteringComponent(*graphicsLidar);
        }
        else if (type == GraphicNotificationType_Remove)
        {
            FVortexRuntimeModule::Get().UnassociateLidarFromUnregisteringComponent(graphicsLidar->id);
        }
        else if (type == GraphicNotificationType_Update)
        {
            FVortexRuntimeModule::Get().UpdateLidar(*graphicsLidar);
        }
    }

    void VortexDepthCameraCallback(GraphicNotificationType type, GraphicsDepthCameraInfo* graphicsDepthCamera)
    {
        if (type == GraphicNotificationType_Add)
        {
            FVortexRuntimeModule::Get().AssociateDepthCameraToRegisteringComponent(*graphicsDepthCamera);
        }
        else if (type == GraphicNotificationType_Remove)
        {
            FVortexRuntimeModule::Get().UnassociateDepthCameraFromUnregisteringComponent(graphicsDepthCamera->id);
        }
        else if (type == GraphicNotificationType_Update)
        {
            FVortexRuntimeModule::Get().UpdateDepthCamera(*graphicsDepthCamera);
        }
    }

    void VortexColorCameraCallback(GraphicNotificationType type, GraphicsColorCameraInfo* graphicsColorCamera)
    {
        if (type == GraphicNotificationType_Add)
        {
            FVortexRuntimeModule::Get().AssociateColorCameraToRegisteringComponent(*graphicsColorCamera);
        }
        else if (type == GraphicNotificationType_Remove)
        {
            FVortexRuntimeModule::Get().UnassociateColorCameraFromUnregisteringComponent(graphicsColorCamera->id);
        }
        else if (type == GraphicNotificationType_Update)
        {
            FVortexRuntimeModule::Get().UpdateColorCamera(*graphicsColorCamera);
        }
    }

    const TCHAR* PromptForDebuggerEnvVar = TEXT("DEBUG_VORTEX_UNREAL_PLUGIN");

    // What do we consider as being an outstanding period (delta time)? Here, we arbitrary define a factor of the period that we consider as invalid.
    const double OutstandingPeriodFactor = 4.0;

    bool SortStrings(const FString& first, const FString& second)
    {
        return first < second;
    }

    void ValidateUnrealAndVortexFrameRates()
    {
        if (!FVortexRuntimeModule::IsIntegrationLoaded())
        {
            return;
        }

        if (GEngine->bUseFixedFrameRate)
        {
            if (fabs(VortexGetSimulationFrameRate() - GEngine->FixedFrameRate) > 0.001)
            {
                FText message = FText::Format(LOCTEXT("VortexRuntime", "When using a fixed frame rate, Vortex Studio and Unreal Engine must run at the same frame rate.\n\nPlease make sure both are running at the same frame rate.\n\nVortex Studio is configured to run at {0} FPS.\nUnreal Engine is configured to run at {1} FPS."),
                    FText::AsNumber(VortexGetSimulationFrameRate()),
                    FText::AsNumber(GEngine->FixedFrameRate));

                FMessageDialog::Open(EAppMsgType::Ok, message);

                UE_LOG(LogVortex, Error, TEXT("%s"), *(message.ToString()));
            }
        }
        else
        {
            UE_LOG(LogVortex, Warning, TEXT("Unreal Engine is currently running using a variable frame rate. A fixed frame rate is recommended when using Vortex Studio."));
        }
    }

    std::pair<std::string, std::string> ExtractMajorMinorVersion(const std::string& Version)
    {
        std::string MajorVersion = "";
        std::string MinorVersion = "";

        size_t FirstDotIndex = Version.find('.');
        if (FirstDotIndex != std::string::npos)
        {
            MajorVersion = Version.substr(0, FirstDotIndex);

            size_t MinorVersionStartingIndex = FirstDotIndex + 1;
            size_t SecondDotIndex = Version.find('.', MinorVersionStartingIndex);
            if (SecondDotIndex != std::string::npos)
            {
                MinorVersion = Version.substr(MinorVersionStartingIndex, SecondDotIndex - MinorVersionStartingIndex);
            }
        }

        return std::make_pair(MajorVersion, MinorVersion);
    }

    std::string ExtractVersion(const std::string& Installation, bool bCurrentUser, const std::pair<std::string, std::string>& Version)
    {
        std::string returnedInstallDir = "";
        long errorCode = 0;

        std::string settingsKey = "SOFTWARE\\CM Labs\\" + Installation + "\\Settings";

        std::string productVersion = WindowsUtilities::GetRegistryValue(bCurrentUser, settingsKey, "ProductVersion", &errorCode);
        if (errorCode == 0L && !productVersion.empty())
        {
            auto ProductMajorMinorVersion = ExtractMajorMinorVersion(productVersion);
            if (Version == ProductMajorMinorVersion)
            {
                std::string installLocation = WindowsUtilities::GetRegistryValue(bCurrentUser, settingsKey, "InstallLocation", &errorCode);
                if (errorCode == 0L && !installLocation.empty())
                {
                    returnedInstallDir = installLocation;
                }
            }
        }

        return returnedInstallDir;
    }

    std::pair<std::string, std::string> GetExpectedVortexVersion()
    {
        return std::make_pair(std::to_string(VORTEX_INTEGRATION_MAJOR), std::to_string(VORTEX_INTEGRATION_MINOR));
    }

    FString GetVortexStudioInstallationPath(const FString& PluginBaseDir)
    {
        FString returnedDir;

        FString vortexOverridesConfigFilePath = FPaths::Combine(PluginBaseDir, TEXT("Config/VortexOverrides.ini"));

        // By default, scan the registry for an officially installed Vortex Studio (VortexOverrides.ini is not packaged with an official build).
        if (!FPaths::FileExists(vortexOverridesConfigFilePath))
        {
            auto versionToFind = GetExpectedVortexVersion();

            long errorCode = 0L;

            // First look into current user for Vortex Installation
            std::string installDir = "";
            auto installations = WindowsUtilities::GetSubKeys(true, "SOFTWARE\\CM Labs", &errorCode);
            if (errorCode == 0L)
            {
                for (auto i = installations.begin(); i < installations.end() && installDir.empty(); ++i)
                {
                    installDir = ExtractVersion(*i, true, versionToFind);
                }
            }

            returnedDir = UTF8_TO_TCHAR(installDir.c_str());
        }
        else
        {
            // If VortexOverrides.ini exists, we read the Vortex Studio absolute path from it
            FConfigFile configFile;
            configFile.Read(vortexOverridesConfigFilePath);

            if (!configFile.GetString(TEXT("VortexOverrides"), TEXT("vortexToolkitRootDir"), returnedDir))
            {
                UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::GetVortexStudioInstallationPath(): Unable to extract \"vortexToolkitRootDir\" from the Vortex Overrides file."));
            }
        }

        return returnedDir;
    }

    // Unreal holds multiple components at the same time to represent the same piece of data in different contexts
    // Dragging an object in the 3D view, editing Blueprints, each have their own component instance
    // We want them all to share the same Vortex Mechanism reference
    // This function produces the proper hash for this purpose
    FString MakeKey(UMechanismComponent* Component)
    {
        if (Component)
        {
            if (auto * world = Component->GetWorld())
            {
                if (auto * actor = Component->GetOwner())
                {
                    if (auto * asset = Component->VortexMechanism)
                    {
                        return FMD5::HashAnsiString(*FString::Printf(TEXT("%s:%s:%s:%s:%s"),
                            *UMechanismComponent::StaticClass()->GetName(),
                            *world->GetName(),
                            *actor->GetName(),
                            *Component->GetName(),
                            *asset->MechanismFilepath.FilePath));
                    }
                }
            }
        }
        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::MakeKey(): Invalid component."));
        return FString();
    }

    void UnloadAsset(VortexObjectHandle VortexObject)
    {
        if (!FVortexRuntimeModule::IsIntegrationLoaded())
        {
            return;
        }

        if (VortexObject != nullptr)
        {
            if (VortexUnloadMechanism(VortexObject))
            {
                UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::UnloadAsset(): Successfully unloaded Vortex object."));
            }
            else
            {
                UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::UnloadAsset(): Failed to unload Vortex object."));
            }
        }
    }

    void LoadAsset(VortexObjectHandle& VortexObject, UMechanismComponent* Component)
    {
        if (!FVortexRuntimeModule::IsIntegrationLoaded())
            return;

        if (Component->VortexMechanism == nullptr)
            return;

        FString absoluteFilepath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), Component->VortexMechanism->MechanismFilepath.FilePath);

        double translation[3];
        double rotation[4];

        AActor* parentActor = Component->GetOwner();
        VortexIntegrationUtilities::ConvertTransform(parentActor->GetTransform(), translation, rotation);
        VortexObject = VortexLoadMechanism(TCHAR_TO_UTF8(*absoluteFilepath), translation, rotation);
        if (VortexObject == nullptr)
        {
            UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::LoadAsset(): Unable to load Vortex object from file <%s>."), *absoluteFilepath);
        }
        else
        {
            UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::LoadAsset(): Successfully loaded Vortex object from file <%s>."), *absoluteFilepath);
        }
    }
    
    void TerrainProviderQuery(VortexTerrainProvider Terrain, const VortexTerrainProviderRequest* request, VortexTerrainProviderResponse* response)
    {
        reinterpret_cast<FVortexTerrain*>(Terrain)->Query(request, response);
    }

    void TerrainProviderPostQuery(VortexTerrainProvider Terrain)
    {
        reinterpret_cast<FVortexTerrain*>(Terrain)->PostQuery();
    }

    void TerrainProviderOnDestroy(VortexTerrainProvider Terrain)
    {
        reinterpret_cast<FVortexTerrain*>(Terrain)->OnDestroy();
    }
}

FVortexRuntimeModule::FVortexRuntimeModule()
    : CurrentWorldContext(nullptr)
    , VortexIntegrationHandle(nullptr)
    , AccumulatedTime(0)
    , VortexPeriod(0.0)
    , Terrain(nullptr)
{
}

void FVortexRuntimeModule::RegisterComponent(UMechanismComponent* Component)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return;
    }

    // We want to enforce the fact that a mechanism is expected to be loaded in Editing mode
    if (VortexGetApplicationMode() != kVortexModeEditing)
    {
        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::RegisterComponent(): The Vortex application MUST be in Editing mode to load Vortex object from file <%s>."), *Component->VortexMechanism->MechanismFilepath.FilePath);
        return;
    }

    if (Component->VortexObject)
    {
        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::RegisterComponent(): The VortexObject should be NULL <%s>."), *Component->VortexMechanism->MechanismFilepath.FilePath);
        return;
    }

    Component->LoadedMechanismKey = MakeKey(Component);

    // Unreal holds multiple components at the same time to represent the same piece of data in different contexts
    // Components get created and destroyed quite often and sometimes out of order (undo-redo, dragging, etc)
    // We don't want to immediately unload a Vortex Mechanism if it will be loaded again in the same frame
    // We keep a short lived pool of recently unloaded mechanisms available for reuse by components sharing an identical
    // 'key'. See MakeKey() anonymous namespace function comment.
    VortexObjectHandle* reuseMechanism = MechanismPool.Find(Component->LoadedMechanismKey);
    if (reuseMechanism != nullptr)
    {
        Component->VortexObject = *reuseMechanism;
        MechanismPool.RemoveSingle(Component->LoadedMechanismKey, Component->VortexObject);
    }
    // We are also willing to share the same Vortex Mechanism reference among multiple instances of components sharing an identical
    // 'key'. See MakeKey() anonymous namespace function comment.
    if (!Component->VortexObject)
    {
        UMechanismComponent** otherComponent = MechanismComponents.Find(Component->LoadedMechanismKey);
        if (otherComponent != nullptr)
        {
            Component->VortexObject = (*otherComponent)->VortexObject;
        }
        else
        {
            RegisteringMechanismComponent = Component->LoadedMechanismKey;
            LoadAsset(Component->VortexObject, Component);
            RegisteringMechanismComponent = nullptr;
        }
    }
    // Track the new mechanism component reference
    MechanismComponents.Add(Component->LoadedMechanismKey, Component);
    MechanismActors.Add(Component->GetOwner());
}

void FVortexRuntimeModule::UnregisterComponent(UMechanismComponent* Component, bool bImmediately)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return;
    }

    MechanismActors.RemoveSingle(Component->GetOwner());
    MechanismComponents.RemoveSingle(Component->LoadedMechanismKey, Component);
    // We don't want to unload a Vortex Mechanism if another component is referring to it.
    if (!MechanismComponents.Contains(Component->LoadedMechanismKey))
    {
        if (bImmediately)
        {
            RegisteringMechanismComponent = Component->LoadedMechanismKey;
            // On EndPlay we want to unload a Vortex Mechanism immediately to guarantee no state leaks from play to play.
            UnloadAsset(Component->VortexObject);
            RegisteringMechanismComponent = nullptr;
        }
        else
        {
            // We don't want to immediately unload a Vortex Mechanism if it will be loaded again in the same frame
            MechanismPool.Add(Component->LoadedMechanismKey, Component->VortexObject);
        }
    }

    Component->onComponentUnregistered();

    Component->LoadedMechanismKey = "";
    Component->VortexObject = nullptr;
}

void FVortexRuntimeModule::UnregisterAllComponents(FString LoadedMechanismKey)
{
    // On EndPlay we want to unload a Vortex Mechanism immediately to guarantee no state leaks from play to play.
    TArray<UMechanismComponent*> siblings;
    MechanismComponents.MultiFind(LoadedMechanismKey, siblings);
    for (auto& sibling : siblings)
    {
        // Unload immediately
        UnregisterComponent(sibling, true);
    }
}

void FVortexRuntimeModule::AssociateLidarToRegisteringComponent(const GraphicsLidarInfo& lidarInfo)
{
    if (!RegisteringMechanismComponent.IsEmpty())
    {
        if (!MechanismLidars.Contains(RegisteringMechanismComponent))
        {
            MechanismLidars.Add(RegisteringMechanismComponent);
        }
        MechanismLidars[RegisteringMechanismComponent].Add(lidarInfo);
        Lidars.Add(lidarInfo.id) = nullptr;
    }
}

void FVortexRuntimeModule::UnassociateLidarFromUnregisteringComponent(uint64_t LidarId)
{
    if (!RegisteringMechanismComponent.IsEmpty())
    {
        if (Lidars.Contains(LidarId)) 
        {
            /*if (Lidars[LidarId]) 
            {
                RegisteringMechanismComponent->GetWorld()->DestroyActor(static_cast<AActor*>(Lidars[LidarId]));
            }*/
            Lidars.Remove(LidarId);
        }
        if (MechanismLidars.Contains(RegisteringMechanismComponent)) 
        {
            for (int32 i = 0; i < MechanismLidars[RegisteringMechanismComponent].Num(); ++i)
            {
                if (MechanismLidars[RegisteringMechanismComponent][i].id == LidarId)
                {
                    MechanismLidars[RegisteringMechanismComponent].RemoveAt(i);
                    break;
                }
            }
            if (MechanismLidars[RegisteringMechanismComponent].Num() == 0)
            {
                MechanismLidars.Remove(RegisteringMechanismComponent);
            }
        }
    }
}

void FVortexRuntimeModule::AssociateDepthCameraToRegisteringComponent(const GraphicsDepthCameraInfo& depthCameraInfo)
{
    if (!RegisteringMechanismComponent.IsEmpty())
    {
        if (!MechanismDepthCamera.Contains(RegisteringMechanismComponent))
        {
            MechanismDepthCamera.Add(RegisteringMechanismComponent);
        }
        MechanismDepthCamera[RegisteringMechanismComponent].Add(depthCameraInfo);
        DepthCameras.Add(depthCameraInfo.id) = nullptr;
    }
}

void FVortexRuntimeModule::UnassociateDepthCameraFromUnregisteringComponent(uint64_t DepthCameraId)
{
    if (!RegisteringMechanismComponent.IsEmpty())
    {
        if (DepthCameras.Contains(DepthCameraId))
        {
            DepthCameras.Remove(DepthCameraId);
        }
        if (MechanismDepthCamera.Contains(RegisteringMechanismComponent))
        {
            for (int32 i = 0; i < MechanismDepthCamera[RegisteringMechanismComponent].Num(); ++i)
            {
                if (MechanismDepthCamera[RegisteringMechanismComponent][i].id == DepthCameraId)
                {
                    MechanismDepthCamera[RegisteringMechanismComponent].RemoveAt(i);
                    break;
                }
            }
            if (MechanismDepthCamera[RegisteringMechanismComponent].Num() == 0)
            {
                MechanismDepthCamera.Remove(RegisteringMechanismComponent);
            }
        }
    }
}

void FVortexRuntimeModule::AssociateColorCameraToRegisteringComponent(const GraphicsColorCameraInfo& colorCameraInfo)
{
    if (!RegisteringMechanismComponent.IsEmpty())
    {
        if (!MechanismColorCamera.Contains(RegisteringMechanismComponent))
        {
            MechanismColorCamera.Add(RegisteringMechanismComponent);
        }
        MechanismColorCamera[RegisteringMechanismComponent].Add(colorCameraInfo);
        ColorCameras.Add(colorCameraInfo.id) = nullptr;
    }
}

void FVortexRuntimeModule::UnassociateColorCameraFromUnregisteringComponent(uint64_t ColorCameraId)
{
    if (!RegisteringMechanismComponent.IsEmpty())
    {
        if (ColorCameras.Contains(ColorCameraId))
        {
            ColorCameras.Remove(ColorCameraId);
        }
        if (MechanismColorCamera.Contains(RegisteringMechanismComponent))
        {
            for (int32 i = 0; i < MechanismColorCamera[RegisteringMechanismComponent].Num(); ++i)
            {
                if (MechanismColorCamera[RegisteringMechanismComponent][i].id == ColorCameraId)
                {
                    MechanismColorCamera[RegisteringMechanismComponent].RemoveAt(i);
                    break;
                }
            }
            if (MechanismColorCamera[RegisteringMechanismComponent].Num() == 0)
            {
                MechanismColorCamera.Remove(RegisteringMechanismComponent);
            }
        }
    }
}

void FVortexRuntimeModule::BeginPlay(UMechanismComponent* Component)
{
    if (MechanismLidars.Contains(Component->LoadedMechanismKey))
    {
        const TArray<GraphicsLidarInfo>& mechanismLidars = MechanismLidars[Component->LoadedMechanismKey];
        for (const auto& lidar : mechanismLidars)
        {
            if (!Lidars[lidar.id])
            {
                FTransform pose = VortexIntegrationUtilities::ConvertTransform(lidar.translation, lidar.rotationQuaternion);
                Lidars[lidar.id] = Component->GetWorld()->SpawnActorDeferred<AVortexLidarActor>(AVortexLidarActor::StaticClass(), pose);
                if (Lidars[lidar.id])
                {
                    auto* lidarActorComponent = static_cast<UVortexLidarActorComponent*>(Lidars[lidar.id]->GetRootComponent());
                    lidarActorComponent->NumberOfChannels = lidar.numberOfChannels;
                    lidarActorComponent->Range = VortexIntegrationUtilities::ConvertLengthToUnreal(lidar.range);
                    lidarActorComponent->HorizontalResolution = lidar.horizontalResolution;
                    lidarActorComponent->HorizontalRotationFrequency = lidar.horizontalRotationFrequency;
                    lidarActorComponent->HorizontalFovStart = FMath::RadiansToDegrees(lidar.horizontalFovStart);
                    lidarActorComponent->HorizontalFovLength = FMath::RadiansToDegrees(lidar.horizontalFovLength);
                    lidarActorComponent->VerticalFovUpper = FMath::RadiansToDegrees(lidar.verticalFovUpper);
                    lidarActorComponent->VerticalFovLower = FMath::RadiansToDegrees(lidar.verticalFovLower);
                    lidarActorComponent->OutputAsDistanceField = lidar.outputAsDistanceField;
                    lidarActorComponent->PointCloudVisualization = lidar.pointCloudVisualization;
                    lidarActorComponent->SetWorldTransform(pose);
                    UGameplayStatics::FinishSpawningActor(Lidars[lidar.id], pose);
                }
            }
        }
    }

    if (MechanismDepthCamera.Contains(Component->LoadedMechanismKey))
    {
        const TArray<GraphicsDepthCameraInfo>& mechanismDepthCameras = MechanismDepthCamera[Component->LoadedMechanismKey];
        for (const auto& depthCamera : mechanismDepthCameras)
        {
            if (!DepthCameras[depthCamera.id])
            {
                FTransform pose = VortexIntegrationUtilities::ConvertTransform(depthCamera.translation, depthCamera.rotationQuaternion);
                DepthCameras[depthCamera.id] = Component->GetWorld()->SpawnActor<AVortexDepthCameraActor>(AVortexDepthCameraActor::StaticClass(), pose);
                if (DepthCameras[depthCamera.id])
                {
                    auto* depthCameraActorComponent = static_cast<UVortexDepthCameraActorComponent*>(DepthCameras[depthCamera.id]->GetRootComponent());
                    depthCameraActorComponent->Width = depthCamera.width;
                    depthCameraActorComponent->Height = depthCamera.height;
                    depthCameraActorComponent->FOV = depthCamera.fov;
                    depthCameraActorComponent->Framerate = depthCamera.framerate;
                    depthCameraActorComponent->ZMax = VortexIntegrationUtilities::ConvertLengthToUnreal(depthCamera.zMax);
                    depthCameraActorComponent->SetWorldTransform(pose);
                }
            }
        }
    }

    if (MechanismColorCamera.Contains(Component->LoadedMechanismKey))
    {
        const TArray<GraphicsColorCameraInfo>& mechanismColorCameras = MechanismColorCamera[Component->LoadedMechanismKey];
        for (const auto& colorCamera : mechanismColorCameras)
        {
            if (!ColorCameras[colorCamera.id])
            {
                FTransform pose = VortexIntegrationUtilities::ConvertTransform(colorCamera.translation, colorCamera.rotationQuaternion);
                ColorCameras[colorCamera.id] = Component->GetWorld()->SpawnActor<AVortexColorCameraActor>(AVortexColorCameraActor::StaticClass(), pose);
                if (ColorCameras[colorCamera.id])
                {
                    auto* colorCameraActorComponent = static_cast<UVortexColorCameraActorComponent*>(ColorCameras[colorCamera.id]->GetRootComponent());
                    colorCameraActorComponent->Width = colorCamera.width;
                    colorCameraActorComponent->Height = colorCamera.height;
                    colorCameraActorComponent->FOV = colorCamera.fov;
                    colorCameraActorComponent->Framerate = colorCamera.framerate;
                    colorCameraActorComponent->SetWorldTransform(pose);
                }
            }
        }
    }
}

void FVortexRuntimeModule::EndPlay(UMechanismComponent* Component)
{
    if (MechanismLidars.Contains(Component->LoadedMechanismKey))
    {
        const TArray<GraphicsLidarInfo>& mechanismLidars = MechanismLidars[Component->LoadedMechanismKey];
        for (const auto& lidar : mechanismLidars)
        {
            if (Lidars[lidar.id])
            {
                Component->GetWorld()->DestroyActor(static_cast<AActor*>(Lidars[lidar.id]));
                Lidars[lidar.id] = nullptr;
            }
        }
    }

    if (MechanismDepthCamera.Contains(Component->LoadedMechanismKey))
    {
        const TArray<GraphicsDepthCameraInfo>& mechanismDepthCameras = MechanismDepthCamera[Component->LoadedMechanismKey];
        for (const auto& depthCamera : mechanismDepthCameras)
        {
            if (DepthCameras[depthCamera.id])
            {
                Component->GetWorld()->DestroyActor(static_cast<AActor*>(DepthCameras[depthCamera.id]));
                DepthCameras[depthCamera.id] = nullptr;
            }
        }
    }

    if (MechanismColorCamera.Contains(Component->LoadedMechanismKey))
    {
        const TArray<GraphicsColorCameraInfo>& mechanismColorCameras = MechanismColorCamera[Component->LoadedMechanismKey];
        for (const auto& colorCamera : mechanismColorCameras)
        {
            if (ColorCameras[colorCamera.id])
            {
                Component->GetWorld()->DestroyActor(static_cast<AActor*>(ColorCameras[colorCamera.id]));
                ColorCameras[colorCamera.id] = nullptr;
            }
        }
    }
}

void FVortexRuntimeModule::UpdateLidar(GraphicsLidarInfo& lidarInfo)
{
    if (Lidars[lidarInfo.id])
    {
        auto* lLidarComponent = static_cast<UVortexLidarActorComponent*>(Lidars[lidarInfo.id]->GetRootComponent());
        FVector translation = VortexIntegrationUtilities::ConvertTranslation(lidarInfo.translation);
        FQuat rotation = VortexIntegrationUtilities::ConvertRotation(lidarInfo.rotationQuaternion);
        FTransform pose = FTransform(rotation, translation);
        Lidars[lidarInfo.id]->SetActorTransform(pose);
        lLidarComponent->PointCloudVisualization = lidarInfo.pointCloudVisualization;
        lLidarComponent->visualizePointCloud(lidarInfo.pointCloud);
        lidarInfo.pointCloud = lLidarComponent->takePointCloud(lidarInfo.pointCloudSize);
    }
    else 
    {
        lidarInfo.pointCloud = nullptr;
        lidarInfo.pointCloudSize = 0;
    }
}

void FVortexRuntimeModule::UpdateDepthCamera(GraphicsDepthCameraInfo& depthCameraInfo)
{
    if (DepthCameras[depthCameraInfo.id])
    {
        auto* lDepthCameraComponent = static_cast<UVortexDepthCameraActorComponent*>(DepthCameras[depthCameraInfo.id]->GetRootComponent());
        FVector translation = VortexIntegrationUtilities::ConvertTranslation(depthCameraInfo.translation);
        FQuat rotation = VortexIntegrationUtilities::ConvertRotation(depthCameraInfo.rotationQuaternion);
        FTransform pose = FTransform(rotation, translation);
        DepthCameras[depthCameraInfo.id]->SetActorTransform(pose);
        depthCameraInfo.depthImagesPixels = lDepthCameraComponent->takeSnapshot(depthCameraInfo.depthImageCount);
        depthCameraInfo.depthImageCount /= (depthCameraInfo.width * depthCameraInfo.height);
    }
    else
    {
        depthCameraInfo.depthImagesPixels = nullptr;
        depthCameraInfo.depthImageCount = 0;
    }
}

void FVortexRuntimeModule::UpdateColorCamera(GraphicsColorCameraInfo& colorCameraInfo)
{
    if (ColorCameras[colorCameraInfo.id])
    {
        auto* lColorCameraComponent = static_cast<UVortexColorCameraActorComponent*>(ColorCameras[colorCameraInfo.id]->GetRootComponent());
        FVector translation = VortexIntegrationUtilities::ConvertTranslation(colorCameraInfo.translation);
        FQuat rotation = VortexIntegrationUtilities::ConvertRotation(colorCameraInfo.rotationQuaternion);
        FTransform pose = FTransform(rotation, translation);
        ColorCameras[colorCameraInfo.id]->SetActorTransform(pose);
        colorCameraInfo.colorImagesPixels = lColorCameraComponent->takeSnapshot(colorCameraInfo.colorImagesCount);
        colorCameraInfo.colorImagesCount /= (colorCameraInfo.width * colorCameraInfo.height * 3);
    }
    else
    {
        colorCameraInfo.colorImagesPixels = nullptr;
        colorCameraInfo.colorImagesCount = 0;
    }
}

void FVortexRuntimeModule::StartupModule()
{
    bool HasError = true;

    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    // Get the base directory of this plugin
    FString baseDir = IPluginManager::Get().FindPlugin("VortexPlugin")->GetBaseDir();

    // Vortex is not loaded yet, because we explicitly said to Unreal (see VortexStudio.Build.cs) that "VortexIntegration.dll" should be delay loaded.
    // Here, we dynamically load the "VortexIntegration.dll" library from the proper path.
    FString vortexDir = GetVortexStudioInstallationPath(baseDir);

    UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::StartupModule(): Unreal Engine will try to load Vortex Studio from \"%s\"."), *vortexDir);

    VortexStudioBinDir.Path = !vortexDir.IsEmpty() ? FPaths::Combine(vortexDir, TEXT("bin")) : "";
    if (FPaths::DirectoryExists(VortexStudioBinDir.Path))
    {
        FPlatformProcess::AddDllDirectory(*VortexStudioBinDir.Path);
        FString LibraryPath = FPaths::Combine(*VortexStudioBinDir.Path, TEXT("VortexIntegration.dll"));

        // Vortex Studio's binaries are delay loaded. For some debuggers, it is necessary to attach to the Unreal process while the dlls are delay loaded if we want to be able to debug.
        // Otherwise, some symbols are not loaded in the debugger.
        if (!FPlatformMisc::GetEnvironmentVariable(PromptForDebuggerEnvVar).IsEmpty())
        {
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("VortexRuntime", "Unreal Engine is about to load Vortex Studio. Please attach your debugger."));
        }

        VortexIntegrationHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;
    }
    
    if (VortexIntegrationHandle != nullptr)
    {
        FString applicationSetupAbsPath;
        FString materialTableAbsPath;
        FString dataProviderAbsPath;
        bool enableLandscapeCollision = false;
        bool enableMeshSimpleCollision = false;
        bool enableMeshComplexCollision = false;
        double terrainTileSizeXY = 0.0;
        double terrainSafetyBandSize = 0.0;
        double terrainLookAheadTime = 0.0;

        bool applicationParamsOK = ResolveVortexApplicationSettings(vortexDir, applicationSetupAbsPath, materialTableAbsPath, dataProviderAbsPath,
                                                                    enableLandscapeCollision, enableMeshSimpleCollision, enableMeshComplexCollision,
                                                                    terrainTileSizeXY, terrainSafetyBandSize, terrainLookAheadTime);

        VortexTerrainProviderInfo terrainProviderInfo;
        VortexTerrainProviderInfoInit(&terrainProviderInfo);

        terrainProviderInfo.terrainProviderTileSizeUV = terrainTileSizeXY;
        terrainProviderInfo.terrainProviderSafetyBandSize = terrainSafetyBandSize;
        terrainProviderInfo.terrainProviderLookAheadTime = terrainLookAheadTime;

        if (enableLandscapeCollision || enableMeshSimpleCollision || enableMeshComplexCollision)
        {
            Terrain = new FVortexTerrain(enableLandscapeCollision, enableMeshSimpleCollision, enableMeshComplexCollision);
            terrainProviderInfo.terrainProvider = Terrain;
            terrainProviderInfo.terrainProviderQuery = ::TerrainProviderQuery;
            terrainProviderInfo.terrainProviderPostQuery = ::TerrainProviderPostQuery;
            terrainProviderInfo.terrainProviderOnDestroy = ::TerrainProviderOnDestroy;
        }

        // Create the application from the resolved settings only if everything is valid
        if (applicationParamsOK && VortexCreateApplication(TCHAR_TO_UTF8(*applicationSetupAbsPath), TCHAR_TO_UTF8(*materialTableAbsPath), TCHAR_TO_UTF8(*dataProviderAbsPath), TCHAR_TO_UTF8(*FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir())), &terrainProviderInfo))
        {
            bool success = true;

            if (!VortexHasValidLicense())
            {
                EAppReturnType::Type returnType = FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::No, NSLOCTEXT("VortexPluginText", "LicenseErrorMessage", "Vortex Studio is not licensed on this computer. Would you like to open the Vortex License Manager to fix the issue?"));
                if (returnType == EAppReturnType::Yes)
                {
                    FPlatformProcess::CreateProc(*FPaths::Combine(*VortexStudioBinDir.Path, TEXT("VortexLicenseManagerGUI.exe")), nullptr, true, false, false, nullptr, 0, nullptr, nullptr);
                }

                UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): Vortex Studio is not licensed on this computer."));
                success = false;
            }

            // Validate some parameter values that we want to enforce
            if (VortexGetSynchronizationMode() != kVortexSyncNone)
            {
                UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): Vortex Application has an incompatible synchronization mode. Please set sync mode to none in setup document."));
                success = false;
            }

            if (VortexGetApplicationMode() != kVortexModeEditing)
            {
                UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): Vortex Application is not starting in Editing mode. Please set application mode to Editing mode in setup document."));
                success = false;
            }

            if (success)
            {

                ::GraphicsIntegrationCallbacks callbacks;
                callbacks.lidarNotification = VortexLidarCallback;
                callbacks.depthCameraNotification = VortexDepthCameraCallback;
                callbacks.colorCameraNotification = VortexColorCameraCallback;
                ::GraphicsRegisterCallbacks(&callbacks);
                ValidateUnrealAndVortexFrameRates();

                HasError = false;
                UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::StartupModule(): VxApplication is valid"));
                VortexPeriod = VortexGetSimulationFrameRate() >= 0.0 ? 1.0 / VortexGetSimulationFrameRate() : 0.0;
                TickDelegate = FTickerDelegate::CreateRaw(this, &FVortexRuntimeModule::Tick);
                TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);

                // Get the total number of Vortex materials
                std::vector<VortexMaterial> VortexMaterials;
                std::uint32_t NumVortexMaterials = 0;
                VortexGetAvailableMaterials(VortexMaterials.data(), &NumVortexMaterials);

                // Now, get all Vortex materials
                VortexMaterials.resize(NumVortexMaterials);
                VortexGetAvailableMaterials(VortexMaterials.data(), &NumVortexMaterials);
                for (size_t IndexMaterial = 0; IndexMaterial < NumVortexMaterials; ++IndexMaterial)
                {
                    AvailableVortexMaterials.Add(VortexMaterials[IndexMaterial].name);
                }
                AvailableVortexMaterials.Sort(SortStrings);

                // Validate material mapping
                TSet<FString> InvalidVortexMaterials;
                const UVortexSettings* settings = GetDefault<UVortexSettings>();
                for (auto& MaterialMapping : settings->MaterialMappings)
                {
                    if (!MaterialMapping.VortexMaterialName.IsEmpty() && AvailableVortexMaterials.Find(MaterialMapping.VortexMaterialName) == nullptr)
                    {
                        // Here, we output the error, but we don't prevent Vortex from starting properly
                        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): Vortex Material \"%s\" does not exist in the provided material table."), *MaterialMapping.VortexMaterialName);
                        InvalidVortexMaterials.Add(MaterialMapping.VortexMaterialName);
                    }
                }

#if WITH_EDITOR
                if (InvalidVortexMaterials.Num() > 0)
                {
                    InvalidVortexMaterials.Sort(SortStrings);
                    FString FormattedListOfMaterials = "";
                    for (auto& MaterialName : InvalidVortexMaterials)
                    {
                        FormattedListOfMaterials += "- ";
                        FormattedListOfMaterials += MaterialName;
                        FormattedListOfMaterials += "\n";
                    }

                    FMessageDialog::Open(EAppMsgType::Ok,
                                         FText::Format(LOCTEXT("Error_InvalidMaterialName", "One or many Vortex Material names are not valid. Here is the list:\n\n{0}\n\nPlease fix this in the Project Settings."),
                                         FText::FromString(FormattedListOfMaterials)));
                }

                // Bind events on PIE (Play-in-Editor) end
                PieEndedEventBinding = FEditorDelegates::PrePIEEnded.AddRaw(this, &FVortexRuntimeModule::OnPieEnded);
                PieSingleStepEventBinding = FEditorDelegates::SingleStepPIE.AddRaw(this, &FVortexRuntimeModule::OnPieSingleStep);
                FEditorDelegates::OnShutdownPostPackagesSaved.AddStatic([]() {
                    // Closing and saving while playing doesn't call UVortexApplicationBlueprintLib::StopSimulation
                    // Yet Tick() still gets called on a dangling GEngine
                    UVortexApplicationBlueprintLib::StopSimulation(nullptr);
                });

                bLastUseFixedFrameRate = GEngine->bUseFixedFrameRate;
                LastFixedFrameRate = GEngine->FixedFrameRate;
#endif
            }
            else
            {
                UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): VxApplication is NOT valid"));
            }
        }
        else
        {
            UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): VxApplication is NOT valid"));
        }
    }
    else
    {
        auto ExpectedVersionMajorAndMinor = GetExpectedVortexVersion();

        std::string expectedVersionFullName = "Vortex Studio ";
        expectedVersionFullName += VORTEX_INTEGRATION_MARKETING_NAME;
        expectedVersionFullName += " (";
        expectedVersionFullName += ExpectedVersionMajorAndMinor.first;
        expectedVersionFullName += ".";
        expectedVersionFullName += ExpectedVersionMajorAndMinor.second;
        expectedVersionFullName += ")";
        
        FMessageDialog::Open(EAppMsgType::Ok,
                             FText::Format(LOCTEXT("VortexRuntime", "{0} is not installed on this computer.\n\nPlease install this product to use the Vortex Studio integration in Unreal."),
                                           FText::FromString(UTF8_TO_TCHAR(expectedVersionFullName.c_str()))));

        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::StartupModule(): Unable to find %s."), UTF8_TO_TCHAR(expectedVersionFullName.c_str()));
    }

    // If an error happened, we shut down Vortex
    if (HasError)
    {
        ShutdownVortex();
    }

    IModuleInterface::StartupModule();
}

bool FVortexRuntimeModule::Tick(float deltaTime)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return true;
    }

#if WITH_EDITOR
    if (GEngine->bUseFixedFrameRate != bLastUseFixedFrameRate || LastFixedFrameRate != GEngine->FixedFrameRate)
    {
        ValidateUnrealAndVortexFrameRates();
        bLastUseFixedFrameRate = GEngine->bUseFixedFrameRate;
        LastFixedFrameRate = GEngine->FixedFrameRate;
    }
#endif
    
    // Flush deferred Mechanism unload pool
    for (auto& pair : MechanismPool)
    {
        UnloadAsset(pair.Value);
    }
    MechanismPool.Empty();
    SCOPE_CYCLE_COUNTER(STAT_ModuleTick);
    // Paused/Resume simulation BEFORE updating the application, so it takes effect right away (it takes one step to be active).
    if (GetCurrentWorld() != nullptr && GetCurrentWorld()->IsPaused() != VortexIsPaused())
    {
        VortexPause(GetCurrentWorld()->IsPaused());
    }

    AccumulatedTime += deltaTime;
    // If the accummulated time is too big, we don't try to catch up
    if (AccumulatedTime >= OutstandingPeriodFactor * VortexPeriod)
    {
        AccumulatedTime = VortexPeriod;
    }
    
    while (AccumulatedTime >= VortexPeriod)
    {
        FlushPersistentDebugLines(GetCurrentWorld());
        VortexUpdateApplication();
        AccumulatedTime -= VortexPeriod;
    }

    return true;
}

UWorld* FVortexRuntimeModule::GetCurrentWorld() const
{
    if (!CurrentWorldContext || !GEngine)
    {
        return nullptr;
    }

    return GEngine->GetWorldFromContextObject(CurrentWorldContext, EGetWorldErrorMode::LogAndReturnNull);
}

const TArray<AActor*>& FVortexRuntimeModule::GetMechanismActors() const
{
    return MechanismActors;
}

FString FVortexRuntimeModule::GetVortexMaterialFromPhysicalMaterial(UPhysicalMaterial* PhysicalMaterial) const
{
    FString VortexMaterialName = "";
    const UVortexSettings* settings = GetDefault<UVortexSettings>();
    for (auto& MaterialMapping : settings->MaterialMappings)
    {
        if (MaterialMapping.PhysicalMaterial == PhysicalMaterial)
        {
            VortexMaterialName = MaterialMapping.VortexMaterialName;
            break;
        }
    }

    return VortexMaterialName;
}

void FVortexRuntimeModule::ShutdownModule()
{
    IModuleInterface::ShutdownModule();
    ShutdownVortex();
}

bool FVortexRuntimeModule::ResolveVortexApplicationSettings(const FString& VortexDir, FString& ApplicationSetupAbsPath, FString& MaterialTableAbsPath, FString& DataProviderAbsPath,
                                                            bool& EnableLandscapeCollision, bool& EnableMeshesSimpleCollision, bool& EnableMeshesComplexCollision,
                                                            double& TerrainTileSizeXY, double& TerrainSafetyBandSize, double& TerrainLookAheadTime) const
{
    bool applicationParamsOK = true;
    UVortexSettings* settings = GetMutableDefault<UVortexSettings>();

    EnableLandscapeCollision = settings->EnableLandscapeCollisionDetection;
    EnableMeshesSimpleCollision = settings->EnableMeshSimpleCollisionDetection;
    EnableMeshesComplexCollision = settings->EnableMeshComplexCollisionDetection;
    TerrainTileSizeXY = settings->TerrainPagingTileSizeXY;
    TerrainLookAheadTime = settings->TerrainPagingLookAheadTime;
    TerrainSafetyBandSize = settings->TerrainPagingSafetyBandSize;

    if (settings->UseDefaultApplicationSetup)
    {
        ApplicationSetupAbsPath = FPaths::Combine(VortexDir, TEXT("resources/config/VortexIntegration.vxc"));
    }
    else
    {
        ApplicationSetupAbsPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), settings->ApplicationSetup.FilePath);
    }

    if (FPaths::FileExists(ApplicationSetupAbsPath))
    {
        UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::ResolveVortexApplicationSettings(): Using Application Setup \"%s\"."), *ApplicationSetupAbsPath);
    }
    else
    {
        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::ResolveVortexApplicationSettings(): Application Setup \"%s\" does not exist."), *ApplicationSetupAbsPath);
        applicationParamsOK = false;
    }

    if (settings->UseDefaultMaterialTable)
    {
        MaterialTableAbsPath = FPaths::Combine(VortexDir, TEXT("resources/DynamicsMaterials/default.vxmaterials"));
    }
    else
    {
        MaterialTableAbsPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), settings->MaterialTableFilepath.FilePath);
    }

    if (FPaths::FileExists(MaterialTableAbsPath))
    {
        UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::ResolveVortexApplicationSettings(): Using Material Table \"%s\"."), *MaterialTableAbsPath);
    }
    else
    {
        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::ResolveVortexApplicationSettings(): Material Table \"%s\" does not exist."), *MaterialTableAbsPath);
        applicationParamsOK = false;
    }

    DataProviderAbsPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), settings->DataProviderDirectoryPath.Path);
    if (FPaths::DirectoryExists(DataProviderAbsPath))
    {
        UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::ResolveVortexApplicationSettings(): Using Data Provider's Path \"%s\"."), *DataProviderAbsPath);
    }
    else
    {
        UE_LOG(LogVortex, Error, TEXT("FVortexRuntimeModule::ResolveVortexApplicationSettings(): Data Provider's Path \"%s\" does not exist."), *DataProviderAbsPath);
        applicationParamsOK = false;
    }

    return applicationParamsOK;
}

void FVortexRuntimeModule::ShutdownVortex()
{
    UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::ShutdownModule()"));

#if WITH_EDITOR
    FEditorDelegates::PrePIEEnded.Remove(PieEndedEventBinding);
    FEditorDelegates::SingleStepPIE.Remove(PieSingleStepEventBinding);
#endif

    FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

    if (IsIntegrationLoaded())
    {
        VortexDestroyApplication();

        VortexIntegrationHandle = nullptr;
    }

    delete Terrain;
    Terrain = nullptr;
}

#if WITH_EDITOR

void FVortexRuntimeModule::OnPieEnded(bool)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return;
    }

    UWorld* pieWorld = GEditor->GetPIEWorldContext()->World();
#if WITH_EDITORONLY_DATA
    bool isGameWorld = !IsRunningCommandlet() && pieWorld->IsGameWorld();
#endif 
    if (isGameWorld && pieWorld != nullptr)
    {
        if (GetCurrentWorld() == pieWorld)
        {
            // Game ends. Make sure the pause flag is reset for next run.
            VortexPause(false);
        }
    }
}

void FVortexRuntimeModule::OnPieSingleStep(bool)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return;
    }

    VortexStepOnce();
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVortexRuntimeModule, VortexRuntime)