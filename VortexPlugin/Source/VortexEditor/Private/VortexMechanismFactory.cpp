#include "VortexMechanismFactory.h"
#include "ActorFactories/ActorFactoryEmptyActor.h"
#include "AssetImportTask.h"
#include "AssetSelection.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetTypeCategories.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include "Misc/Paths.h"
#include "RawMesh.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/MeshDescription/Public/MeshDescription.h"
#include "Runtime/MeshDescription/Public/MeshAttributes.h"
#include "Runtime/MeshDescription/Public/MeshAttributeArray.h"
#include "Runtime/StaticMeshDescription/Public/StaticMeshAttributes.h"
#include "ComponentReregisterContext.h"
#include "UObject/ConstructorHelpers.h"
#include "VortexIntegrationUtilities.h"
#include "VortexRuntime.h"
#include "VortexSettings.h"
#include <string>
#include <vector>
#include <cstdio>
#include <functional>

UVortexMechanismFactory::UVortexMechanismFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{    
    //Can we create a new mechanism without a file
    bCreateNew = false;
    //Can we use the import button
    bEditorImport = true;
    //Is the source file text (Vortex mechanisms are binary data)
    bText = false;

    bEditAfterNew = true;

    Formats.Add("vxmechanism;Vortex Mechanism");

    SupportedClass = UVortexMechanism::StaticClass();
}

