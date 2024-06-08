#include "VortexLidarActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Core.h"
#include "VortexIntegrationUtilities.h"
#include "VortexIntegration/VortexIntegration.h"

constexpr uint32 MAX_POINTS_IN_SCAN = 1e5;

namespace
{
    void normalizeAngles(float inStartAngle, float inEndAngle, float& outNormStartAngle, float& outNormEndAngle)
    {
        outNormStartAngle = FMath::Fmod(360.0f + inStartAngle, 360.0f);
        outNormEndAngle = inEndAngle;
        if (inStartAngle != inEndAngle)
        {
            while (outNormEndAngle <= outNormStartAngle)
            {
                outNormEndAngle += 360.f;
            }
        }
    }

    float getDeltaAngles(float inStartAngle, float inEndAngle)
    {
        if (inStartAngle <= inEndAngle)
        {
            return inEndAngle - inStartAngle;
        }
        return 360.f - inStartAngle + inEndAngle;
    }

    float snapToGrid(float inLocation, float inCellSize)
    {
        return FMath::Floor((inLocation + (inCellSize / 2.f)) / inCellSize) * inCellSize;
    }

    void slideAngles(float inStartAngle, float inEndAngle, float& outNormStartAngle, float& outNormEndAngle)
    {
        outNormStartAngle = inStartAngle + 360.f;
        outNormEndAngle = inEndAngle + 360.f;
    }

    bool findIntervalOverlap(float inX1, float inX2,
                             float inY1, float inY2,
                             float& outOverlapStart, float& outOverlapEnd)
    {
        if (inX1 < inY2 && inY1 < inX2)
        {
            outOverlapStart = FMath::Max(inX1, inY1);
            outOverlapEnd = FMath::Min(inX2, inY2);
            return true;
        }
        return false;
    }

    struct CircleOverlap
    {
        float mStart;
        float mEnd;
    };

    int findCircleOverlapSnapped(float inScanStart, float inScanEnd,
                                 float inGridStart, float inGridEnd, float inCellSize,
                                 TStaticArray<CircleOverlap, 3>& outOverlaps)
    {
        float lNormAlpha1;
        float lNormAlpha2;
        normalizeAngles(inScanStart, inScanEnd, lNormAlpha1, lNormAlpha2);

        float lNormBeta1;
        float lNormBeta2;
        normalizeAngles(inGridStart, inGridEnd, lNormBeta1, lNormBeta2);

        float lSlideNormAlpha1;
        float lSlideNormAlpha2;
        slideAngles(lNormAlpha1, lNormAlpha2, lSlideNormAlpha1, lSlideNormAlpha2);

        float lSlideNormBeta1;
        float lSlideNormBeta2;
        slideAngles(lNormBeta1, lNormBeta2, lSlideNormBeta1, lSlideNormBeta2);

        int lOverlapFound = 0;
        if (findIntervalOverlap(lSlideNormAlpha1, lSlideNormAlpha2,
                                lNormBeta1, lNormBeta2,
                                outOverlaps[lOverlapFound].mStart,
                                outOverlaps[lOverlapFound].mEnd))
        {
            outOverlaps[lOverlapFound].mStart = snapToGrid(outOverlaps[lOverlapFound].mStart - lNormBeta1, inCellSize) + lNormBeta1;
            outOverlaps[lOverlapFound].mEnd = snapToGrid(outOverlaps[lOverlapFound].mEnd - lNormBeta1, inCellSize) + lNormBeta1;
            ++lOverlapFound;
        }
        if (findIntervalOverlap(lNormAlpha1, lNormAlpha2,
                                lNormBeta1, lNormBeta2,
                                outOverlaps[lOverlapFound].mStart,
                                outOverlaps[lOverlapFound].mEnd))
        {
            outOverlaps[lOverlapFound].mStart = snapToGrid(outOverlaps[lOverlapFound].mStart - lNormBeta1, inCellSize) + lNormBeta1;
            outOverlaps[lOverlapFound].mEnd = snapToGrid(outOverlaps[lOverlapFound].mEnd - lNormBeta1, inCellSize) + lNormBeta1;
            ++lOverlapFound;
        }
        if (findIntervalOverlap(lNormAlpha1, lNormAlpha2,
                                lSlideNormBeta1, lSlideNormBeta2,
                                outOverlaps[lOverlapFound].mStart,
                                outOverlaps[lOverlapFound].mEnd))
        {
            outOverlaps[lOverlapFound].mStart = snapToGrid(outOverlaps[lOverlapFound].mStart - lSlideNormBeta1, inCellSize) + lSlideNormBeta1;
            outOverlaps[lOverlapFound].mEnd = snapToGrid(outOverlaps[lOverlapFound].mEnd - lSlideNormBeta1, inCellSize) + lSlideNormBeta1;
            ++lOverlapFound;
        }
        return lOverlapFound;
    }
} // anonymous namespace

