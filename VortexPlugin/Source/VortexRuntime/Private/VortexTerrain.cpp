#include "VortexTerrain.h"
#include "VortexRuntime.h"

#include "VortexIntegration/VortexIntegration.h"
#include "Runtime/Engine/Classes/Components/InstancedStaticMeshComponent.h"
#include "Runtime/Engine/Classes/Components/PrimitiveComponent.h"
#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h"
#include "Runtime/Engine/Classes/Engine/StaticMesh.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "Runtime/Engine/Classes/PhysicsEngine/BodySetup.h"
#include "Runtime/Engine/Classes/PhysicalMaterials/PhysicalMaterialMask.h"

#include "Landscape/Classes/LandscapeHeightfieldCollisionComponent.h"
#include "Landscape/Public/LandscapeDataAccess.h"

#if WITH_PHYSX
#include "PhysXPublic.h"
#endif

#include <algorithm>

DECLARE_CYCLE_STAT(TEXT("Query"), STAT_Query, STATGROUP_VortexTerrain);
DECLARE_CYCLE_STAT(TEXT("PostQuery"), STAT_PostQuery, STATGROUP_VortexTerrain);
DECLARE_CYCLE_STAT(TEXT("Destroy"), STAT_Destroy, STATGROUP_VortexTerrain);

namespace
{
    int32 FillInlineShapeArray_AssumesLocked(PhysicsInterfaceTypes::FInlineShapeArray& Array, const FPhysicsActorHandle& Actor)
    {
        FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Array);

        return Array.Num();
    }

    /** Util for finding the number of 'collision sim' shapes on this Actor */
    int32 GetNumSimShapes_AssumesLocked(const FPhysicsActorHandle& ActorRef)
    {
        PhysicsInterfaceTypes::FInlineShapeArray PShapes;
        const int32 NumShapes = ::FillInlineShapeArray_AssumesLocked(PShapes, ActorRef);

        int32 NumSimShapes = 0;

        for (FPhysicsShapeHandle& Shape : PShapes)
        {
            if (FPhysicsInterface::IsSimulationShape(Shape))
            {
                NumSimShapes++;
            }
        }

        return NumSimShapes;
    }

    bool IsSimulatingComplexGeometry(const FPhysicsActorHandle& ActorHandle)
    {
        bool IsUsingComplexCollision = false;
        FPhysicsCommand::ExecuteWrite(ActorHandle, [&](const FPhysicsActorHandle& Actor)
            {
                if (Actor.IsValid())
                {
                    if (GetNumSimShapes_AssumesLocked(Actor) > 0)
                    {
                        const int32 NumShapes = FPhysicsInterface::GetNumShapes(Actor);

                        TArray<FPhysicsShapeHandle> Shapes;
                        Shapes.AddUninitialized(NumShapes);
                        FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);

                        for (int32 ShapeIdx = Shapes.Num() - 1; ShapeIdx >= 0; --ShapeIdx)
                        {
                            const FPhysicsShapeHandle& Shape = Shapes[ShapeIdx];
                            if (FPhysicsInterface::GetShapeType(Shape) == ECollisionShapeType::Trimesh)
                            {
                                IsUsingComplexCollision = FPhysicsInterface::IsSimulationShape(Shape);
                                break;
                            }
                        }
                    }
                }
            });

        return IsUsingComplexCollision;
    }

    FString MakeNiceName(UPrimitiveComponent* Component)
    {
        FString NiceName = "";
        AActor* OwnerActor = Component->GetOwner();
        if (OwnerActor != nullptr)
        {
            NiceName += OwnerActor->GetName();
            NiceName += "-";
        }

        NiceName += Component->GetName();
        return NiceName;
    }
}

FVortexTerrain::ComponentKey::operator uint64() const
{
    return uint64(UniqueID) << 32 | uint64(InstanceID);
}

FVortexTerrain::LandscapeComponentBuffers::LandscapeComponentBuffers()
    : UsageCounter(0)
{

}

FVortexTerrain::LandscapeComponentBuffer& FVortexTerrain::LandscapeComponentBuffers::GetNextBuffer()
{
    if (UsageCounter == Buffers.size())
    {
        Buffers.push_back(LandscapeComponentBuffer());
    }

    return Buffers[UsageCounter++];
}

