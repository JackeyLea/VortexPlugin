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
#include "Engine/DeveloperSettings.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "VortexSettings.generated.h"


USTRUCT()
struct FMaterialMapping
{
public:
    GENERATED_USTRUCT_BODY()

    FMaterialMapping();

    UPROPERTY(EditAnywhere, Category = "Material Mapping", meta = (DisplayName = "Physical Material"))
    UPhysicalMaterial* PhysicalMaterial;

    UPROPERTY(EditAnywhere, Category = "Material Mapping", meta = (DisplayName = "Vortex Material Name"))
    FString VortexMaterialName;
};

//
// settings for the vortex application
//
UCLASS(config=Engine, defaultconfig, autoexpandcategories=("Vortex|Application Setup", "Vortex|Material Table", "Vortex|Static Collision"), meta = (DisplayName = "Vortex Studio Plugin"))
class VORTEXRUNTIME_API UVortexSettings : public UDeveloperSettings
{    
    GENERATED_BODY()

public:
    UVortexSettings();

    virtual void PostInitProperties() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
    virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Application Setup", meta = (DisplayName = "Use Default", ConfigRestartRequired = true))
    bool UseDefaultApplicationSetup;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Application Setup", meta = (FilePathFilter = "vxc", DisplayName = "Custom Application Setup", ConfigRestartRequired = true, EditCondition="!useDefaultApplicationSetup"))
    FFilePath ApplicationSetup;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Material Table", meta = (DisplayName = "Use Default", ConfigRestartRequired = true))
    bool UseDefaultMaterialTable;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Material Table", meta = (FilePathFilter = "vxmaterials", DisplayName = "Custom Material Table", ConfigRestartRequired = true, EditCondition = "!useDefaultMaterialTable"))
    FFilePath MaterialTableFilepath;

    UPROPERTY(config, EditAnywhere, Category = "Vortex", meta = (DisplayName = "Data Provider's Path", ConfigRestartRequired = true, RelativeToGameContentDir))
    FDirectoryPath DataProviderDirectoryPath;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision", meta = (DisplayName = "Enable Landscape Collision Detection", ConfigRestartRequired = true))
    bool EnableLandscapeCollisionDetection;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision", meta = (DisplayName = "Enable Mesh Simple Collision Detection", ConfigRestartRequired = true))
    bool EnableMeshSimpleCollisionDetection;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision", meta = (DisplayName = "Enable Mesh Complex Collision Detection", ConfigRestartRequired = true))
    bool EnableMeshComplexCollisionDetection;

    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision")
    TArray<FMaterialMapping> MaterialMappings;

    /// Terrain Paging Tile Size XY
    ///
    /// Size in meters, in world X and Y direction, of the square tiles used by the terrain pager for collision queries and caching.
    /// Terrain will be queried by the terrain pager in tiles of the given size around the dynamically moving Vortex mechanisms.
    /// Tiles will be cached and only queried again if not already present in the cache in order to reduce query time.
    ///
    /// Default: 50 m
    ///
    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision|Terrain Paging", meta = (DisplayName = "Tile Size XY", ConfigRestartRequired = true))
    double TerrainPagingTileSizeXY;

    /// Terrain Paging Look Ahead Time
    ///
    /// Specifies the time, in simulation seconds, the terrain pager should forecast ahead.
    /// All the terrain to be reached within the specified time, at the current velocity,
    /// will be requested.
    ///
    /// Default: 1 sec
    ///
    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision|Terrain Paging", meta = (DisplayName = "Look Ahead Time", ConfigRestartRequired = true))
    double TerrainPagingLookAheadTime;

    /// Terrain Paging Safety Band Size
    ///
    /// Specifies the distance (in meters) to grow the paged region by, as a safety band,
    /// to accomodate for objects changing direction.
    ///
    /// Default: 1 m
    ///
    UPROPERTY(config, EditAnywhere, Category = "Vortex|Static Collision|Terrain Paging", meta = (DisplayName = "Safety Band Size", ConfigRestartRequired = true))
    double TerrainPagingSafetyBandSize;

private:

    bool IsMaterialMappingErrorBeingShown;
};
