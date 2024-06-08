#pragma once

// Copyright (c) 2000-2019 CMLabs Simulations Inc.
// 
// http://www.cm-labs.com
// 
// This software and its accompanying manuals have been developed by CMLabs
// Simulations Inc. ("CMLabs").
// 
// The copyright to the Vortex Toolkits and all associated materials
// belongs to CMLabs.
// 
// All intellectual property rights in the software belong to CMLabs.
// 
// All rights conferred by law (including rights under international
// copyright conventions) are reserved to CMLabs. This software may also
// incorporate information which is confidential to CMLabs.
// 
// Save to the extent permitted by law, or as otherwise expressly permitted
// by CMLabs, this software and the manuals must not be copied (in whole or
// in part), re-arranged, altered or adapted in any way without the prior
// written consent of CMLabs. In addition, the information contained in the
// software may not be disseminated without the prior written consent of
// CMLabs.
// 

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /// This file defines a standard "C" API that can be used by an external application to embed Vortex.
    /// This interface should always remain Vortex agnostic, so external applications don't have to include any Vortex header and directly link with Vortex.
    ///
    /// By isolating Vortex with this "C" interface, it also allows to integrate safely with other applications using a different version of the C++ runtimes.
    ///

    /// Friendly name for a Vortex object handle. Corresponds to a VxSim::VxExtension*
    /// Used to access data of Vortex Objects
    ///
    typedef void* VortexObjectHandle;

    #define VortexNameSize 64
    #define VortexLongNameSize 256

    /// Vortex Material Definition
    /// @see VortexShape
    ///
    typedef struct VortexMaterial
    {
        /// Name of the material
        char name[VortexNameSize];
    } VortexMaterial;


    /// Vortex Height Field Definition
    /// @see VortexGetOutputHeightField
    /// @see VortexShape
    ///
    typedef struct VortexHeightField
    {
        /// Length of a cell along the X axis, in meters
        ///
        double cellSizeX;

        /// Length of a cell along the Y axis, in meters
        ///
        double cellSizeY;
        
        /// Array of heights for each vertices. The heights are listed by row (X axis). The size should be @ref nbVerticesX * @ref nbVerticesY
        ///
        const double* heights;

        /// Array of materials for each cells. There are 2 triangles per cell and this array is the material on the first triangle.
        /// The materials are listed by row (X axis) and refers to index on array materials. The size should be (@ref nbVerticesX - 1) * (@ref nbVerticesY - 1)
        ///
        const uint8_t* materials0;

        /// Array of materials for each cells. There are 2 triangles per cell and this array is the material on the second triangle.
        /// The materials are listed by row (X axis) and refers to index on array materials. The size should be ((@ref nbVerticesX - 1) * (@ref nbVerticesY - 1))
        ///
        const uint8_t* materials1;
        
        /// Number of vertices along the X axis
        ///
        uint32_t nbVerticesX;

        /// Number of vertices along the Y axis
        ///
        uint32_t nbVerticesY;

        /// Dictionary of all used materials in this heightfield
        ///
        const VortexMaterial* materials;
        
        /// Materials count of @ref materials
        ///
        uint8_t materialsCount;

    } VortexHeightField;


    /// Single tile of a Vortex Tiled Height Field Definition
    /// @see VortexGetOutputTiledHeightField
    ///
    typedef struct VortexTiledHeightFieldTile
    {
        /// Position along the X axis in the local space of VortexTiledHeightField, in meters.
        ///
        double tilePositionX;

        /// Position along the Y axis in the local space of VortexTiledHeightField, in meters.
        ///
        double tilePositionY;

        /// Array of heights for the tile. The heights are listed by row (X axis).
        ///
        const float* heights;

        /// State of the deformation. 0 means undisturbed, 1 is disturbed
        ///
        const uint16_t* landUseIds;

        /// Tile coordinate in X in VortexTiledHeightField.tiles or VortexTiledHeightField.stitchTiles
        ///
        uint32_t tileX;

        /// Tile coordinate in Y in VortexTiledHeightField.tiles or VortexTiledHeightField.stitchTiles
        ///
        uint32_t tileY;
    } VortexTiledHeightFieldTile;

    
    /// Vortex Tiled Height Field Definition
    /// @see VortexGetOutputTiledHeightField
    ///
    typedef struct VortexTiledHeightField
    {
        /// Length of a cell along the X axis, in meters
        ///
        double cellSizeX;
        
        /// Length of a cell along the Y axis, in meters
        ///
        double cellSizeY;

        /// Tiles in the height field
        ///
        VortexTiledHeightFieldTile* tiles;

        /// Tiles count in tiles
        ///
        uint32_t tileCount;

        /// Tiles count in tiles along the X axis
        ///
        uint32_t tileCountX;
        
        /// Tiles count in tiles along the Y axis
        ///
        uint32_t tileCountY;

        /// Number of vertices along the X axis
        ///
        uint32_t nbVerticesX;

        /// Number of vertices along the Y axis
        ///
        uint32_t nbVerticesY;

        /// On a call VortexGetOutputTiledHeightField for only modified tiles, those are the tiles adjacent to the modified tiles
        ///
        VortexTiledHeightFieldTile* stitchTiles;

        /// Tiles count in @ref stitchTileCount
        ///
        uint32_t stitchTileCount;

    } VortexTiledHeightField;


    /// Vortex Bounding Box Definition
    /// A box defined by opposing corners min and max.
    /// @see VortexTerrainProviderRequest
    typedef struct VortexBoundingBox
    {
        /// Minimum coordinates of the bounding box
        ///
        double min[3];

        /// Maximum coordinates of the bounding box
        ///
        double max[3];

    } VortexBoundingBox;

    /// Vortex Sphere Collision geometry
    /// @see VortexShape
    ///
    typedef struct VortexSphere
    {
        /// Radius of the sphere, in meters
        ///
        double radius;

        /// Material of the collision geometry
        ///
        VortexMaterial material;
    } VortexSphere;

    /// Vortex Box Collision geometry
    /// @see VortexShape
    ///
    typedef struct VortexBox
    {
        /// Size of the box in X, Y and Z, in meters
        ///
        double extents[3];

        /// Material of the collision geometry
        ///
        VortexMaterial material;
    } VortexBox;

    /// Vortex Box Collision geometry
    /// @see VortexShape
    ///
    typedef struct VortexCapsule
    {
        /// Radius of the capsule, in meters
        ///
        double radius;

        /// Length of the capsule, in meters
        ///
        double length;

        /// Material of the collision geometry
        ///
        VortexMaterial material;
    } VortexCapsule;

    /// Vortex Convex Mesh Collision geometry
    /// @see VortexShape
    ///
    typedef struct VortexConvex
    {
        /// Vertices coordinates, in X, Y and Z. There are @ref vertexCount*3 vertices
        ///
        const double* vertices;
        
        /// Number of vertices in @ref vertices
        ///
        uint32_t vertexCount;

        /// Material of the collision geometry
        ///
        VortexMaterial material;
    } VortexConvex;

    /// Vortex Triangle Mesh Collision geometry
    /// @see VortexShape
    ///
    typedef struct VortexTriangleMesh
    {
        /// Vertices coordinates, in X, Y and Z. There are @ref vertexCount*3 vertices
        ///
        const double* vertices;
        
        /// Number of vertices 
        ///
        uint32_t vertexCount;

        /// Indices of a triangle, by vertex. There are @ref triangleCount*3 indices.
        ///
        const uint32_t* indices;

        /// Number of triangles in the mesh.
        ///
        uint32_t triangleCount;
        
        /// Indices of the materials of a triangle in materials. There are @ref triangleCount items in materialPerTriangle
        ///
        const uint16_t* materialPerTriangle;

        /// Dictionary of all used materials in this triangle mesh
        ///
        VortexMaterial* materials;
        
        /// Materials count in @ref materials
        ///
        uint8_t materialsCount;
    } VortexTriangleMesh;

    /// Shape type for Terrain Provider object
    /// @see VortexShape
    /// 
    typedef enum VortexShapeType
    {
        kVortexSphere = 0,
        kVortexBox,
        kVortexCapsule,
        kVortexConvex,
        kVortexTriangleMesh,
        kVortexHeightField
    } VortexShapeType;

    /// Request for the Terrain Provider
    /// Coordinates for which terrain data is needed
    /// @see VortexTerrainProviderQuery
    /// 
    typedef struct VortexTerrainProviderRequest
    {
        /// Bounding Box, in World coordinates
        ///
        VortexBoundingBox bbox;
    } VortexTerrainProviderRequest;

    /// A single shape for an object for the Terrain Provider
    /// @see VortexTerrainProviderObject
    /// 
    typedef struct VortexShape
    {
        /// Position of the shape, in local X, Y, Z coordinates
        ///
        double position[3];

        /// Rotation quaternion of the shape, in local W, X, Y, Z coordinates
        ///
        double rotation[4];

        /// Type of the shape
        ///
        VortexShapeType shapeType;
        union
        {
            VortexSphere sphere;
            VortexBox box;
            VortexCapsule capsule;
            VortexConvex convex;
            VortexHeightField heightField;
            VortexTriangleMesh triangleMesh;
        };
    } VortexShape;

    /// An object for the Terrain Provider
    /// @see VortexTerrainProviderResponse
    /// 
    typedef struct VortexTerrainProviderObject
    {
        /// Name of the object for reference
        ///
        char name[VortexLongNameSize];

        /// Unique Id of the object for reference
        ///
        uint32_t uniqueID;

        /// Position of the object, in world X, Y, Z coordinates
        ///
        double position[3];
        
        /// Rotation quaternion of the object, in world W, X, Y, Z coordinates
        ///
        double rotation[4];

        /// Vortex shapes count, there can be no more than 8
        uint32_t shapeCount;
        
        /// Vortex shapes
        VortexShape shapes[8];
    } VortexTerrainProviderObject;

    /// A Terrain Provider response
    /// @see VortexTerrainProviderQuery
    /// 
    typedef struct VortexTerrainProviderResponse
    {
        /// Array of objects in the bounding box
        ///
        VortexTerrainProviderObject* objects;

        /// Object count in @ref objects array
        ///
        uint32_t objectCount;
    } VortexTerrainProviderResponse;


    /// A user object when Vortex Terrain Provider callbacks are invoked
    /// 
    typedef void* VortexTerrainProvider;

    /// Terrain Producer callback definition
    ///
    /// Terrain Producer Query
    /// A query from Vortex to get as it needs to produce terrain data, called during VortexUpdateApplication(). There can be multiple queries during a single update.
    /// @param VortexTerrainProvider user object
    /// @param VortexTerrainProviderRequest Request details
    /// @param VortexTerrainProviderResponse Response to the request. 
    ///
    /// @note VortexTerrainProviderResponse pointer is valid. User must ensure that the memory allocated for VortexTerrainProviderObject* stays valid until next call to @ref VortexTerrainProviderPostQuery
     ///@note Memory allocated to @ref VortexTerrainProviderResponse.objects will not be freed by Vortex Integration
    ///
    typedef void (*VortexTerrainProviderQuery)(VortexTerrainProvider, const VortexTerrainProviderRequest*, VortexTerrainProviderResponse*);
    
    /// Terrain Producer Post Query
    /// Called after all calls to VortexTerrainProviderQuery during @ref VortexUpdateApplication(). 
    /// @param VortexTerrainProvider user object
    ///
    /// @note Memory allocated for the VortexTerrainProviderResponse.objects can be freed.
    ///
    typedef void (*VortexTerrainProviderPostQuery)(VortexTerrainProvider);
    
    /// Terrain Producer on Destroy
    /// Called when the Terrain Producer is no longer needed. 
    /// @note Called when Vortex Application Mode change from simulating to editing.
    /// @note Called during @ref VortexDestroyApplication()
    ///
    /// @param VortexTerrainProvider user object
    ///
    typedef void (*VortexTerrainProviderOnDestroy)(VortexTerrainProvider);

    /// Terrain Producer Information
    /// Given as a parameter to @ref VortexCreateApplication()
    ///
    typedef struct VortexTerrainProviderInfo
    {
        /// User object
        ///
        VortexTerrainProvider terrainProvider;

        /// Terrain Producer Query callback
        ///
        VortexTerrainProviderQuery terrainProviderQuery;
        
        /// Terrain Producer Post Query callback
        ///
        VortexTerrainProviderPostQuery terrainProviderPostQuery;

        /// Terrain Producer On Destroy callback
        ///
        VortexTerrainProviderOnDestroy terrainProviderOnDestroy;

        /// Terrain Producer Tile Size UV
        ///
        double terrainProviderTileSizeUV;

        /// Terrain Producer Look Ahead Time
        ///
        double terrainProviderLookAheadTime;

        /// Terrain Producer Forecasting Safety Band Size
        ///
        double terrainProviderSafetyBandSize;
    } VortexTerrainProviderInfo;

    /// Vortex Particles representation
    /// @see getOutputParticles
    ///
    typedef struct VortexParticles
    {
        /// Positions of the particles, in X, Y, Z coordinates. The are 3*@ref count positions
        ///
        const float* positions;
        
        /// Radii of the particles, in meters
        ///
        const float* radii;

        /// Ids of the particles
        ///
        const int32_t* ids;
        
        /// Number of particles
        ///
        uint32_t count;
    } VortexParticles;

    /// Vortex UV Data
    /// @see VortexMeshData
    ///
    typedef struct VortexMeshUVData
    {
        const float* uvs;
        uint32_t uvCount;
        uint32_t uvSize;
    } VortexMeshUVData;

    /// Vortex Mesh Data
    /// @see VortexGetOutputGraphicsMesh
    /// @see VortexGetGraphicsMeshData
    ///
    typedef struct VortexMeshData
    {
        VortexMeshUVData uvs[3];
        const void* indexes;
        const float* vertices;
        const float* normals;
        const float* tangents;
        uint32_t indexCount;
        uint32_t indexSize;
        uint32_t vertexCount;
        uint32_t normalCount;
        uint32_t tangentCount;
        uint32_t padding;
        char name[VortexNameSize];
    } VortexMeshData;

    /// Update modes for Vortex Application
    /// 
    typedef enum VortexApplicationMode
    {
        /// Simulating mode is the default for VxApplication. In that mode, the 
        /// simulation is computed at each update. Objects can be moved kinematically, 
        /// but should not be altered fundamentally.
        ///
        kVortexModeSimulating = 0,

        /// Editing mode is used when modifying the content objects. Simulation 
        /// is not computed, but objects can be moved and modified. 
        ///
        kVortexModeEditing,

        /// Playback mode is used when replaying a recorded simulation. Simulation
        /// is not computed, but read from a file. Objects should not be moved or modified
        ///
        kVortexModePlayingBack
    } VortexApplicationMode;

    typedef enum VortexSynchronizationMode
    {
        /// No synchronisation : all nodes go as fast as possible
        ///
        kVortexSyncNone = 0,

        /// Application is synchronized with graphics: VSync is activated on the graphics card
        /// applications without graphics behave like kSyncNone
        ///
        kVortexSyncVSync,

        /// Synchronized with software sync inside VxApplication
        /// all applications are limited to the frame rate given to VxApplication
        ///
        kVortexSyncSoftware,

        /// Synchronized with software and VSync activated
        /// Combines the VxApplication frame rate with VSync
        ///
        kVortexSyncSoftwareAndVSync,

        /// Unknown sync type
        ///
        kVortexUnknownSyncType
    } VortexSynchronizationMode;

    /// Vortex Texture Data Format
    /// @see VortexTextureData
    ///
    typedef enum VortexTextureDataFormat
    {
        kVortexTextureFormatR8G8B8A8 = 0,
        kVortexTextureFormatB8G8R8A8,
        kVortexTextureFormatBC1RGB,
        kVortexTextureFormatBC1RGBA,
        kVortexTextureFormatBC2,
        kVortexTextureFormatBC3,
    } VortexTextureDataFormat;

    /// Vortex Texture Data
    /// @see VortexGetGraphicsTextureData
    /// @see VortexGetGraphicsTextureCopy
    ///
    typedef struct VortexTextureData
    {
        const uint8_t* bytes;
        uint32_t byteCount;
        int32_t sizeX;
        int32_t sizeY;
        VortexTextureDataFormat format;
        char name[VortexNameSize];
    } VortexTextureData;

    /// Vortex Material Layer Data
    /// @see VortexMaterialData
    ///
    typedef struct VortexMaterialLayerData
    {
        uint8_t color[4];
        VortexObjectHandle textureHandle;
    } VortexMaterialLayerData;

    /// Vortex Material Data
    /// @see VortexGetOutputGraphicsMaterial
    /// @see VortexGetGraphicsMaterialData
    ///
    typedef struct VortexMaterialData
    {
        VortexMaterialLayerData emissionLayers[3];
        VortexMaterialLayerData occlusionLayers[3];
        VortexMaterialLayerData albedoLayers[3];
        VortexMaterialLayerData specularLayers[3];
        VortexMaterialLayerData glossLayers[3];
        VortexMaterialLayerData metalnessLayers[3];
        VortexMaterialLayerData roughnessLayers[3];
        VortexMaterialLayerData normalLayers[3];
        VortexMaterialLayerData heightMapLayers[3];
        char name[VortexNameSize];
    } VortexMaterialData;

    /// Vortex Graphics Node Data
    /// @see VortexGetGraphicNodeData
    ///
    typedef struct VortexGraphicNodeData
    {
        double position[3];
        double scale[3];
        double rotation[4];
        uint64_t parentNodeContentID[2];
        uint64_t contentID[2];
        char name[VortexNameSize];
        bool hasGeometry;
        bool hasConnection;
    } VortexGraphicNodeData;

    /// The type of a Vortex field.
    ///
    typedef enum VortexFieldType
    {
        kVortexFieldTypeInput = 0,
        kVortexFieldTypeOutput, 
        kVortexFieldTypeParameter
    } VortexFieldType;

    /// The data type of a Vortex field.
    ///
    typedef enum VortexDataType
    {
        kVortexDataTypeUnknown = 0,
        kVortexDataTypeBoolean,
        kVortexDataTypeInt,
        kVortexDataTypeReal,
        kVortexDataTypeVector2,
        kVortexDataTypeVector3,
        kVortexDataTypeVector4,
        kVortexDataTypeMatrix,
        kVortexDataTypeString,
        kVortexDataTypeColor,
        kVortexDataTypeExtensionPointer
    } VortexDataType;

    /// This is used to signify the status of a Vortex field, i.e if it exists and is the correct data type.
    ///
    typedef enum VortexFieldStatus
    {
        kVortexFieldStatusOK = 0, // The field exists.
        kVortexFieldStatusNotFound, // A field with this name of this field type does not exist in the Vortex object.
        kVortexFieldStatusWrongDataType, // A field with this name of this field type exists but is not of the expected data type (e.g. it is an int not a real).
        kVortexFieldStatusInterfaceNotFound, // The interface could not be found.
        kVortexFieldStatusObjectNotFound // The Vortex object is not valid. 
    } VortexFieldStatus;

#ifdef __cplusplus
}
#endif