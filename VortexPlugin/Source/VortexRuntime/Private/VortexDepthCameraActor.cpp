#include "VortexDepthCameraActor.h"
#include "Components/SceneComponent.h"

#include "DrawDebugHelpers.h"
#include "Core.h"

AVortexDepthCameraActor::AVortexDepthCameraActor()
{
    PrimaryActorTick.bCanEverTick = false;
   
    RootComponent = CreateDefaultSubobject<UVortexDepthCameraActorComponent>(TEXT("DepthCameraComponent"));
}
