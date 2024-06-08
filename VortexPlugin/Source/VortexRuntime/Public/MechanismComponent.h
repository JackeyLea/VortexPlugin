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
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "VortexIntegration/VortexIntegration.h"
#include "VortexMechanism.h"
#include "MechanismComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("VortexMechanism"), STATGROUP_VortexMechanism, STATCAT_Advanced);

USTRUCT(BlueprintType)
struct FMechanismComponentMapping
{
    GENERATED_USTRUCT_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
    FString TransformFieldName;

    UPROPERTY(EditAnywhere, Category = "Vortex")
    FComponentReference Component;
};

USTRUCT(BlueprintType)
struct FMechanismComponentMappingSection
{
    GENERATED_USTRUCT_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
    FString VHLName;

    // Vortex mechanism component mappings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
    TArray<FMechanismComponentMapping> Mappings;
};

class FVortexRuntimeModule;

// Vortex mechanism component for unreal engine actors
UCLASS( ClassGroup=(Vortex), meta=(BlueprintSpawnableComponent) )
class VORTEXRUNTIME_API UMechanismComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UMechanismComponent();

protected:
    virtual void OnUnregister() override;
    virtual void OnRegister() override;
    virtual void PostRename(UObject* OldOuter, const FName OldName) override;

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Called when the game starts
    virtual void BeginPlay() override;

    // Called when the game ends
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Called when this component has been unregistered from the FVortexRuntimeModule. It allows to do some internal cleanup.
    void onComponentUnregistered();

public:    

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex")
    UVortexMechanism* VortexMechanism;

    // Mapping for Components that follow Vortex VHL transform fields
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
    TArray<FMechanismComponentMappingSection> ComponentMappings;

    // Get/Set for Boolean
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsBool(FString VHLName, FString FieldName, bool& Value);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsBool(FString VHLName, FString FieldName, bool Value);

    // Get/Set for Integer
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsInteger(FString VHLName, FString FieldName, int32& Value);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsInteger(FString VHLName, FString FieldName, int32 Value);

    // Get/Set for Float
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsFloat(FString VHLName, FString FieldName, float& Value);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsFloat(FString VHLName, FString FieldName, float Value);

    // Get/Set for Vector2
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsVector2(FString VHLName, FString FieldName, FVector2D& Value);
    
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsVector2(FString VHLName, FString FieldName, FVector2D Value);

    // Get/Set for Vector3
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsVector3(FString VHLName, FString FieldName, FVector& Value);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsVector3(FString VHLName, FString FieldName, FVector Value);

    // Get/Set for Vector4
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsVector4(FString VHLName, FString FieldName, FVector4& Value);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsVector4(FString VHLName, FString FieldName, FVector4 Value);

    // Get/Set for String
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsString(FString VHLName, FString FieldName, FString& Value);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsString(FString VHLName, FString FieldName, FString Value);

    // Get/Set for Transform
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsTransform(FString VHLName, FString FieldName, FTransform& Transform);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|set")
    void SetVHLFieldAsTransform(FString VHLName, FString FieldName, FTransform Transform);

    // Get for IMobile
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsIMobile(FString VHLName, FString FieldName, FTransform& Transform);

    // Get for Vortex HeightField
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsHeightField(FString VHLName, FString FieldName, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UV0, TArray<FProcMeshTangent>& Tangents);

    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsTiledHeightField(FString VHLName, FString FieldName, UProceduralMeshComponent* ProceduralMesh, bool bUpdate, bool& bOK);

    // Get for Vortex Particles
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsParticles(FString VHLName, FString FieldName, TArray<FVector>& Positions, TArray<float>& Radii, TArray<int32>& IDs);

    // Get for Vortex Graphics Mesh
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsGraphicsMesh(FString VHLName, FString FieldName,
        TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UV0, TArray<FVector2D>& UV1, TArray<FVector2D>& UV2, TArray<FProcMeshTangent>& Tangents);

    // Get for Vortex Graphics Mesh
    UFUNCTION(BlueprintCallable, Category = "Vortex|VHL|get")
    void GetVHLFieldAsGraphicsMaterial(FString VHLName, FString FieldName,
        UTexture2D*& EmissionColor0, UTexture2D*& EmissionColor1, UTexture2D*& EmissionColor2,
        UTexture2D*& OcclusionColor0, UTexture2D*& OcclusionColor1, UTexture2D*& OcclusionColor2,
        UTexture2D*& AlbedoColor0, UTexture2D*& AlbedoColor1, UTexture2D*& AlbedoColor2,
        UTexture2D*& SpecularColor0, UTexture2D*& SpecularColor1, UTexture2D*& SpecularColor2,
        UTexture2D*& GlossColor0, UTexture2D*& GlossColor1, UTexture2D*& GlossColor2,
        UTexture2D*& MetalnessColor0, UTexture2D*& MetalnessColor1, UTexture2D*& MetalnessColor2,
        UTexture2D*& RoughnessColor0, UTexture2D*& RoughnessColor1, UTexture2D*& RoughnessColor2,
        UTexture2D*& NormalColor0, UTexture2D*& NormalColor1, UTexture2D*& NormalColor2,
        UTexture2D*& HeightMapColor0, UTexture2D*& HeightMapColor1, UTexture2D*& HeightMapColor2);

private:
    friend class FVortexRuntimeModule;

    bool PreValidateVHLFunction(const FString& FunctionName, const FString& FilePath, const FString& VHLName, const FString& FieldName);
    void LogErrorForVHLFunction(const FString& FunctionName, const FString& FilePath, VortexObjectHandle objectHandle, const FString& VHLName, const FString& FieldName, VortexFieldType fieldType, VortexDataType dataType);

    FString LoadedMechanismKey;
    VortexObjectHandle VortexObject;

    TArray<VortexObjectHandle> GraphicNodeObjectHandles;
    TArray<USceneComponent*> GraphicNodeSceneComponentsTwins;
};

UCLASS()
class VORTEXRUNTIME_API UMechanismComponentMappingBlueprint : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Vortex|Component Mapping")
    static void SetComponent(const FMechanismComponentMapping& MappingRef, FMechanismComponentMapping& Mapping, USceneComponent* SceneComponent);

    UFUNCTION(BlueprintCallable, Category = "Vortex|Component Mapping")
    static void Set(const FMechanismComponentMapping& MappingRef, FMechanismComponentMapping& Mapping, AActor* Actor, FName ComponentProperty);
};