void FVortexTerrain::LandscapeComponentBuffers::Reset()
{
    UsageCounter = 0;

    // List of materials always changes, so we clear it.
    for (auto& Buffer : Buffers)
    {
        Buffer.MaterialDictionary.clear();
    }
}

FVortexTerrain::FVortexTerrain(bool EnableLandscapeCollision, bool EnableMeshSimpleCollision, bool EnableMeshComplexCollision)
    : ComponentUniqueIds()
    , SequentialTerrainProviderID(0)
    , EnableLandscapeCollisionDetection(EnableLandscapeCollision)
    , EnableMeshSimpleCollisionDetection(EnableMeshSimpleCollision)
    , EnableMeshComplexCollisionDetection(EnableMeshComplexCollision)
{
    if (EnableLandscapeCollision)
    {
        ComponentClassFilters.Add(ULandscapeHeightfieldCollisionComponent::StaticClass());
    }

    if (EnableMeshSimpleCollision || EnableMeshComplexCollision)
    {
        ComponentClassFilters.Add(UStaticMeshComponent::StaticClass());
    }
}

void FVortexTerrain::Query(const VortexTerrainProviderRequest* request, VortexTerrainProviderResponse* response)
{
    SCOPE_CYCLE_COUNTER(STAT_Query);

    FVortexRuntimeModule& RuntimeModule = FVortexRuntimeModule::Get();

    double ExtentsVx[3] = {
        request->bbox.max[0] - request->bbox.min[0],
        request->bbox.max[1] - request->bbox.min[1],
        request->bbox.max[2] - request->bbox.min[2] };
    FVector Extents = VortexIntegrationUtilities::ConvertScale(ExtentsVx);
    double PositionVx[3] = {
        (request->bbox.max[0] + request->bbox.min[0]) / 2.0,
        (request->bbox.max[1] + request->bbox.min[1]) / 2.0,
        (request->bbox.max[2] + request->bbox.min[2]) / 2.0 };
    FVector Position = VortexIntegrationUtilities::ConvertTranslation(PositionVx);

    // Toggling an object from SimulatingPhysics false to true than back to false leaves it in the ECC_WorldDynamic
    // when it initially was in ECC_WorldStatic. We don't want to miss those objects in our query so we filter on IsSimulatingPhysics() later instead
    TArray<TEnumAsByte<EObjectTypeQuery> > ObjectTypes = {
        UEngineTypes::ConvertToObjectType(ECC_WorldStatic),
        UEngineTypes::ConvertToObjectType(ECC_WorldDynamic) };

    TArray<UPrimitiveComponent*> OutComponents;
    // RuntimeModule.GetMechanismActors() excludes actors that contain a vortex mechanism since they are already properly simulated in vortex
    UKismetSystemLibrary::BoxOverlapComponents(RuntimeModule.GetCurrentWorld(), Position, Extents, ObjectTypes, nullptr, RuntimeModule.GetMechanismActors(), OutComponents);

    for (UPrimitiveComponent* PrimitiveComponent : OutComponents)
    {
        // The Terrain Provider only creates static objects for collision enabled components that are not simulated by PhysX
        if (PrimitiveComponent->IsCollisionEnabled() && !PrimitiveComponent->IsSimulatingPhysics()
            && ComponentClassFilters.FindByPredicate([PrimitiveComponent](UClass* ClassFilter) { return PrimitiveComponent->IsA(ClassFilter); }) != nullptr)
        {
            if (UInstancedStaticMeshComponent * InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(PrimitiveComponent))
            {
                FVector Min = VortexIntegrationUtilities::ConvertTranslation(request->bbox.min);
                FVector Max = VortexIntegrationUtilities::ConvertTranslation(request->bbox.max);
                FBox OverlapBox(Min, Min);
                OverlapBox += Max;

                auto Instances = InstancedStaticMeshComponent->GetInstancesOverlappingBox(OverlapBox, true);
                for (int32 k : Instances)
                {
                    VortexTerrainProviderObject responseObject = {};

                    ComponentKey Key = MakeKey(PrimitiveComponent, k);
                    responseObject.uniqueID = GetOrGenerateUniqueId(Key);

                    // Check if the component is still part of the terrain on the Vortex side.
                    // In the affirmative, we bypass any computation and only send the component unique ID (that's the only information needed by Vortex
                    // when the collision geometry has already been added).
                    bool MustSendCollision = true;
                    if (!VortexContainsTerrain(responseObject.uniqueID))
                    {
                        FString NiceName = MakeNiceName(InstancedStaticMeshComponent);
                        strncpy_s(responseObject.name, TCHAR_TO_UTF8(*NiceName), _countof(responseObject.name) - 1);
                        responseObject.name[_countof(responseObject.name) - 1] = '\0';

                        FTransform Transform;
                        InstancedStaticMeshComponent->GetInstanceTransform(k, Transform, true);
                        
                        bool Exported = ExportBody(InstancedStaticMeshComponent->GetBodySetup(), InstancedStaticMeshComponent->InstanceBodies[k], Transform, responseObject);
                        MustSendCollision = Exported;
                    }

                    if (MustSendCollision)
                    {
                        TerrainProviderObjects.push_back(responseObject);
                    }
                }
            }
            else if (ULandscapeHeightfieldCollisionComponent * LandscapeHeightfieldComponent = Cast<ULandscapeHeightfieldCollisionComponent>(PrimitiveComponent))
            {
                VortexTerrainProviderObject responseObject = {};

                ComponentKey Key = MakeKey(PrimitiveComponent);
                responseObject.uniqueID = GetOrGenerateUniqueId(Key);

                // Check if the component is still part of the terrain on the Vortex side.
                // In the affirmative, we bypass any computation and only send the component unique ID (that's the only information needed by Vortex
                // when the collision geometry has already been added).
                bool MustSendCollision = true;
                if (!VortexContainsTerrain(responseObject.uniqueID))
                {
                    if (IsValidRef(LandscapeHeightfieldComponent->HeightfieldRef) && LandscapeHeightfieldComponent->HeightfieldRef->RBHeightfield)
                    {
                        FString NiceName = MakeNiceName(PrimitiveComponent);
                        strncpy_s(responseObject.name, TCHAR_TO_UTF8(*NiceName), _countof(responseObject.name) - 1);
                        responseObject.name[_countof(responseObject.name) - 1] = '\0';

                        physx::PxHeightField* RBHeightfield = LandscapeHeightfieldComponent->HeightfieldRef->RBHeightfield;

                        FTransform HFToW = LandscapeHeightfieldComponent->GetComponentTransform();
                        HFToW.MultiplyScale3D(FVector(LandscapeHeightfieldComponent->CollisionScale, LandscapeHeightfieldComponent->CollisionScale, LANDSCAPE_ZSCALE));

                        responseObject.shapeCount = 1;
                        responseObject.shapes[0].shapeType = kVortexHeightField;

                        auto& shape = responseObject.shapes[0];

                        // We try to reuse the same Vertex buffers when we can, since all landscape components have the same number of Vertexes.
                        int32 VerticesCount = RBHeightfield->getNbRows() * RBHeightfield->getNbColumns();
                        LandscapeComponentBuffer& CurrentLandscapeComponentBuffer = GetNextLandscapeComponentBuffer(VerticesCount);

                        TArray<int32> IndexBuffer;
                        
                        FVector UnrealPosition;
                        float CellSizeX;
                        float CellSizeY;
                        ExportPxHeightField(LandscapeHeightfieldComponent->GetWorld(), RBHeightfield, HFToW, CurrentLandscapeComponentBuffer, UnrealPosition, CellSizeX, CellSizeY);

                        shape.heightField.heights = CurrentLandscapeComponentBuffer.Vertices.data();
                        shape.heightField.materials0 = CurrentLandscapeComponentBuffer.Materials0.data();
                        shape.heightField.materials1 = CurrentLandscapeComponentBuffer.Materials1.data();

                        TArray<UPhysicalMaterial*>& PhysicalMaterials = LandscapeHeightfieldComponent->CookedPhysicalMaterials;
                        CurrentLandscapeComponentBuffer.MaterialDictionary.resize(PhysicalMaterials.Num());
                        for (size_t MaterialIndex = 0; MaterialIndex < PhysicalMaterials.Num(); ++MaterialIndex)
                        {
                            FString VortexMaterialName = RuntimeModule.GetVortexMaterialFromPhysicalMaterial(PhysicalMaterials[MaterialIndex]);
                            VortexMaterial& VortexMaterial = CurrentLandscapeComponentBuffer.MaterialDictionary[MaterialIndex];

                            strncpy_s(VortexMaterial.name, TCHAR_TO_UTF8(*VortexMaterialName), _countof(VortexMaterial.name) - 1);
                            VortexMaterial.name[_countof(VortexMaterial.name) - 1] = '\0';
                        }
                        shape.heightField.materials = CurrentLandscapeComponentBuffer.MaterialDictionary.data();
                        shape.heightField.materialsCount = CurrentLandscapeComponentBuffer.MaterialDictionary.size();

                        shape.heightField.nbVerticesY = LandscapeHeightfieldComponent->HeightfieldRef->RBHeightfield->getNbRows();
                        shape.heightField.nbVerticesX = LandscapeHeightfieldComponent->HeightfieldRef->RBHeightfield->getNbColumns();
                        shape.heightField.cellSizeX = VortexIntegrationUtilities::ConvertLengthToVortex(CellSizeX);
                        shape.heightField.cellSizeY = VortexIntegrationUtilities::ConvertLengthToVortex(CellSizeY);
                        
                        // Parent is identity
                        VortexIntegrationUtilities::ConvertTransform(FTransform(), responseObject.position, responseObject.rotation);

                        // Component transform is not directly the one from Unreal, since the origin of the heigth field in Vortex is at the bottom right corner, and not the bottom left corner like in Unreal.
                        FTransform ComponentTransformAtVortexOrigin;
                        ComponentTransformAtVortexOrigin.SetTranslation(LandscapeHeightfieldComponent->GetComponentTransform().TransformPositionNoScale(UnrealPosition));
                        ComponentTransformAtVortexOrigin.SetRotation(LandscapeHeightfieldComponent->GetComponentTransform().GetRotation());
                        
                        VortexIntegrationUtilities::ConvertTransform(ComponentTransformAtVortexOrigin, shape.position, shape.rotation);
                    }
                    else
                    {
                        MustSendCollision = false;
                    }
                }

                if (MustSendCollision)
                {
                    TerrainProviderObjects.push_back(responseObject);
                }
            }
            else if (UBodySetup * BodySetup = PrimitiveComponent->GetBodySetup())
            {
                FBodyInstance* BodyInstance = PrimitiveComponent->GetBodyInstance();
                if (BodyInstance->ActorHandle.IsValid())
                {
                    VortexTerrainProviderObject responseObject = {};

                    ComponentKey Key = MakeKey(PrimitiveComponent);
                    responseObject.uniqueID = GetOrGenerateUniqueId(Key);

                    // Check if the component is still part of the terrain on the Vortex side.
                    // In the affirmative, we bypass any computation and only send the component unique ID (that's the only information needed by Vortex
                    // when the collision geometry has already been added).
                    bool MustSendCollision = true;
                    if (!VortexContainsTerrain(responseObject.uniqueID))
                    {
                        FString NiceName = MakeNiceName(PrimitiveComponent);
                        strncpy_s(responseObject.name, TCHAR_TO_UTF8(*NiceName), _countof(responseObject.name) - 1);
                        responseObject.name[_countof(responseObject.name) - 1] = '\0';

                        bool Exported = ExportBody(BodySetup, BodyInstance, PrimitiveComponent->GetComponentToWorld(), responseObject);
                        MustSendCollision = Exported;
                    }

                    if (MustSendCollision)
                    {
                        TerrainProviderObjects.push_back(responseObject);
                    }
                }
            }
        }
    }

    response->objects = TerrainProviderObjects.data();
    response->objectCount = TerrainProviderObjects.size();
}

