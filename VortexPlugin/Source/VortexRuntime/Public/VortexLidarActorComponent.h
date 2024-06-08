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
#include "Math/Vector.h"
#include "VortexLidarActorComponent.generated.h"

UCLASS()
class VORTEXRUNTIME_API UVortexLidarActorComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    // Sets default values for this actor's properties
    UVortexLidarActorComponent();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    int NumberOfChannels = 16;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    float Range = 10000.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    int HorizontalResolution = 1000;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    float HorizontalRotationFrequency = 10.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    float HorizontalFovStart = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    float HorizontalFovLength = 360.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    float VerticalFovUpper = 10.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    float VerticalFovLower = -10.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    bool OutputAsDistanceField = false;

    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    //float UpdateFrequency = 10.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lidar Parameters")
    bool PointCloudVisualization = true;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    void visualizePointCloud(const float* inLastFullScan);
    const float* takePointCloud(uint64_t& Size);

private:
    void generatePointCloud();
    void shootLaser(float horizontalAngle, float verticalAngle, FVector& pointWorld, FVector& pointLidar);

private:
    //class USceneComponent* mRootSceneComponent;
    float mCurrentHorizontalAngle;
    TArray<float> mLasersAngles;
    TArray<float> mPointClouds[2];
    TArray<float>* mFrontPointCloud;
    TArray<float>* mBackPointCloud;
};