UObject* UVortexMechanismFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    UVortexMechanism* vortexMechanismAsset = NewObject<UVortexMechanism>(InParent, InClass, InName, Flags);
    vortexMechanismAsset->AutomatedMappingImport = true;
    vortexMechanismAsset->MechanismFilepath.FilePath = Filename;
    FPaths::MakePathRelativeTo(vortexMechanismAsset->MechanismFilepath.FilePath, *FPaths::ProjectContentDir());

    UBlueprint* importedMechanismBlueprint = FKismetEditorUtilities::CreateBlueprintUsingAsset(vortexMechanismAsset, true);

    AActor* RootActorContainer = nullptr;
    USceneComponent* ActorRootComponent = nullptr;

    UActorFactory* Factory = GEditor->FindActorFactoryByClass(UActorFactoryEmptyActor::StaticClass());
    FAssetData EmptyActorAssetData = FAssetData(Factory->GetDefaultActorClass(FAssetData()));
    UObject* EmptyActorAsset = EmptyActorAssetData.GetAsset();
    RootActorContainer = FActorFactoryAssetProxy::AddActorForAsset(EmptyActorAsset, false, EObjectFlags::RF_Transactional, Factory);
    ActorRootComponent = NewObject<USceneComponent>(RootActorContainer, USceneComponent::GetDefaultSceneRootVariableName());
    ActorRootComponent->Mobility = EComponentMobility::Movable;
    RootActorContainer->SetRootComponent(ActorRootComponent);
    RootActorContainer->AddInstanceComponent(ActorRootComponent);
    ActorRootComponent->RegisterComponent();
    RootActorContainer->SetActorLabel("TMP"+InName.ToString());
    RootActorContainer->SetFlags(RF_Transactional);
    ActorRootComponent->SetFlags(RF_Transactional);

    if (importedMechanismBlueprint && FVortexRuntimeModule::IsIntegrationLoaded())
    {
        // Create some sub-packages
        const FString MaterialsPackageName = FPackageName::GetLongPackagePath(InParent->GetName()) + TEXT("/") + InName.ToString() + TEXT("Materials");
        const FString TexturesPackageName = FPackageName::GetLongPackagePath(InParent->GetName()) + TEXT("/") + InName.ToString() + TEXT("Textures");
        const FString MeshesPackageName = FPackageName::GetLongPackagePath(InParent->GetName()) + TEXT("/") + InName.ToString() + TEXT("Meshes");

        FString absoluteFilepath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), vortexMechanismAsset->MechanismFilepath.FilePath);

        double translation[3];
        double rotation[4];

        VortexIntegrationUtilities::ConvertTransform(FTransform(), translation, rotation);
        VortexObjectHandle VortexObject = VortexLoadMechanism(TCHAR_TO_UTF8(*absoluteFilepath), translation, rotation);

        // get count
        std::uint32_t MaterialHandleCount = 0;
        VortexGetGraphicsMaterialHandles(VortexObject, nullptr, &MaterialHandleCount);
        // get handles
        std::vector<VortexObjectHandle> MaterialHandleArray(MaterialHandleCount, nullptr);
        VortexGetGraphicsMaterialHandles(VortexObject, MaterialHandleArray.data(), &MaterialHandleCount);

        // get count
        std::uint32_t MeshHandleCount = 0;
        VortexGetGraphicsMeshHandles(VortexObject, nullptr, &MeshHandleCount);
        // get handles
        std::vector<VortexObjectHandle> MeshHandleArray(MeshHandleCount, nullptr);
        VortexGetGraphicsMeshHandles(VortexObject, MeshHandleArray.data(), &MeshHandleCount);

        if ((MaterialHandleCount > 0 || MeshHandleCount > 0))
        {
            UPackage* MaterialsPackage = CreatePackage(*MaterialsPackageName);
            UPackage* TexturesPackage = CreatePackage(*TexturesPackageName);
            UPackage* MeshesPackage = CreatePackage(*MeshesPackageName);

            // Create a unique set of all textures used in materials
            TMap<VortexObjectHandle, UTexture2D*> TextureHandleMap;
            for (std::uint32_t k = 0; k < MaterialHandleArray.size() && k < MaterialHandleCount; ++k)
            {
                VortexObjectHandle MaterialHandle = MaterialHandleArray[k];
                VortexMaterialData MaterialData = {};
                if (VortexGetGraphicsMaterialData(MaterialHandle, &MaterialData))
                {
                    for (auto& Layer : MaterialData.emissionLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.occlusionLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.albedoLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.specularLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.glossLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.metalnessLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.roughnessLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.normalLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                    for (auto& Layer : MaterialData.heightMapLayers)
                    {
                        if (Layer.textureHandle != nullptr)
                        {
                            TextureHandleMap.Add(Layer.textureHandle, nullptr);
                        }
                    }
                }
            }

            // Textures
            for (auto& pair : TextureHandleMap)
            {
                VortexTextureData TextureData = {};
                // get byte count
                if (VortexGetGraphicsTextureCopy(pair.Key, &TextureData))
                {
                    if (TextureData.sizeX > 0 && TextureData.sizeY > 0)
                    {
                        UTexture2D* Texture = NewObject<UTexture2D>(TexturesPackage, *FString(TextureData.name), RF_Public | RF_Standalone);
                        
                        Texture->PreEditChange(nullptr);

                        Texture->PlatformData = new FTexturePlatformData();
                        Texture->PlatformData->SizeX = TextureData.sizeX;
                        Texture->PlatformData->SizeY = TextureData.sizeY;
                        Texture->PlatformData->PixelFormat = PF_B8G8R8A8;

                        // Allocate first mipmap.
                        FTexture2DMipMap* Mip = new FTexture2DMipMap();
                        Texture->PlatformData->Mips.Add(Mip);
                        Mip->SizeX = TextureData.sizeX;
                        Mip->SizeY = TextureData.sizeY;
                        Mip->BulkData.Lock(LOCK_READ_WRITE);
                        uint8* Data = (uint8*)Mip->BulkData.Realloc(TextureData.byteCount);

                        // get uncompressed bytes
                        TextureData.bytes = Data;
                        VortexGetGraphicsTextureCopy(pair.Key, &TextureData);
                        Texture->Source.Init(TextureData.sizeX, TextureData.sizeY, 1, 1, ETextureSourceFormat::TSF_BGRA8, TextureData.bytes);
                        Mip->BulkData.Unlock();

                        // PostEditChange() MUST be called BEFORE UpdateResource(). Otherwise, the internal Texture's build settings won't be properly updated for the UpdateResource().
                        // As an example, it is causing a crash when the source texture has a width or height that isn't a power of 2, because it will try to create multiple mips in the mip map and this will fail.
                        Texture->PostEditChange();

                        Texture->UpdateResource();

                        pair.Value = Texture;

                        // Notify asset registry of new asset
                        FAssetRegistryModule::AssetCreated(Texture);
                    }
                }
            }

            // Base Material, only one per import.
            UMaterial* BaseGraphicsMaterial = nullptr;
            {
                auto MaterialFactory = NewObject<UMaterialFactoryNew>();
                BaseGraphicsMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), MaterialsPackage, TEXT("BaseGraphicsMaterial"), RF_Standalone | RF_Public, NULL, GWarn);

                // Let the material update itself if necessary
                BaseGraphicsMaterial->PreEditChange(NULL);

                // Make texture sampler
                UMaterialExpressionTextureSampleParameter2D* TextureExpression = NewObject<UMaterialExpressionTextureSampleParameter2D>(BaseGraphicsMaterial);
                TextureExpression->SetEditableName(TEXT("albedo"));
                TextureExpression->SamplerType = SAMPLERTYPE_Color;
                BaseGraphicsMaterial->Expressions.Add(TextureExpression);
                BaseGraphicsMaterial->BaseColor.Expression = TextureExpression;

                BaseGraphicsMaterial->PostEditChange();

                FAssetRegistryModule::AssetCreated(BaseGraphicsMaterial);

                // make sure that any static meshes, etc using this material will stop using the FMaterialResource of the original
                // material, and will use the new FMaterialResource created when we make a new UMaterial in place
                FGlobalComponentReregisterContext RecreateComponents;
            }

            // Material instances, multiple.
            for (std::uint32_t k = 0; k < MaterialHandleArray.size() && k < MaterialHandleCount; ++k)
            {
                VortexObjectHandle MaterialHandle = MaterialHandleArray[k];
                VortexMaterialData MaterialData = {};
                if (VortexGetGraphicsMaterialData(MaterialHandle, &MaterialData))
                {
                    const UVortexSettings* Settings = GetDefault<UVortexSettings>();
                    if (BaseGraphicsMaterial != nullptr)
                    {
                        // Create an unreal material instance from the template default in project properties
                        auto MaterialFactory = NewObject<UMaterialInstanceConstantFactoryNew>();
                        MaterialFactory->InitialParent = BaseGraphicsMaterial;
                        UMaterialInstanceConstant* UnrealMaterial = (UMaterialInstanceConstant*)MaterialFactory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), MaterialsPackage, *FString(MaterialData.name), RF_Standalone | RF_Public, nullptr, GWarn);

                        // Let the material update itself if necessary
                        UnrealMaterial->PreEditChange(nullptr);

                        // albedo only for now
                        if (MaterialData.albedoLayers[0].textureHandle != nullptr)
                        {
                            UnrealMaterial->SetTextureParameterValueEditorOnly(FName("albedo"), TextureHandleMap[MaterialData.albedoLayers[0].textureHandle]);
                        }

                        UnrealMaterial->PostEditChange();

                        // Notify asset registry of new asset
                        FAssetRegistryModule::AssetCreated(UnrealMaterial);
                    }
                }
            }

            // Geometry
            for (std::uint32_t k = 0; k < MeshHandleArray.size() && k < MeshHandleCount; ++k)
            {
                VortexObjectHandle MeshHandle = MeshHandleArray[k];

                // get count
                std::uint32_t MeshDataCount = 0;
                VortexGetGraphicsMeshData(MeshHandle, nullptr, &MeshDataCount);
                // get data
                std::vector<VortexMeshData> MeshDataArray(MeshDataCount, VortexMeshData{});
                VortexGetGraphicsMeshData(MeshHandle, MeshDataArray.data(), &MeshDataCount);
                if (MeshDataCount > 0)
                {
                    FMeshDescription MeshDescription;
                    FStaticMeshAttributes AttributeGetter(MeshDescription);
                    AttributeGetter.Register();

                    TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
                    TVertexAttributesRef<FVector> VertexPositions = AttributeGetter.GetVertexPositions();
                    TVertexInstanceAttributesRef<FVector> Tangents = AttributeGetter.GetVertexInstanceTangents();
                    TVertexInstanceAttributesRef<float> BinormalSigns = AttributeGetter.GetVertexInstanceBinormalSigns();
                    TVertexInstanceAttributesRef<FVector> Normals = AttributeGetter.GetVertexInstanceNormals();
                    TVertexInstanceAttributesRef<FVector4> Colors = AttributeGetter.GetVertexInstanceColors();
                    TVertexInstanceAttributesRef<FVector2D> UVs = AttributeGetter.GetVertexInstanceUVs();

                    std::vector<FPolygonGroupID> FPolygonGroupIDs;

                    // Materials to apply to new mesh
                    int32 VertexCount = 0;
                    int32 VertexInstanceCount = 0;
                    int32 PolygonCount = 0;
                    //Get all the info we need to create the MeshDescription
                    for (std::uint32_t j = 0; j < MeshDataCount && j < MeshDataArray.size(); ++j)
                    {
                        const VortexMeshData& MeshData = MeshDataArray[j];

                        VertexCount += MeshData.vertexCount;
                        VertexInstanceCount += MeshData.indexCount;
                        PolygonCount += MeshData.indexCount / 3;

                        FPolygonGroupIDs.push_back(MeshDescription.CreatePolygonGroup());
                    }

                    MeshDescription.ReserveNewVertices(VertexCount);
                    MeshDescription.ReserveNewVertexInstances(VertexInstanceCount);
                    MeshDescription.ReserveNewPolygons(PolygonCount);
                    MeshDescription.ReserveNewEdges(PolygonCount * 2);
                    UVs.SetNumIndices(1);
                    //Add Vertex and VertexInstance and polygon for each section
                    for (std::uint32_t j = 0; j < MeshDataCount && j < MeshDataArray.size(); ++j)
                    {
                        const VortexMeshData& MeshData = MeshDataArray[j];

                        //Create the vertex
                        int32 NumVertex = MeshData.vertexCount;
                        TMap<int32, FVertexID> VertexIndexToVertexID;
                        VertexIndexToVertexID.Reserve(NumVertex);
                        for (int32 VertexIndex = 0; VertexIndex < NumVertex; ++VertexIndex)
                        {
                            double pos[3] = { MeshData.vertices[VertexIndex * 3], MeshData.vertices[VertexIndex * 3 + 1], MeshData.vertices[VertexIndex * 3 + 2] };

                            const FVertexID VertexID = MeshDescription.CreateVertex();
                            VertexPositions[VertexID] = VortexIntegrationUtilities::ConvertTranslation(pos);
                            VertexIndexToVertexID.Add(VertexIndex, VertexID);
                        }
                        //Create the VertexInstance
                        int32 NumIndices = MeshData.indexCount;
                        int32 NumTri = NumIndices / 3;
                        TMap<int32, FVertexInstanceID> IndiceIndexToVertexInstanceID;
                        IndiceIndexToVertexInstanceID.Reserve(NumVertex);
                        const std::uint8_t* indices = reinterpret_cast<const std::uint8_t*>(MeshData.indexes);
                        for (int32 IndiceIndex = 0; IndiceIndex < NumIndices; IndiceIndex++)
                        {
                            uint32_t VertexIndex = 0;
                            switch (MeshData.indexSize)
                            {
                            case 4:
                                VertexIndex = *reinterpret_cast<const std::uint32_t*>(&indices[IndiceIndex * 4]);
                                break;
                            case 2:
                                VertexIndex = *reinterpret_cast<const std::uint16_t*>(&indices[IndiceIndex * 2]);
                                break;
                            case 1:
                                VertexIndex = *reinterpret_cast<const std::uint8_t*>(&indices[IndiceIndex * 1]);
                                break;
                            }

                            const FVertexID VertexID = VertexIndexToVertexID[VertexIndex];
                            const FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);
                            IndiceIndexToVertexInstanceID.Add(IndiceIndex, VertexInstanceID);

                            FVector TangentX(1, 0, 0);
                            FVector TangentY(0, 1, 0);
                            FVector TangentZ(0, 0, 1);

                            if (MeshData.normalCount > 0)
                            {
                                double Normal[3] = {
                                    MeshData.normals[VertexIndex * 3],
                                    MeshData.normals[VertexIndex * 3 + 1],
                                    MeshData.normals[VertexIndex * 3 + 2] };
                                TangentZ = VortexIntegrationUtilities::ConvertDirection(Normal);

                                if (MeshData.tangentCount > 0)
                                {
                                    double Tangent[3] = {
                                        MeshData.tangents[VertexIndex * 3],
                                        MeshData.tangents[VertexIndex * 3 + 1],
                                        MeshData.tangents[VertexIndex * 3 + 2] };
                                    TangentX = VortexIntegrationUtilities::ConvertDirection(Tangent);
                                }
                            }
                                                        
                            Tangents[VertexInstanceID] = TangentX;
                            Normals[VertexInstanceID] = TangentZ;
                            BinormalSigns[VertexInstanceID] = 1;

                            Colors[VertexInstanceID] = FLinearColor(FColor(255, 255, 255, 255));

                            FVector2D TexCoord(0, 0);

                            if (MeshData.uvs[0].uvCount > 0)
                            {
                                TexCoord = FVector2D(
                                    MeshData.uvs[0].uvs[VertexIndex * MeshData.uvs[0].uvSize],
                                    MeshData.uvs[0].uvs[VertexIndex * MeshData.uvs[0].uvSize + 1]);
                            }

                            UVs.Set(VertexInstanceID, 0, TexCoord);
                        }

                        //Create the polygons for this section
                        for (int32 TriIdx = 0; TriIdx < NumTri; TriIdx++)
                        {
                            FVertexID VertexIndexes[3];
                            TArray<FVertexInstanceID> VertexInstanceIDs;
                            VertexInstanceIDs.SetNum(3);

                            for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
                            {
                                const int32 IndiceIndex = (TriIdx * 3) + CornerIndex;

                                uint32_t VertexIndex = 0;
                                switch (MeshData.indexSize)
                                {
                                case 4:
                                    VertexIndex = *reinterpret_cast<const std::uint32_t*>(&indices[IndiceIndex * 4]);
                                    break;
                                case 2:
                                    VertexIndex = *reinterpret_cast<const std::uint16_t*>(&indices[IndiceIndex * 2]);
                                    break;
                                case 1:
                                    VertexIndex = *reinterpret_cast<const std::uint8_t*>(&indices[IndiceIndex * 1]);
                                    break;
                                }

                                VertexIndexes[CornerIndex] = VertexIndexToVertexID[VertexIndex];
                                VertexInstanceIDs[CornerIndex] = IndiceIndexToVertexInstanceID[IndiceIndex];
                            }

                            // discard degenerates
                            if (VertexIndexes[0] == VertexIndexes[1] ||
                                VertexIndexes[1] == VertexIndexes[2] ||
                                VertexIndexes[0] == VertexIndexes[2])
                            {
                                continue;
                            }

                            // Insert a polygon into the mesh
                            MeshDescription.CreatePolygon(FPolygonGroupIDs[j], VertexInstanceIDs);
                        }
                    }

                    // If we got some valid data.
                    if (MeshDescription.Polygons().Num() > 0)
                    {
                        // Create StaticMesh object
                        UStaticMesh* StaticMesh = NewObject<UStaticMesh>(MeshesPackage, *FString(MeshDataArray[0].name), RF_Public | RF_Standalone);
                        StaticMesh->InitResources();

                        StaticMesh->LightingGuid = FGuid::NewGuid();

                        // Add source to new StaticMesh
                        FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
                        SrcModel.BuildSettings.bRecomputeNormals = false;
                        SrcModel.BuildSettings.bRecomputeTangents = false;
                        SrcModel.BuildSettings.bRemoveDegenerates = false;
                        SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
                        SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
                        SrcModel.BuildSettings.bGenerateLightmapUVs = true;
                        SrcModel.BuildSettings.SrcLightmapIndex = 0;
                        SrcModel.BuildSettings.DstLightmapIndex = 1;
                        StaticMesh->CreateMeshDescription(0, MoveTemp(MeshDescription));
                        StaticMesh->CommitMeshDescription(0);

                        StaticMesh->StaticMaterials.Add(FStaticMaterial());

                        //Set the Imported version before calling the build
                        StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

                        // Build mesh from source
                        StaticMesh->Build(false);
                        StaticMesh->PostEditChange();

                        // Notify asset registry of new asset
                        FAssetRegistryModule::AssetCreated(StaticMesh);
                    }
                }
            }

            MaterialsPackage->MarkPackageDirty();
            TexturesPackage->MarkPackageDirty();
            MeshesPackage->MarkPackageDirty();
        }

        // get node count
        std::uint32_t MaterialNodeCount = 0;
        VortexGetGraphicsNodeHandles(VortexObject, nullptr, &MaterialNodeCount);
        // get node handles
        std::vector<VortexObjectHandle> GraphicsNodeArray(MaterialNodeCount, nullptr);
        VortexGetGraphicsNodeHandles(VortexObject, GraphicsNodeArray.data(), &MaterialNodeCount);

        for (VortexObjectHandle node : GraphicsNodeArray)
        {
            VortexGraphicNodeData nodeData = {};
            VortexGetGraphicNodeData(node, &nodeData);

            USceneComponent* SceneComponent;

            VortexMeshData meshData;
            std::uint32_t one = 1;
            if (VortexGetGraphicsMeshData(node, &meshData, &one))
            {
                nodeData.hasGeometry = true;
                
                FMechanismGraphicNodeMapping& mapping = CreateMappingWithUniqueName(vortexMechanismAsset, nodeData);

                UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(ActorRootComponent, mapping.GraphicsNodeName);

                VortexMaterialData materialData;
                FString materialName;
                if (VortexGetGraphicsMaterialData(node, &materialData))
                {
                    materialName = UTF8_TO_TCHAR(materialData.name);
                }

                FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                TArray<FAssetData> MeshesData;
                AssetRegistryModule.Get().GetAssetsByPackageName(FName(*MeshesPackageName), MeshesData);
                for (FAssetData assetDatum : MeshesData)
                {
                    if (assetDatum.AssetName == UTF8_TO_TCHAR(meshData.name))
                    {
                        if (UStaticMesh * mesh = Cast<UStaticMesh>(assetDatum.GetAsset()))
                        {
                            StaticMeshComponent->SetStaticMesh(mesh);
                        }
                    }
                }

                TArray<FAssetData> MaterialsData;
                AssetRegistryModule.Get().GetAssetsByPackageName(FName(*MaterialsPackageName), MaterialsData);
                for (FAssetData assetDatum : MaterialsData)
                {
                    if (assetDatum.AssetName == FName(*materialName))
                    {
                        if (UMaterialInstance* materialInstance = Cast<UMaterialInstance>(assetDatum.GetAsset()))
                        {
                            StaticMeshComponent->SetMaterial(0, materialInstance);
                        }
                    }
                }

                SceneComponent = StaticMeshComponent;
            }
            else
            {
                FMechanismGraphicNodeMapping& mapping = CreateMappingWithUniqueName(vortexMechanismAsset, nodeData);

                SceneComponent = NewObject<USceneComponent>(ActorRootComponent, mapping.GraphicsNodeName);
            }
            
            SceneComponent->SetRelativeScale3D(FVector(nodeData.scale[0], nodeData.scale[1], nodeData.scale[2]));
            SceneComponent->SetRelativeLocation(VortexIntegrationUtilities::ConvertTranslation(nodeData.position));
            SceneComponent->SetRelativeRotation(VortexIntegrationUtilities::ConvertRotation(nodeData.rotation));

            SceneComponent->Mobility = EComponentMobility::Movable;

            RootActorContainer->AddInstanceComponent(SceneComponent);
            SceneComponent->RegisterComponent();

            //find parent name in the mappings in vortexMechanismAsset->GraphicNodeMappings
            FName parentName;
            for (const FMechanismGraphicNodeMapping& mapping : vortexMechanismAsset->GraphicNodeMappings)
            {
                uint64 mappingContentID[2] = { ((uint64)mapping.ContentID.id00 & 0xFFFFFFFF) | ((uint64)mapping.ContentID.id01) << 32,
                                               ((uint64)mapping.ContentID.id10 & 0xFFFFFFFF) | ((uint64)mapping.ContentID.id11) << 32 };
                if (mappingContentID[0] == nodeData.parentNodeContentID[0] && mappingContentID[1] == nodeData.parentNodeContentID[1])
                {
                    parentName = mapping.GraphicsNodeName;
                    break;
                }
            }

            //find parent using the name obtained from the contentID
            USceneComponent* parent = ActorRootComponent;
            for (auto component : RootActorContainer->GetInstanceComponents())
            {               
                if (component->GetFName() == parentName)
                {
                    parent = Cast<USceneComponent>(component);
                    break;
                }
            }
            SceneComponent->AttachToComponent(parent, FAttachmentTransformRules::KeepRelativeTransform);
        }
        FKismetEditorUtilities::FAddComponentsToBlueprintParams AddComponentsToBlueprintParams;
        AddComponentsToBlueprintParams.HarvestMode = FKismetEditorUtilities::EAddComponentToBPHarvestMode::None;
        AddComponentsToBlueprintParams.OptionalNewRootNode = nullptr;
        AddComponentsToBlueprintParams.bKeepMobility = true;
        FKismetEditorUtilities::AddComponentsToBlueprint(importedMechanismBlueprint, RootActorContainer->GetInstanceComponents(), AddComponentsToBlueprintParams);
        FKismetEditorUtilities::CompileBlueprint(importedMechanismBlueprint);

        UWorld* World = RootActorContainer->GetWorld();
        World->DestroyActor(RootActorContainer);
        GEngine->BroadcastLevelActorListChanged();

        VortexUnloadMechanism(VortexObject);
    }

    return vortexMechanismAsset;
}

