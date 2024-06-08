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
#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "UObject/NoExportTypes.h"
#include "VortexMechanism.generated.h"

USTRUCT(BlueprintType)
struct FContentID
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex")
    int32 id00; //because int64 is not supported by blueprint

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex")
    int32 id01; //because int64 is not supported by blueprint

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex")
    int32 id10; //because int64 is not supported by blueprint

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex")
    int32 id11; //because int64 is not supported by blueprint
};

USTRUCT(BlueprintType)
struct FMechanismGraphicNodeMapping
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vortex")
    FContentID ContentID;

    // Vortex mechanism component mappings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
    FName GraphicsNodeName;
};

//
// uasset for vortex mechanisms
//

UCLASS(Blueprintable)
class VORTEXRUNTIME_API UVortexMechanism : public UObject
{
    GENERATED_BODY()
public:
    UVortexMechanism(const FObjectInitializer& ObjectInitializer);
    virtual void PostLoad() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif
    UPROPERTY()
    FString filepath_DEPRECATED;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
    FFilePath MechanismFilepath;

    UPROPERTY(BlueprintReadOnly, Category = "Properties")
    bool AutomatedMappingImport;

    // Mapping for Vortex Graphic Nodes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
    TArray<FMechanismGraphicNodeMapping> GraphicNodeMappings;
};
