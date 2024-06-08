#include "VortexColorCameraActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Core.h"
#include "VortexIntegrationUtilities.h"
#include "RenderCore/Public/RenderingThread.h"

// Sets default values
UVortexColorCameraActorComponent::UVortexColorCameraActorComponent()
{
    CaptureComponent = nullptr;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
    mFrontReadback = &mReadbacks[0];
    mBackReadback = &mReadbacks[1];
}

void UVortexColorCameraActorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    if (CaptureComponent->TextureTarget->SizeX != Width || CaptureComponent->TextureTarget->SizeY != Height)
    {
        CaptureComponent->TextureTarget->ResizeTarget(Width, Height);
        mBackReadback->Reserve(Width * Height * 3 * 3);
        mFrontReadback->Reserve(Width * Height * 3 * 3);
    }
    
    // Vortex produces the vertical FOV in radians. Unreal consumes the horizontal FOV in degrees.
    const float aspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    const float unrealFOV = FMath::RadiansToDegrees(2.0f * FMath::Atan(FMath::Tan(FOV * 0.5f) * aspectRatio));

    if (CaptureComponent->FOVAngle != unrealFOV)
    {
        CaptureComponent->FOVAngle = unrealFOV;
    }
    
    mTimeAccumulator += DeltaTime;
    while (mTimeAccumulator >= 1.f / static_cast<float>(Framerate))
    {
        CaptureComponent->CaptureScene();
        ENQUEUE_RENDER_COMMAND(FEnqueueColorCaptureDownloadRequest)(
            [this](FRHICommandListImmediate& RHICmdList)
            {
                TUniquePtr<FRHIGPUTextureReadback> Readback = MakeUnique<FRHIGPUTextureReadback>(NAME_None);
                Readback->EnqueueCopy(RHICmdList,
                                      CaptureComponent->TextureTarget->Resource->GetTexture2DRHI());
                mRHITextureReadbacks.Enqueue(MoveTemp(Readback));
            });

        mTimeAccumulator -= 1.f / static_cast<float>(Framerate);
    }
    ENQUEUE_RENDER_COMMAND(FDequeueColorCaptureDownloadRequest)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            QUICK_SCOPE_CYCLE_COUNTER(STAT_VortexColorCameraActorComponent_Readback);
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
                auto* SurfaceData = reinterpret_cast<uint8*>(LockedPtr);
                {
                    FScopeLock lLock(&mReadBacksLock);
                    if (mBackReadback->Num() >= Width * Height * 3 * 3)
                    {
                        mBackReadback->Reset();
                    }
                    for (int32 i = 0; i < Height; ++i)
                    {
                        for (int32 j = 0; j < Width; ++j)
                        {
                            //bgra -> rgb
                            mBackReadback->Add(SurfaceData[j * 4 + 2]);
                            mBackReadback->Add(SurfaceData[j * 4 + 1]);
                            mBackReadback->Add(SurfaceData[j * 4 + 0]);
                        }
                        SurfaceData += RowPitch * 4;
                    }
                }
                (*Last)->Unlock();
                mRHITextureReadbacks.Pop();
            }
        });
}

// Called when the game starts or when spawned
void UVortexColorCameraActorComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UVortexColorCameraActorComponent::OnRegister()
{
    if (CaptureComponent == nullptr)
    {
        CaptureComponent = NewObject<USceneCaptureComponent2D>(GetOwner(), NAME_None, RF_Transient | RF_TextExportTransient);
        CaptureComponent->SetupAttachment(this);
        CaptureComponent->CreationMethod = CreationMethod;
        CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
        CaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
        CaptureComponent->bCaptureEveryFrame = false;
        CaptureComponent->bCaptureOnMovement = false;
        CaptureComponent->bAlwaysPersistRenderingState = true;
        CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>(GetOwner(), NAME_None, RF_Transient | RF_TextExportTransient);
        CaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
        CaptureComponent->TextureTarget->SRGB = false;
        CaptureComponent->TextureTarget->bNeedsTwoCopies = true;
        CaptureComponent->TextureTarget->SizeX = 1280;
        CaptureComponent->TextureTarget->SizeY = 720;
        CaptureComponent->TextureTarget->UpdateResource();
        CaptureComponent->RegisterComponentWithWorld(GetWorld());
    }
    Super::OnRegister();
}

void UVortexColorCameraActorComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);
    FlushRenderingCommands();
    if (CaptureComponent)
    {
        CaptureComponent->DestroyComponent();
        CaptureComponent = nullptr;
    }
}

const uint8_t* UVortexColorCameraActorComponent::takeSnapshot(uint64_t& Size)
{
    {
        FScopeLock lLock(&mReadBacksLock);
        Swap(mFrontReadback, mBackReadback);
        mBackReadback->Reset();
    }
    Size = mFrontReadback->Num();
    return mFrontReadback->GetData();
}