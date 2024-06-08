#include "VortexDepthCameraActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Core.h"
#include "VortexIntegrationUtilities.h"
#include "RenderCore/Public/RenderingThread.h"

// Sets default values
UVortexDepthCameraActorComponent::UVortexDepthCameraActorComponent()
{
    CaptureComponent = nullptr;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
    mFrontReadback = &mReadbacks[0];
    mBackReadback = &mReadbacks[1];
}

void UVortexDepthCameraActorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    if (CaptureComponent->TextureTarget->SizeX != Width || CaptureComponent->TextureTarget->SizeY != Height)
    {
        CaptureComponent->TextureTarget->SizeX = Width;
        CaptureComponent->TextureTarget->SizeY = Height;
        CaptureComponent->TextureTarget->UpdateResource();
        mBackReadback->Reserve(Width * Height * 3);
        mFrontReadback->Reserve(Width * Height * 3);
    }
    
    // Vortex produces the vertical FOV in radians. Unreal consumes the horizontal FOV in degrees.
    const float aspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    const float unrealFOV = FMath::RadiansToDegrees(2.0f * FMath::Atan(FMath::Tan(FOV * 0.5f) * aspectRatio));

    if (CaptureComponent->FOVAngle != unrealFOV || CaptureComponent->MaxViewDistanceOverride != ZMax)
    {
        CaptureComponent->FOVAngle = unrealFOV;
        CaptureComponent->MaxViewDistanceOverride = ZMax;
    }
    
    mTimeAccumulator += DeltaTime;
    while (mTimeAccumulator >= 1.f / static_cast<float>(Framerate))
    {
        CaptureComponent->CaptureScene();
        ENQUEUE_RENDER_COMMAND(FEnqueueCaptureDownloadRequest)(
            [this](FRHICommandListImmediate& RHICmdList)
            {
                TUniquePtr<FRHIGPUTextureReadback> Readback = MakeUnique<FRHIGPUTextureReadback>(NAME_None);
                Readback->EnqueueCopy(RHICmdList,
                                      CaptureComponent->TextureTarget->Resource->GetTexture2DRHI());
                mRHITextureReadbacks.Enqueue(MoveTemp(Readback));
            });

        mTimeAccumulator -= 1.f / static_cast<float>(Framerate);
    }
    ENQUEUE_RENDER_COMMAND(FDequeueCaptureDownloadRequest)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            QUICK_SCOPE_CYCLE_COUNTER(STAT_VortexDepthCameraActorComponent_Readback);
            while (!mRHITextureReadbacks.IsEmpty())
            {
                TUniquePtr<FRHIGPUTextureReadback>* Last = mRHITextureReadbacks.Peek();
                if (!(*Last)->IsReady())
                {
                    break;
                }
                void* LockedPtr = nullptr;
                int32 RowPitch = 0;
                (*Last)->LockTexture(RHICmdList, LockedPtr, RowPitch);
                auto* SurfaceData = reinterpret_cast<float*>(LockedPtr);
                {
                    FScopeLock lLock(&mReadBacksLock);
                    if (mBackReadback->Num() >= Width * Height * 3)
                    {
                        mBackReadback->Reset();
                    }
                    for (int32 i = 0; i < Height * RowPitch; ++i)
                    {
                        float lDepth = SurfaceData[i];
                        mBackReadback->Add(FMath::Clamp((lDepth - GNearClippingPlane) / (ZMax - GNearClippingPlane), 0.f, 1.f));
                    }
                }
                (*Last)->Unlock();
                mRHITextureReadbacks.Pop();
            }
        });
}

// Called when the game starts or when spawned
void UVortexDepthCameraActorComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UVortexDepthCameraActorComponent::OnRegister()
{
    if (CaptureComponent == nullptr)
    {
        CaptureComponent = NewObject<USceneCaptureComponent2D>(GetOwner(), NAME_None, RF_Transient | RF_TextExportTransient);
        CaptureComponent->SetupAttachment(this);
        CaptureComponent->CreationMethod = CreationMethod;
        CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
        CaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
        CaptureComponent->bCaptureEveryFrame = false;
        CaptureComponent->bCaptureOnMovement = false;
        CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>(GetOwner(), NAME_None, RF_Transient | RF_TextExportTransient);
        CaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_R32f;
        CaptureComponent->TextureTarget->SRGB = false;
        CaptureComponent->TextureTarget->bNeedsTwoCopies = true;
        CaptureComponent->TextureTarget->SizeX = 1280;
        CaptureComponent->TextureTarget->SizeY = 720;
        CaptureComponent->TextureTarget->UpdateResource();
        CaptureComponent->RegisterComponentWithWorld(GetWorld());
    }
    Super::OnRegister();
}

void UVortexDepthCameraActorComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);
    FlushRenderingCommands();
    if (CaptureComponent)
    {
        CaptureComponent->DestroyComponent();
        CaptureComponent = nullptr;
    }
}

const float* UVortexDepthCameraActorComponent::takeSnapshot(uint64_t& Size)
{
    {
        FScopeLock lLock(&mReadBacksLock);
        Swap(mFrontReadback, mBackReadback);
        mBackReadback->Reset();
    }
    Size = mFrontReadback->Num();
    return mFrontReadback->GetData();
}