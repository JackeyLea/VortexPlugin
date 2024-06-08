#include "VortexColorCameraActor.h"
#include "Components/SceneComponent.h"

#include "DrawDebugHelpers.h"
#include "Core.h"

AVortexColorCameraActor::AVortexColorCameraActor()
{
    PrimaryActorTick.bCanEverTick = false;
   
    RootComponent = CreateDefaultSubobject<UVortexColorCameraActorComponent>(TEXT("ColorCameraComponent"));
}
