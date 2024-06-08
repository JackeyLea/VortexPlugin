#pragma once
//Copyright(c) 2019 CM Labs Simulations Inc. All rights reserved.
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//the sample code software and associated documentation files (the "Software"), to deal with
//the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
//of the Software, and to permit persons to whom the Software is furnished to
//do so, subject to the following conditions :
//
//Redistributions of source code must retain the above copyright notice,
//this list of conditions and the following disclaimers.
//Redistributions in binary form must reproduce the above copyright notice,
//this list of conditions and the following disclaimers in the documentation
//and/or other materials provided with the distribution.
//Neither the names of CM Labs or Vortex Studio
//nor the names of its contributors may be used to endorse or promote products
//derived from this Software without specific prior written permission.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
//SOFTWARE.
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"
#include "VortexIntegration/VortexIntegration.h"
#include "VortexIntegration/VortexIntegrationTypes.h"
#include "Runtime/Engine/Public/TimerManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVortex, Log, All);

class UMechanismComponent;
class AVortexLidarActor;
class UVortexLidarActorComponent;
class UVortexApplicationBlueprintLib;
class FVortexTerrain;
DECLARE_STATS_GROUP(TEXT("VortexRuntimeModule"), STATGROUP_VortexRuntimeModule, STATCAT_Advanced);
/// Runtime module for Vortex Studio integration
class FVortexRuntimeModule
    : public IModuleInterface
{
public:

    FVortexRuntimeModule();

    //
    // Singleton-like access to this module's interface.  This is just for convenience!
    // Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
    //
    // @return Returns singleton instance, loading the module on demand if needed
    //
    static inline FVortexRuntimeModule& Get() { return FModuleManager::LoadModuleChecked<FVortexRuntimeModule>("VortexRuntime"); }

    //
    // Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
    //
    // @return True if the module is loaded and ready to use
    //
    static inline bool IsAvailable() { return FModuleManager::Get().IsModuleLoaded("VortexRuntime"); }

    //
    // Checks to see if the Vortex integration is loaded and ready.
    //
    // @return True if the Vortex integration is loaded and ready to use
    //
    static inline bool IsIntegrationLoaded() { return Get().VortexIntegrationHandle != nullptr; }

    //
    // Get the bin folder of Vortex Studio
    //
    static inline FString GetVortexStudioBinDir() { return Get().VortexStudioBinDir.Path; }

    //
    // Register mechanism component
    // This will either load the Vortex Mechanism UAsset from disk or shared an already loaded strictly equivalent Mechanism with existing components
    //
    void RegisterComponent(UMechanismComponent* Component);

    void AssociateLidarToRegisteringComponent(const GraphicsLidarInfo& lidarInfo);

    void UnassociateLidarFromUnregisteringComponent(uint64_t LidarId);

    void AssociateDepthCameraToRegisteringComponent(const GraphicsDepthCameraInfo& depthCameraInfo);

    void UnassociateDepthCameraFromUnregisteringComponent(uint64_t DepthCameraId);

    void AssociateColorCameraToRegisteringComponent(const GraphicsColorCameraInfo& colorCameraInfo);

    void UnassociateColorCameraFromUnregisteringComponent(uint64_t ColorCameraId);

    void BeginPlay(UMechanismComponent* Component);

    void EndPlay(UMechanismComponent* Component);

    void UpdateLidar(GraphicsLidarInfo& lidarInfo);

    void UpdateDepthCamera(GraphicsDepthCameraInfo& depthCameraInfo);

    void UpdateColorCamera(GraphicsColorCameraInfo& colorCameraInfo);
    //
    // Unregister mechanism component
    // Remove a reference to the Mechanism
    // If the component was the only reference to it this will either unload the Vortex Mechanism UAsset immediately or put it back in a short-lived pool for reuse
    //
    void UnregisterComponent(UMechanismComponent* Component, bool bImmediately = false);

    //
    // Clear all references for mechanism components corresponding to a given Vortex Mechanism
    //
    void UnregisterAllComponents(FString LoadedMechanismKey);


    // IModuleInterface implementation
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    bool Tick(float);

    UWorld* GetCurrentWorld() const;
    const TArray<AActor*>& GetMechanismActors() const;

    FString GetVortexMaterialFromPhysicalMaterial(UPhysicalMaterial* PhysicalMaterial) const;

    const TSet<FString>& GetAvailableVortexMaterials() const { return AvailableVortexMaterials; }

private:
    friend class UVortexApplicationBlueprintLib;

    /// This internal function crunshes the VortexSettings and returns the resulting parameters to pass for creation of the Vortex Application.
    ///
    /// @param[in]  VortexDir                    The root directory of the Vortex Studio installation
    /// @param[out] ApplicationSetupAbsPath      The absolute path of the application setup file to use.
    /// @param[out] MaterialTableAbsPath         The absolute path of the material table file to use.
    /// @param[out] DataProviderAbsPath          The absolute path of the data provider's path to use.
    /// @param[out] EnableLandscapeCollision     Enable collision with Landscape.
    /// @param[out] EnableMeshesSimpleCollision  Enable collision with meshes' simple collisions (when enabled).
    /// @param[out] EnableMeshesComplexCollision Enable collision with meshes' complex collisions (when enabled).
    ///
    /// @return True if all parameters are valid and can be used to create a Vortex Application.
    ///
    bool ResolveVortexApplicationSettings(const FString& VortexDir, FString& ApplicationSetupAbsPath, FString& MaterialTableAbsPath, FString& DataProviderAbsPath,
                                          bool& EnableLandscapeCollision, bool& EnableMeshesSimpleCollision, bool& EnableMeshesComplexCollision,
                                          double& TerrainTileSizeXY, double& TerrainSafetyBandSize, double& TerrainLookAheadTime) const;

    /// Callbacks when worlds are created or destroyed
    ///
    void OnWorldCreated(UWorld* NewWorld);
    void OnWorldDestroyed(UWorld* NewWorld);

    void ShutdownVortex();

    UObject* CurrentWorldContext;

#if WITH_EDITOR
    FDelegateHandle PieEndedEventBinding;
    FDelegateHandle PieSingleStepEventBinding;

    void OnPieEnded(bool bIsSimulating);
    void OnPieSingleStep(bool bIsSimulating);

    bool bLastUseFixedFrameRate;
    float LastFixedFrameRate;
#endif

    /// Handle to the VortexIntegration dll
    void* VortexIntegrationHandle;
    double AccumulatedTime;
    double VortexPeriod;
    FTickerDelegate TickDelegate;
    FDelegateHandle TickDelegateHandle;
    FDirectoryPath VortexStudioBinDir;

    /// Registered components
    TArray<AActor*> MechanismActors;
    TMultiMap<FString, UMechanismComponent*> MechanismComponents;

    /// Short lived pool of freed mechanisms available for reuse
    TMultiMap<FString, VortexObjectHandle> MechanismPool;

    FString RegisteringMechanismComponent;
    TMap<FString, TArray<GraphicsLidarInfo>> MechanismLidars;
    TMap<uint64_t, AActor*> Lidars;
    TMap<FString, TArray<GraphicsDepthCameraInfo>> MechanismDepthCamera;
    TMap<uint64_t, AActor*> DepthCameras;
    TMap<FString, TArray<GraphicsColorCameraInfo>> MechanismColorCamera;
    TMap<uint64_t, AActor*> ColorCameras;

    FVortexTerrain* Terrain;

    /// A set of all currently available Vortex Materials
    TSet<FString> AvailableVortexMaterials;
};
