#pragma once
//Copyright(c) 2019 CM Labs Simulations Inc.All rights reserved.
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//the sample code softwareand associated documentation files(the "Software"), to deal with
//the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
//of the Software, and to permit persons to whom the Software is furnished to
//do so, subject to the following conditions :
//
//Redistributions of source code must retain the above copyright notice,
//this list of conditionsand the following disclaimers.
//Redistributions in binary form must reproduce the above copyright notice,
//this list of conditionsand the following disclaimers in the documentation
//and /or other materials provided with the distribution.
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
#include "VortexIntegrationUtilities.h"
#include "VortexIntegration/Structs.h"

#include "Stats/Stats2.h"
#include "Runtime/Core/Public/Containers/Map.h"

#include <map>
#include <vector>

DECLARE_STATS_GROUP(TEXT("VortexTerrain"), STATGROUP_VortexTerrain, STATCAT_Advanced);

namespace physx
{
    class PxHeightField;
}

class UPrimitiveComponent;
class FVortexTerrain
{
public:

    FVortexTerrain(bool EnableLandscapeCollision, bool EnableMeshSimpleCollision, bool EnableMeshComplexCollision);

    void Query(const VortexTerrainProviderRequest* request, VortexTerrainProviderResponse* response);
    void PostQuery();

    /// Called when the terrain producer destroys its terrain representation
    ///
    void OnDestroy();

private:

    struct ComponentKey
    {
        uint32 UniqueID;
        int32  InstanceID;
        operator uint64() const;
    };

    struct LandscapeComponentBuffer
    {
        std::vector<double> Vertices;
        std::vector<uint8_t> Materials0;
        std::vector<uint8_t> Materials1;
        std::vector<VortexMaterial> MaterialDictionary;
    };

    struct LandscapeComponentBuffers
    {
        LandscapeComponentBuffers();

        LandscapeComponentBuffer& GetNextBuffer();
        void Reset();

        int32 UsageCounter;
        std::vector<LandscapeComponentBuffer> Buffers;
    };

    struct TriangleMeshBuffer
    {
        std::vector<double> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<uint16_t> Materials;
        std::vector<VortexMaterial> MaterialDictionary;
    };

    void ExportPxHeightField(UObject* object, PxHeightField const* const HeightField, const FTransform& LocalToWorld, LandscapeComponentBuffer& Buffer, FVector& UnrealPosition, float& CellSizeX, float& CellSizeY);
    bool ExportBody(UBodySetup* BodySetup, FBodyInstance* BodyInstance, const FTransform& WorldTransform, VortexTerrainProviderObject& ResponseObject);

    ComponentKey MakeKey(UPrimitiveComponent* Component, int32 InstanceID = 0);

    uint32 GetOrGenerateUniqueId(const FVortexTerrain::ComponentKey& Key);

    LandscapeComponentBuffer& GetNextLandscapeComponentBuffer(int32 VerticesCount);
    void RestartLandscapeComponentBuffersUsage();

    /// Reusable buffers containing all vertexes heights.
    /// It is assumed that each landscape component has the same number of vertexes.
    /// Each vector is not cleaned up after use, since we want to reuse them.
    ///
    std::map<int32, LandscapeComponentBuffers> LandscapeBuffers;
    std::vector<std::vector<double>> ConvexBuffers;
    std::vector<TriangleMeshBuffer> TriangleMeshBuffers;

    /// Set of all already sent components unique ID
    ///
    TMap<uint64, uint32> ComponentUniqueIds;
    uint32 SequentialTerrainProviderID;
    std::vector<VortexTerrainProviderObject> TerrainProviderObjects;

    /// Copy of collision detection settings
    ///
    bool EnableLandscapeCollisionDetection;
    bool EnableMeshSimpleCollisionDetection;
    bool EnableMeshComplexCollisionDetection;

    /// Filters used when detecting the terrain
    ///
    TArray<UClass*> ComponentClassFilters;
};