void FVortexTerrain::PostQuery()
{
    SCOPE_CYCLE_COUNTER(STAT_PostQuery);

    TerrainProviderObjects.clear();
    ConvexBuffers.clear();
    TriangleMeshBuffers.clear();
    RestartLandscapeComponentBuffersUsage();
}

void FVortexTerrain::OnDestroy()
{
    SCOPE_CYCLE_COUNTER(STAT_Destroy);

    // Terrain is destroyed on the Vortex side. We need to reset our list of already sent components.
    ComponentUniqueIds.Empty();
}

/// This function is a modified version of:
/// Runtime\NavigationSystem\Private\NavMesh\RecastNavMeshGenerator.cpp (ExportPxHeightField() function)
///
void FVortexTerrain::ExportPxHeightField(UObject* object, PxHeightField const* const HeightField, const FTransform& LocalToWorld, LandscapeComponentBuffer& Buffer, FVector& UnrealPosition, float& CellSizeX, float& CellSizeY)
{
    const int32 NumRows = HeightField->getNbRows();
    const int32 NumCols = HeightField->getNbColumns();
    const int32 VertexCount = NumRows * NumCols;
    const int32 CellCount = (NumRows - 1) * (NumCols - 1);
    Buffer.Vertices.resize(VertexCount);
    Buffer.Materials0.resize(CellCount);
    Buffer.Materials1.resize(CellCount);

    // Unfortunately we have to use PxHeightField::saveCells instead PxHeightField::getHeight here 
    // because current PxHeightField interface does not provide an access to a triangle material index by HF 2D coordinates
    // PxHeightField::getTriangleMaterialIndex uses some internal adressing which does not match HF 2D coordinates
    static TArray<PxHeightFieldSample> HFSamples; // Make it static to reuse always the same array. We assume here that all landscape components have the same number of vertexes.
    HFSamples.SetNumUninitialized(VertexCount);
    {
        HeightField->saveCells(HFSamples.GetData(), VertexCount * HFSamples.GetTypeSize());
    }
    const bool bMirrored = (LocalToWorld.GetDeterminant() < 0.f);

    FTransform LocalToWorldScaleOnly;
    LocalToWorldScaleOnly.SetScale3D(LocalToWorld.GetScale3D());
   

    FVector ReferencePosition;
    FVector ReferencePositionX1;
    FVector ReferencePositionY1;

    size_t vertexIndex = 0;
    for (int32 Y = NumRows - 1; Y >= 0; Y--)
    {
        for (int32 X = 0; X < NumCols; X++)
        {
            const int32 SampleIdx = (bMirrored ? X : (NumCols - X - 1)) * NumCols + Y;

            const PxHeightFieldSample& Sample = HFSamples[SampleIdx];

            FVector ScaledUnrealCoords = LocalToWorldScaleOnly.TransformPosition(FVector(X, Y, Sample.height));

            Buffer.Vertices[vertexIndex] = VortexIntegrationUtilities::ConvertLengthToVortex(ScaledUnrealCoords.Z);
            ++vertexIndex;

            if (X == 0 && Y == NumRows - 1)
            {
                // Vortex needs to know the bottom right corner of the tile. This will be the origin.
                // As well, since the vertices heights are already at the right height, we discard the height of the reference position of the component.
                UnrealPosition = FVector(ScaledUnrealCoords.X, ScaledUnrealCoords.Y, 0.0);
            }
            // Keep some reference points to compute cell sizes in X and Y
            else if (X == 0 && Y == 0)
            {
                ReferencePosition = ScaledUnrealCoords;
            }
            else if (X == 0 && Y == 1)
            {
                ReferencePositionY1 = ScaledUnrealCoords;
            }
            else if (X == 1 && Y == 0)
            {
                ReferencePositionX1 = ScaledUnrealCoords;
            }

            // Can be helpful when debugging
#if 0
            FVector UnrealCoords = LocalToWorld.TransformPosition(FVector(X, Y, Sample.height));

            FLinearColor color = FLinearColor::Gray;
            float radius = 20.0;

            if ((X == 0 && Y == 0))
            {
                color = FLinearColor::Red;
                radius = 40.0;
            }
            if ((X == 0 && Y == NumRows - 1))
            {
                color = FLinearColor::Blue;
                radius = 60.0;
            }
            if ((X == NumCols - 1 && Y == 0))
            {
                color = FLinearColor::Yellow;
                radius = 70.0;
            }
            if ((X == NumCols - 1 && Y == NumRows - 1))
            {
                color = FLinearColor::Black;
                radius = 80.0;
            }

            if ((X == 0 && Y == 0) ||
                (X == 0 && Y == NumRows - 1) ||
                (X == NumCols - 1 && Y == 0) ||
                (X == NumCols - 1 && Y == NumRows - 1))
            {
                UKismetSystemLibrary::DrawDebugSphere(object, UnrealCoords, radius, 12, color, 1000.f);
                UE_LOG(LogVortex, Display, TEXT("FVortexRuntimeModule::UnrealCoords(): %.3f, %.3f, %.3f"), UnrealCoords.X, UnrealCoords.Y, UnrealCoords.Z);
            }
#endif
        }
    }

    CellSizeX = FVector::DistXY(ReferencePositionX1, ReferencePosition);
    CellSizeY = FVector::DistXY(ReferencePositionY1, ReferencePosition);

    size_t cellIndex = 0;
    for (int32 Y = NumRows - 1 - 1; Y >= 0; Y--)
    {
        for (int32 X = 0; X < NumCols - 1; X++)
        {
            const int32 SampleIdx = (bMirrored ? X : (NumCols - X - 1 - 1)) * NumCols + Y;
            const PxHeightFieldSample& Sample = HFSamples[SampleIdx];
            Buffer.Materials0[cellIndex] = Sample.materialIndex0;
            Buffer.Materials1[cellIndex] = Sample.materialIndex1;
            ++cellIndex;
        }
    }
}

