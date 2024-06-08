#pragma once
//Copyright(c) 2021 CM Labs Simulations Inc. All rights reserved.
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
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Containers/Array.h"
#include "Containers/Queue.h"
#include "Math/Vector.h"
#include "Misc/ScopeLock.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RHI/Public/RHIGPUReadback.h"

#include "VortexDepthCameraActorComponent.generated.h"

UCLASS()
class VORTEXRUNTIME_API UVortexDepthCameraActorComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    // Sets default values for this actor's properties
    UVortexDepthCameraActorComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Depth Camera Parameters")
    int Width = 1280;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Depth Camera Parameters")
    int Height = 720;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Depth Camera Parameters")
    float FOV = 45.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Depth Camera Parameters")
    int Framerate = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Depth Camera Parameters")
    float ZMax = 1000.f; // cm

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Depth Camera Parameters")
    USceneCaptureComponent2D* CaptureComponent;
    
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void OnRegister() override;

public:
    // Called every frame
    const float* takeSnapshot(uint64_t& Size);

    virtual void TickComponent(float DeltaTime,enum ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction);

private:
    TQueue<TUniquePtr<FRHIGPUTextureReadback>> mRHITextureReadbacks;
    TArray<float> mReadbacks[2];
    TArray<float>* mFrontReadback;
    TArray<float>* mBackReadback;
    FCriticalSection mReadBacksLock;
    float mTimeAccumulator;
};
