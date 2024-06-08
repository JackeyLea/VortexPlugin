#include "..\Public\MechanismComponentAssetBroker.h"
#include <VortexMechanism.h>
#include <MechanismComponent.h>

FMechanismComponentAssetBroker::FMechanismComponentAssetBroker()
{
}

FMechanismComponentAssetBroker::~FMechanismComponentAssetBroker()
{
}

bool FMechanismComponentAssetBroker::AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset)
{
    UMechanismComponent* component = Cast<UMechanismComponent>(InComponent);
    UVortexMechanism* asset = Cast<UVortexMechanism>(InAsset);
    if (component && asset)
    {
        component->VortexMechanism = asset;
        return true;
    }
    return false;
}

UClass* FMechanismComponentAssetBroker::GetSupportedAssetClass()
{
    return UVortexMechanism::StaticClass();
}

UObject* FMechanismComponentAssetBroker::GetAssetFromComponent(UActorComponent* InComponent)
{
    UMechanismComponent* component = Cast<UMechanismComponent>(InComponent);
    if (component)
    {
        return component->VortexMechanism;
    }
    return nullptr;
}