bool FVortexTerrain::ExportBody(UBodySetup* BodySetup, FBodyInstance* BodyInstance, const FTransform& WorldTransform, VortexTerrainProviderObject& ResponseObject)
{
    bool MustSendCollision = false;

    const FVector Scale3D = WorldTransform.GetScale3D();
    FTransform ParentTM = WorldTransform;
    ParentTM.RemoveScaling();

    VortexIntegrationUtilities::ConvertTransform(ParentTM, ResponseObject.position, ResponseObject.rotation);

    bool UseComplexCollision = IsSimulatingComplexGeometry(BodyInstance->ActorHandle);
    if (!UseComplexCollision)
    {
        if (EnableMeshSimpleCollisionDetection)
        {
            // We support Sphere, Box, Capsule, Convex
            // We do not currently support TaperedCapsule and of course, Unknown
            for (int32 j = 0; j < BodySetup->AggGeom.BoxElems.Num() && ResponseObject.shapeCount < _countof(ResponseObject.shapes); ++j)
            {
                FTransform ElemTM = BodySetup->AggGeom.BoxElems[j].GetTransform();
                ElemTM.ScaleTranslation(Scale3D);

                auto& shape = ResponseObject.shapes[ResponseObject.shapeCount];
                shape.shapeType = kVortexBox;
                FVector BoxExtents(BodySetup->AggGeom.BoxElems[j].X, BodySetup->AggGeom.BoxElems[j].Y, BodySetup->AggGeom.BoxElems[j].Z);
                VortexIntegrationUtilities::ConvertScale(BoxExtents * Scale3D, shape.box.extents);
                VortexIntegrationUtilities::ConvertTransform(ElemTM, shape.position, shape.rotation);

                auto* material = BodyInstance->GetSimplePhysicalMaterial();
                FString VortexMaterialName = FVortexRuntimeModule::Get().GetVortexMaterialFromPhysicalMaterial(material);
                strncpy_s(shape.box.material.name, TCHAR_TO_UTF8(*VortexMaterialName), _countof(shape.box.material.name) - 1);
                shape.box.material.name[_countof(shape.box.material.name) - 1] = '\0';

                ++ResponseObject.shapeCount;

                MustSendCollision = true;
            }
            for (int32 j = 0; j < BodySetup->AggGeom.SphereElems.Num() && ResponseObject.shapeCount < _countof(ResponseObject.shapes); ++j)
            {
                FTransform ElemTM = BodySetup->AggGeom.SphereElems[j].GetTransform();
                ElemTM.ScaleTranslation(Scale3D);

                auto& shape = ResponseObject.shapes[ResponseObject.shapeCount];
                shape.shapeType = kVortexSphere;
                FVector SphereExtents(BodySetup->AggGeom.SphereElems[j].Radius, 0, 0);
                double extents[3];
                VortexIntegrationUtilities::ConvertScale(SphereExtents * Scale3D.GetAbsMin(), extents);
                shape.sphere.radius = extents[0];
                VortexIntegrationUtilities::ConvertTransform(ElemTM, shape.position, shape.rotation);

                auto* material = BodyInstance->GetSimplePhysicalMaterial();
                FString VortexMaterialName = FVortexRuntimeModule::Get().GetVortexMaterialFromPhysicalMaterial(material);
                strncpy_s(shape.sphere.material.name, TCHAR_TO_UTF8(*VortexMaterialName), _countof(shape.sphere.material.name) - 1);
                shape.sphere.material.name[_countof(shape.sphere.material.name) - 1] = '\0';

                ++ResponseObject.shapeCount;

                MustSendCollision = true;
            }
            for (int32 j = 0; j < BodySetup->AggGeom.SphylElems.Num() && ResponseObject.shapeCount < _countof(ResponseObject.shapes); ++j)
            {
                FTransform ElemTM = BodySetup->AggGeom.SphylElems[j].GetTransform();
                ElemTM.ScaleTranslation(Scale3D);

                const FVector Scale3DAbs = Scale3D.GetAbs();
                const float ScaleRadius = FMath::Max(Scale3DAbs.X, Scale3DAbs.Y);
                const float ScaleLength = Scale3DAbs.Z;

                auto& shape = ResponseObject.shapes[ResponseObject.shapeCount];
                shape.shapeType = kVortexCapsule;
                FVector SphylExtents(
                    BodySetup->AggGeom.SphylElems[j].Radius * ScaleRadius,
                    BodySetup->AggGeom.SphylElems[j].Length * ScaleLength,
                    0.0);
                double extents[3];
                VortexIntegrationUtilities::ConvertScale(SphylExtents, extents);
                shape.capsule.radius = extents[0];
                shape.capsule.length = extents[1];
                VortexIntegrationUtilities::ConvertTransform(ElemTM, shape.position, shape.rotation);

                auto* material = BodyInstance->GetSimplePhysicalMaterial();
                FString VortexMaterialName = FVortexRuntimeModule::Get().GetVortexMaterialFromPhysicalMaterial(material);
                strncpy_s(shape.capsule.material.name, TCHAR_TO_UTF8(*VortexMaterialName), _countof(shape.capsule.material.name) - 1);
                shape.capsule.material.name[_countof(shape.capsule.material.name) - 1] = '\0';

                ++ResponseObject.shapeCount;

                MustSendCollision = true;
            }
            for (int32 j = 0; j < BodySetup->AggGeom.ConvexElems.Num() && ResponseObject.shapeCount < _countof(ResponseObject.shapes); ++j)
            {
                FTransform ElemTM = BodySetup->AggGeom.ConvexElems[j].GetTransform();
                ElemTM.ScaleTranslation(Scale3D);

                auto& shape = ResponseObject.shapes[ResponseObject.shapeCount];
                shape.shapeType = kVortexConvex;

                auto& convex = BodySetup->AggGeom.ConvexElems[j];
                ConvexBuffers.push_back({});
                auto& data = ConvexBuffers.back();
                data.resize(convex.VertexData.Num() * 3);
                for (int32 k = 0; k < convex.VertexData.Num(); ++k)
                {
                    VortexIntegrationUtilities::ConvertTranslation(convex.VertexData[k] * Scale3D, &data[k * 3]);
                }
                shape.convex.vertices = data.data();
                shape.convex.vertexCount = uint32_t(convex.VertexData.Num());
                VortexIntegrationUtilities::ConvertTransform(ElemTM, shape.position, shape.rotation);

                auto* material = BodyInstance->GetSimplePhysicalMaterial();
                FString VortexMaterialName = FVortexRuntimeModule::Get().GetVortexMaterialFromPhysicalMaterial(material);
                strncpy_s(shape.convex.material.name, TCHAR_TO_UTF8(*VortexMaterialName), _countof(shape.convex.material.name) - 1);
                shape.convex.material.name[_countof(shape.convex.material.name) - 1] = '\0';

                ++ResponseObject.shapeCount;

                MustSendCollision = true;
            }
        }
    }
    else
    {
        if (EnableMeshComplexCollisionDetection)
        {
            // When using complex collisions, use the triangle meshes
            for (int32 j = 0; j < BodySetup->TriMeshes.Num() && ResponseObject.shapeCount < _countof(ResponseObject.shapes); ++j)
            {
                auto& shape = ResponseObject.shapes[ResponseObject.shapeCount];
                shape.shapeType = kVortexTriangleMesh;

                // Allocate buffer for new triangle mesh
                TriangleMeshBuffers.push_back({});
                auto& NewTriangleMeshBuffer = TriangleMeshBuffers.back();

                PxTriangleMesh* TempTriMesh = BodySetup->TriMeshes[j];
                PxU32 VertexCount = TempTriMesh->getNbVertices();
                PxU32 TriNumber = TempTriMesh->getNbTriangles();
                const PxVec3* Vertices = TempTriMesh->getVertices();
                const void* Triangles = TempTriMesh->getTriangles();

                NewTriangleMeshBuffer.Vertices.resize(VertexCount * 3);
                for (uint32 k = 0; k < VertexCount; ++k)
                {
                    VortexIntegrationUtilities::ConvertTranslation(FVector(Vertices[k].x, Vertices[k].y, Vertices[k].z) * Scale3D, &NewTriangleMeshBuffer.Vertices[k*3]);
                }
                shape.triangleMesh.vertexCount = uint32_t(VertexCount);
                shape.triangleMesh.vertices = NewTriangleMeshBuffer.Vertices.data();
                VortexIntegrationUtilities::ConvertTransform(FTransform(), shape.position, shape.rotation);

                // Grab triangle indices
                uint32_t I0, I1, I2;
                bool enableLog = false;

                NewTriangleMeshBuffer.Indices.resize(TriNumber * 3);
                NewTriangleMeshBuffer.Materials.resize(TriNumber);
                for (uint32 TriIndex = 0; TriIndex < TriNumber; ++TriIndex)
                {
                    if (TempTriMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES)
                    {
                        PxU16* P16BitIndices = (PxU16*)Triangles;
                        I0 = P16BitIndices[(TriIndex * 3) + 0];
                        I1 = P16BitIndices[(TriIndex * 3) + 1];
                        I2 = P16BitIndices[(TriIndex * 3) + 2];
                    }
                    else
                    {
                        PxU32* P32BitIndices = (PxU32*)Triangles;
                        I0 = P32BitIndices[(TriIndex * 3) + 0];
                        I1 = P32BitIndices[(TriIndex * 3) + 1];
                        I2 = P32BitIndices[(TriIndex * 3) + 2];
                    }

                    NewTriangleMeshBuffer.Indices[TriIndex * 3 + 0] = I2;
                    NewTriangleMeshBuffer.Indices[TriIndex * 3 + 1] = I1;
                    NewTriangleMeshBuffer.Indices[TriIndex * 3 + 2] = I0;
                    NewTriangleMeshBuffer.Materials[TriIndex] = TempTriMesh->getTriangleMaterialIndex(TriIndex);
                }
                shape.triangleMesh.triangleCount = uint32_t(TriNumber);
                shape.triangleMesh.indices = NewTriangleMeshBuffer.Indices.data();
                shape.triangleMesh.materialPerTriangle = NewTriangleMeshBuffer.Materials.data();

                // Materials
                TArray<UPhysicalMaterial*> PhysicalMaterials = BodyInstance->GetComplexPhysicalMaterials();

                NewTriangleMeshBuffer.MaterialDictionary.resize(PhysicalMaterials.Num());
                for (size_t MaterialIndex = 0; MaterialIndex < PhysicalMaterials.Num(); ++MaterialIndex)
                {
                    FString VortexMaterialName = FVortexRuntimeModule::Get().GetVortexMaterialFromPhysicalMaterial(PhysicalMaterials[MaterialIndex]);
                    VortexMaterial& VortexMaterial = NewTriangleMeshBuffer.MaterialDictionary[MaterialIndex];

                    strncpy_s(VortexMaterial.name, TCHAR_TO_UTF8(*VortexMaterialName), _countof(VortexMaterial.name) - 1);
                    VortexMaterial.name[_countof(VortexMaterial.name) - 1] = '\0';
                }
                shape.triangleMesh.materials = NewTriangleMeshBuffer.MaterialDictionary.data();
                shape.triangleMesh.materialsCount = uint8_t(PhysicalMaterials.Num());

                ++ResponseObject.shapeCount;
                MustSendCollision = true;
            }
        }
    }

    return MustSendCollision;
}

FVortexTerrain::ComponentKey FVortexTerrain::MakeKey(UPrimitiveComponent* Component, int32 InstanceID)
{
    ComponentKey Key = { Component->GetUniqueID(), InstanceID };
    return Key;
}

uint32 FVortexTerrain::GetOrGenerateUniqueId(const FVortexTerrain::ComponentKey& Key)
{
    bool IsIdAlreadyCreated = ComponentUniqueIds.Contains(Key);
    if (!IsIdAlreadyCreated)
    {
        ComponentUniqueIds.Add(Key, SequentialTerrainProviderID++);
    }
    return ComponentUniqueIds[Key];
}

FVortexTerrain::LandscapeComponentBuffer& FVortexTerrain::GetNextLandscapeComponentBuffer(int32 VerticesCount)
{
    return LandscapeBuffers[VerticesCount].GetNextBuffer();
}

void FVortexTerrain::RestartLandscapeComponentBuffersUsage()
{
    for (auto& Buffers : LandscapeBuffers)
    {
        Buffers.second.Reset();
    }
}