bool UVortexMechanismFactory::FactoryCanImport(const FString& Filename)
{
    const FString Extension = FPaths::GetExtension(Filename);

    if (Extension == TEXT("vxmechanism"))
    {
        return true;
    }
    return false;
}

uint32 UVortexMechanismFactory::GetMenuCategories() const
{
    return EAssetTypeCategories::Physics;
}

FText UVortexMechanismFactory::GetDisplayName() const
{
    return FText();
}

FMechanismGraphicNodeMapping& UVortexMechanismFactory::CreateMappingWithUniqueName(UVortexMechanism* vortexMechanismAsset, VortexGraphicNodeData& nodeData)
{
    //Determine a unique name
    int suffix = 0;
    FString nameWithSuffix = UTF8_TO_TCHAR(nodeData.name); //suffix 0 is not displayed
    for (const FMechanismGraphicNodeMapping& mapping : vortexMechanismAsset->GraphicNodeMappings)
    {
        if (mapping.GraphicsNodeName.ToString() == nameWithSuffix)
        {
            ++suffix;
            nameWithSuffix = UTF8_TO_TCHAR(nodeData.name) + FString("_") + FString::FromInt(suffix);
            //This assumes that names are sorted by order of suffix. Otherwise, we would need to restart the loop!
        }
    }

    //Add a mapping for nodeData.contentID to nodeData.name
    FMechanismGraphicNodeMapping mapping;
    mapping.ContentID.id00 = uint32(nodeData.contentID[0] & 0xFFFFFFFF);
    mapping.ContentID.id01 = uint32(nodeData.contentID[0] >> 32);
    mapping.ContentID.id10 = uint32(nodeData.contentID[1] & 0xFFFFFFFF);
    mapping.ContentID.id11 = uint32(nodeData.contentID[1] >> 32);
    mapping.GraphicsNodeName = FName(*nameWithSuffix);
    vortexMechanismAsset->GraphicNodeMappings.Add(mapping);

    return vortexMechanismAsset->GraphicNodeMappings.Last();
}