// Sets default values
UVortexLidarActorComponent::UVortexLidarActorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    mFrontPointCloud = &mPointClouds[0];
    mBackPointCloud = &mPointClouds[1];
    mCurrentHorizontalAngle = 0.f;
    mPointClouds[0].Reserve(MAX_POINTS_IN_SCAN * 3);
    mPointClouds[1].Reserve(MAX_POINTS_IN_SCAN * 3);
}

// Called when the game starts or when spawned
void UVortexLidarActorComponent::BeginPlay()
{
    Super::BeginPlay();
    mFrontPointCloud->Reset();
    mBackPointCloud->Reset();
    mCurrentHorizontalAngle = FMath::Fmod(360.f + HorizontalFovStart, 360.f);
}

const float* UVortexLidarActorComponent::takePointCloud(uint64_t& Size)
{
    generatePointCloud();
    Swap(mFrontPointCloud, mBackPointCloud);
    mBackPointCloud->Reset();
    if (OutputAsDistanceField)
    {
        Size = mFrontPointCloud->Num();
    }
    else
    {
        Size = mFrontPointCloud->Num() / 3;
    }
    return mFrontPointCloud->GetData();
}

void UVortexLidarActorComponent::generatePointCloud()
{
    QUICK_SCOPE_CYCLE_COUNTER(STAT_VortexLidarActorComponent_GeneratePointCloud);
    if (!GetOwner()->HasActorBegunPlay())
    {
        return;
    }
    if (NumberOfChannels > 0 && mLasersAngles.Num() != NumberOfChannels)
    {
        mLasersAngles.Reset();
        float deltaAngle = 0.f;
        if (NumberOfChannels > 1)
        {
            deltaAngle = (VerticalFovUpper - VerticalFovLower) / static_cast<float>(NumberOfChannels - 1);
        }

        for (int i = 0; i < NumberOfChannels; ++i)
        {
            float verticalAngle = VerticalFovLower + static_cast<float>(i) * deltaAngle;
            mLasersAngles.Emplace(verticalAngle);
        }
    }

    float lAngleDistanceOfTick =
        HorizontalRotationFrequency
        * 360.0f
        / ::VortexGetSimulationFrameRate();

    float lStartHorizontalAngle = mCurrentHorizontalAngle;
    float lEndHorizontalAngle = FMath::Fmod(lStartHorizontalAngle + lAngleDistanceOfTick, 360.0f);
    mCurrentHorizontalAngle = lEndHorizontalAngle;

    TStaticArray<CircleOverlap, 3> lROIs = {};
    float lGridCellSize = HorizontalFovLength / static_cast<float>(HorizontalResolution);
    int lOverlapsFound = findCircleOverlapSnapped(lStartHorizontalAngle,
                                                  lStartHorizontalAngle + lAngleDistanceOfTick,
                                                  HorizontalFovStart,
                                                  HorizontalFovStart + HorizontalFovLength,
                                                  lGridCellSize,
                                                  lROIs);
    if (0 == lOverlapsFound)
    {
        return;
    }
    int lCounter = 0;
    while (lCounter != lOverlapsFound)
    {
        int32_t lPointsToScanWithOneLaser = static_cast<int32_t>(FMath::RoundHalfFromZero(((getDeltaAngles(lROIs[lCounter].mStart, lROIs[lCounter].mEnd)) / lGridCellSize)));
        if (lPointsToScanWithOneLaser <= 0)
        {
            ++lCounter;
            continue;
        }
        for (auto i = 0; i < lPointsToScanWithOneLaser; ++i)
        {
            const float horizontalAngle = FMath::Fmod(FMath::Fmod(lROIs[lCounter].mStart, 360.f) + lGridCellSize * i, 360.f);
            for (int laser = 0; laser < NumberOfChannels; ++laser)
            {
                const float verticalAngle = mLasersAngles[laser];

                FVector pointWorld;
                FVector pointLidar;
                // shoot laser and get the impact point
                shootLaser(horizontalAngle, verticalAngle, pointWorld, pointLidar);
                if (OutputAsDistanceField)
                {
                    double vortexTranslation[3];
                    VortexIntegrationUtilities::ConvertTranslation(pointLidar, vortexTranslation);
                    FVector vortexPoint(static_cast<float>(vortexTranslation[0]),
                                        static_cast<float>(vortexTranslation[1]),
                                        static_cast<float>(vortexTranslation[2]));
                    mBackPointCloud->Add(vortexPoint.Size());
                }
                else
                {
                    double vortexTranslation[3];
                    VortexIntegrationUtilities::ConvertTranslation(pointWorld, vortexTranslation);
                    mBackPointCloud->Add(static_cast<float>(vortexTranslation[0]));
                    mBackPointCloud->Add(static_cast<float>(vortexTranslation[1]));
                    mBackPointCloud->Add(static_cast<float>(vortexTranslation[2]));
                }
            }
        }
        ++lCounter;
    }
}

