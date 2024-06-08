#pragma once
//Copyright(c) 2019 CM Labs Simulations Inc. All rights reserved.
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
#include <Math/Box.h>
#include <Math/TransformVectorized.h>

///
/// This file defines some utility functions for the vortex integration
/// Here are some key elements to take into consideration when integrating Vortex to Unreal:
///
/// 1. The Coordinate System
///    - Vortex uses a right hand coordinate system (X-Forward, Y-Left, Z-Up).
///    - Unreal uses a left hand coordinate system (X-Forward, Y-Right, Z-Up).
///
/// 2. The default distance unit
///    - Vortex is in meters.
///    - Unreal is in centimeters.
///
namespace VortexIntegrationUtilities
{
    /// Converts a length from the Unreal world to an equivalent length in the Vortex world.
    ///
    /// @param[in]  unrealLength  A length in the Unreal world.
    ///
    /// @return The equivalent length in the Vortex world.
    ///
    VORTEXRUNTIME_API double ConvertLengthToVortex(double unrealLength);

    /// Converts a length from the Vortex world to an equivalent length in the Unreal world.
    ///
    /// @param[in]  vortexLength A length in the Vortex world.
    ///
    /// @return The equivalent length in the Unreal world.
    ///
    VORTEXRUNTIME_API double ConvertLengthToUnreal(double vortexLength);

    /// Converts a FTransform from the Unreal world to an equivalent translation and rotation in the Vortex world.
    ///
    /// @param[in]  unrealTransform    A transform in the Unreal world.
    /// @param[out] translation        The equivalent translation in the Vortex world (x, y, z).
    /// @param[out] rotationQuaternion The equivalent rotation (as a quaternion) in the Vortex world (w, x, y, z).
    ///
    /// @note This method will take care of changing the used coordinate system and default distance unit.
    /// @note Scaling is not supported.
    ///
    VORTEXRUNTIME_API void ConvertTransform(const FTransform& unrealTransform, double translation[3], double rotationQuaternion[4]);

    /// Converts a translation and rotation from the Vortex world to an equivalent transform in the Unreal world.
    ///
    /// @param[in] translation        A translation in the Vortex world (x, y, z).
    /// @param[in] rotationQuaternion A rotation (as a quaternion) in the Vortex world (w, x, y, z).
    ///
    /// @return The equivalent transform in the Unreal world.
    ///
    /// @note This method will take care of changing the used coordinate system and default distance unit.
    /// @note Scaling is not supported.
    ///
    VORTEXRUNTIME_API FTransform ConvertTransform(const double translation[3], const double rotationQuaternion[4]);

    /// Converts a translation from the Unreal world to an equivalent translation in the Vortex world.
    ///
    /// @param[in]  unrealVector  A vector in the Unreal world.
    /// @param[out] translation   The equivalent translation in the Vortex world (x, y, z).
    ///
    /// @note This method assumes that the provided vector is in centimeters. It will convert the vector to meters.
    ///
    VORTEXRUNTIME_API void ConvertTranslation(const FVector& unrealVector, double translation[3]);

    /// Converts a translation from the Vortex world to an equivalent translation in the Unreal world.
    ///
    /// @param[in] translation A translation in the Vortex world (x, y, z).
    ///
    /// @return The equivalent translation in the Unreal world.
    ///
    /// @note This method assumes that the provided vector is in meters. It will convert the vector to centimeters.
    ///
    VORTEXRUNTIME_API FVector ConvertTranslation(const double translation[3]);

    /// Converts a box from the Unreal world to an equivalent box in the Vortex world.
    ///
    /// @param[in]  unrealBox  A box in the Unreal world.
    /// @param[out] min        The equivalent min of a box in the Vortex world (x, y, z).
    /// @param[out] max        The equivalent max of a box in the Vortex world (x, y, z).
    ///
    /// @note This method assumes that the provided box is in centimeters. It will convert the box to meters.
    ///
    VORTEXRUNTIME_API void ConvertBox(const FBox& unrealBox, double min[3], double max[3]);

    /// Converts a scale from the Vortex world to an equivalent scale in the Unreal world.
    ///
    /// @param[in] scale A scale in the Vortex world (x, y, z).
    ///
    /// @return The equivalent scale in the Unreal world.
    ///
    /// @note This method assumes that the provided vector is in meters. It will convert the vector to centimeters.
    ///
    VORTEXRUNTIME_API FVector ConvertScale(const double scale[3]);

    /// Converts a scale from the Unreal world to an equivalent scale in the Vortex world.
    ///
    /// @param[in] scale A scale in the Unreal world (x, y, z).
    ///
    /// @param[out] out The equivalent scale in the Vortex world.
    ///
    /// @note This method assumes that the provided vector is in centimeters. It will convert the vector to meters.
    ///
    VORTEXRUNTIME_API void ConvertScale(const FVector& scale, double out[3]);

    /// Converts a direction vector from the Vortex world to an equivalent direction vector in the Unreal world.
    ///
    /// @param[in] translation A direction vector in the Vortex world (x, y, z).
    ///
    /// @return The equivalent direction vector in the Unreal world.
    ///
    VORTEXRUNTIME_API FVector ConvertDirection(const double direction[3]);

    /// Converts a FQuat from the Unreal world to an equivalent Quaternion in the Vortex world.
    ///
    /// @param[in]  unrealQuat         A quaternion in the Unreal world.
    /// @param[out] rotationQuaternion The equivalent rotation (as a quaternion) in the Vortex world (w, x, y, z).
    ///
    VORTEXRUNTIME_API void ConvertRotation(const FQuat& unrealQuat, double rotationQuaternion[4]);

    /// Converts a rotation as quaternion from the Vortex world to an equivalent FQuat in the Unreal world.
    ///
    /// @param[in] rotationQuaternion A rotation (as a quaternion) in the Vortex world (w, x, y, z).
    ///
    /// @return The equivalent rotation in the Unreal world.
    ///
    VORTEXRUNTIME_API FQuat ConvertRotation(const double rotationQuaternion[4]);
}
