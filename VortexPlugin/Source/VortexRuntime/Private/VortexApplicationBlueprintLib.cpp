#include "VortexApplicationBlueprintLib.h"
#include "VortexRuntime.h"

#include "VortexIntegration/VortexIntegration.h"

void UVortexApplicationBlueprintLib::StartSimulation(UObject* WorldContextObject)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return;
    }

    FVortexRuntimeModule::Get().CurrentWorldContext = WorldContextObject;
    UE_LOG(LogVortex, Display, TEXT("UVortexApplicationBlueprintLib::StartSimulation(): Starting simulation."));
    VortexSetApplicationMode(kVortexModeSimulating, true);
}

void UVortexApplicationBlueprintLib::StopSimulation(UObject*)
{
    if (!FVortexRuntimeModule::IsIntegrationLoaded())
    {
        return;
    }

    VortexSetApplicationMode(kVortexModeEditing, true);
    VortexResetSimulationTime();
    UE_LOG(LogVortex, Display, TEXT("UVortexApplicationBlueprintLib::StopSimulation(): Stopping simulation."));
    FVortexRuntimeModule::Get().CurrentWorldContext = nullptr;
}