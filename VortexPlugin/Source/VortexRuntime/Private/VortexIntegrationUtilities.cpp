#include "VortexIntegrationUtilities.h"

#include <algorithm>

namespace
{
    const double kMToCm = 100.0f;
    const double kCmToM = 1.0f / kMToCm;
}

namespace VortexIntegrationUtilities
{
    double ConvertLengthToVortex(double unrealLength)
    {
        return unrealLength * kCmToM;
    }

    double ConvertLengthToUnreal(double vortexLength)
    {
        return vortexLength * kMToCm;
    }

    void ConvertTransform(const FTransform& UnrealTransform, double Translation[3], double RotationQuaternion[4])
    {
        VortexIntegrationUtilities::ConvertTranslation(UnrealTransform.GetTranslation(), Translation);
        VortexIntegrationUtilities::ConvertRotation(UnrealTransform.GetRotation(), RotationQuaternion);
    }

    FTransform ConvertTransform(const double Translation[3], const double RotationQuaternion[4])
    {
        FVector TranslationInUnreal = VortexIntegrationUtilities::ConvertTranslation(Translation);
        FQuat RotationInUnreal = VortexIntegrationUtilities::ConvertRotation(RotationQuaternion);

        return FTransform(RotationInUnreal, TranslationInUnreal);
    }

    void ConvertTranslation(const FVector& UnrealVector, double Translation[3])
    {
        FVector UnrealVectorInMeters = UnrealVector * kCmToM;
        Translation[0] = UnrealVectorInMeters.X;
        Translation[1] = -UnrealVectorInMeters.Y;
        Translation[2] = UnrealVectorInMeters.Z;
    }

    FVector ConvertTranslation(const double Translation[3])
    {
        return FVector(Translation[0] * kMToCm, -Translation[1] * kMToCm, Translation[2] * kMToCm);
    }

    void ConvertBox(const FBox& unrealBox, double min[3], double max[3])
    {
        double Min[3], Max[3];
        ConvertTranslation(unrealBox.Min, Min);
        ConvertTranslation(unrealBox.Max, Max);
        min[0] = std::min(Min[0], Max[0]);
        min[1] = std::min(Min[1], Max[1]);
        min[2] = std::min(Min[2], Max[2]);
        max[0] = std::max(Min[0], Max[0]);
        max[1] = std::max(Min[1], Max[1]);
        max[2] = std::max(Min[2], Max[2]);
    }

    FVector ConvertScale(const double Scale[3])
    {
        return FVector(Scale[0] * kMToCm, Scale[1] * kMToCm, Scale[2] * kMToCm);
    }

    void ConvertScale(const FVector& Scale, double Out[3])
    {
        Out[0] = Scale[0] / kMToCm;
        Out[1] = Scale[1] / kMToCm;
        Out[2] = Scale[2] / kMToCm;
    }

    FVector ConvertDirection(const double Direction[3])
    {
        return FVector(Direction[0], -Direction[1], Direction[2]);
    }

    void ConvertRotation(const FQuat& UnrealQuat, double RotationQuaternion[4])
    {
        RotationQuaternion[0] = UnrealQuat.W;
        RotationQuaternion[1] = -UnrealQuat.X;
        RotationQuaternion[2] = UnrealQuat.Y;
        RotationQuaternion[3] = -UnrealQuat.Z;
    }

    FQuat ConvertRotation(const double RotationQuaternion[4])
    {
        // Conversion is made in 2 steps:
        //
        // 1. Bring the rotation axis of the Vortex quaternion(x, y, z) into Unreal space(x, -y, z).
        // 2. Flip the direction of the axis to get the left-handedness for the rotation(-x, y, -z).
        //
        return FQuat(-RotationQuaternion[1], RotationQuaternion[2], -RotationQuaternion[3], RotationQuaternion[0]);
    }
}
