#include "VortexLidarActor.h"
#include "Components/SceneComponent.h"

#include "DrawDebugHelpers.h"
#include "Core.h"

AVortexLidarActor::AVortexLidarActor()
{
    PrimaryActorTick.bCanEverTick = false;
   
    RootComponent = CreateDefaultSubobject<UVortexLidarActorComponent>(TEXT("LidarComponent"));
}