void UVortexLidarActorComponent::shootLaser(float horizontalAngle, float verticalAngle, FVector& pointWorld, FVector& pointLidar)
{
    FVector start = GetComponentLocation();
    FQuat rayLidarFrame = FRotator(verticalAngle, horizontalAngle, 0).Quaternion();
    FQuat rayWorldFrame = (GetComponentQuat() * rayLidarFrame);
    rayWorldFrame.Normalize();
    FVector end = (rayWorldFrame.RotateVector(FVector::ForwardVector) * Range) + start;
    FHitResult hitResult = FHitResult(ForceInit);
    FCollisionQueryParams traceParams;
    traceParams.AddIgnoredActor(GetOwner());
    traceParams.bTraceComplex = true;
    bool isHit = GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECC_Visibility, traceParams);
    if (isHit)
    {
        pointWorld = hitResult.ImpactPoint;
    }
    else
    {
        pointWorld = end;
    }
    if (OutputAsDistanceField)
    {
        pointLidar = rayWorldFrame.Inverse().RotateVector(pointWorld);
        pointLidar = pointLidar - start;
    }
}

void UVortexLidarActorComponent::visualizePointCloud(const float* inLastFullScan)
{
    if (PointCloudVisualization && !OutputAsDistanceField && inLastFullScan != nullptr)
    {
        for (int i = 0; i < HorizontalResolution * NumberOfChannels; ++i)
        {
            double vortexTranslation[3];
            vortexTranslation[0] = static_cast<double>(inLastFullScan[i * 3 + 0]);
            vortexTranslation[1] = static_cast<double>(inLastFullScan[i * 3 + 1]);
            vortexTranslation[2] = static_cast<double>(inLastFullScan[i * 3 + 2]);
            FVector lPoint = VortexIntegrationUtilities::ConvertTranslation(vortexTranslation);
            DrawDebugPoint(
                GetWorld(),
                lPoint,
                5,
                i == 0 ? FColor::Red : FColor::Blue,
                true
            );
        }
    }
}