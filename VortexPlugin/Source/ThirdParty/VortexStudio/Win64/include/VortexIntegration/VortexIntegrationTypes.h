#pragma once

// Copyright (c) 2020 CMLabs Simulations Inc.
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

#include <VortexIntegration/Structs.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    /// This file defines a standard "C" API that can be used by an external application to integrate with Vortex graphics layer.
    /// This interface should always remain Vortex agnostic, so external applications don't have to include any Vortex header and directly link with Vortex.
    ///
    /// By isolating Vortex with this "C" interface, it also allows to integrate safely with other applications using a different version of the C++ runtimes.
    ///

    /// HUD Rotation
    ///
    enum HUDRotation
    {
        HUDRotation_0,
        HUDRotation_90,
        HUDRotation_180,
        HUDRotation_270
    };

    /// Structure containing the information for displaying a viewport on a screen, and the associated perspective camera.
    /// Only full screen viewports are supported at this time
    ///
    typedef struct ViewportInfo
    {
        /// ID for the viewport
        ///
        uint64_t id;

        /// Where on the screen is the camera rendered in pixel coordinates.
        ///
        int32_t cameraLeft;
        int32_t cameraBottom;
        int32_t cameraWidth;
        int32_t cameraHeight;

        /// Camera frustum
        ///
        double cameraFrustumLeft;
        double cameraFrustumRight;
        double cameraFrustumTop;
        double cameraFrustumBottom;

        /// Clip planes
        ///
        double nearClipPlane;
        double farClipPlane;

        /// Camera position and rotation (as quaternion)
        ///
        double cameraPosition[3];
        double cameraRotation[4];

        /// the name of the viewport
        ///
        const char* viewportName;

        /// The orientation of the HUD on this viewport
        ///
        enum HUDRotation hudRotation;

        /// Display index on which this viewport will be located
        ///
        uint8_t displayIndex;

        /// The main display index used to offset from UnityEngine's Display array
        ///
        uint8_t mainDisplayIndex;

    } ViewportInfo;

    /// Structure containing the information for a viewpoint
    ///
    typedef struct ViewpointInfo
    {
        /// ID for the Viewpoint
        ///
        uint64_t id;

        /// The content path of the viewpoint
        const char* contentPath;

        /// the position of the viewpoint
        double position[3];

        /// the rotation of the viewpoint
        double rotation[4];

    } ViewpointInfo;

    /// Structure containing the information for a graphics gallery
    ///
    typedef struct GraphicsGalleryInfo
    {
        /// ID for the Graphics Gallery
        ///
        uint64_t id;

        /// the name of the graphics gallery instance
        const char* name;

        /// the full path of the graphics gallery definition
        const char* path;

        /// the translation of the graphics gallery
        double translation[3];

        /// the rotation of the graphics gallery
        double rotationQuaternion[4];

        /// the scale of the graphics gallery
        double scale[3];

    } GraphicsGalleryInfo;

    /// Structure containing a graphic node information.
    ///
    typedef struct GraphicNodeInfo
    {
        /// Id of the graphic node
        ///
        uint64_t id;

        /// the name of the graphic node
        ///
        const char* name;

        /// the content path of the node
        ///
        const char* contentPath;

        /// Node's world position
        ///
        double position[3];

        /// Node's world rotation quaternion
        ///
        double rotation[4];

        /// Node's local position
        ///
        double localPosition[3];

        /// Node's local rotation quaternion
        ///
        double localRotation[4];

        /// Node's local scale
        ///
        double localScale[3];

        /// Node visibility
        ///
        bool visible;

    } GraphicNodeInfo;

    /// Structure containing a controlled light information.
    ///
    typedef struct LightInfo
    {
        /// Id of the light
        ///
        uint64_t id;

        /// the name of the light extension
        ///
        const char* contentPath;

        /// The name of the light extension in the file
        ///
        const char* vortexExtensionName;

        /// The file containing the light extension
        ///
        const char* vortexDocumentPath;

        /// Light's world position
        ///
        double position[3];

        /// Light's world rotation quaternion
        ///
        double rotation[4];

        /// light visibility
        ///
        bool visible;
    } LightInfo;

    enum HookingDisplayState
    {
        HookingDisplayState_HookOutOfRange,
        HookingDisplayState_LoopOutOfRange,
        HookingDisplayState_InRange,
        HookingDisplayState_Attracting,
        HookingDisplayState_Attached,
        HookingDisplayState_Detachable,
        HookingDisplayState_Invisible
    };

    /// Structure containing hooking display information.
    ///
    typedef struct HookingDisplayInfo
    {
        /// contentPath Id
        ///
        uint64_t id;

        /// the name of the hook extension
        ///
        const char* contentPath;

        /// hook's world position
        ///
        double position[3];

        /// display size
        ///
        float radiusHookingSphere;

        /// amount of hooking spheres in hooking display
        ///
        int nbHookingSpheres;

        /// index of hooking sphere in hooking display
        ///
        int index;

        /// hooking state
        ///
        enum HookingDisplayState state;

    } HookingDisplayInfo;


    typedef struct LoadingImageInfo
    {
        /// Id of the image
        ///
        uint64_t id;

        /// the name of node, if exists, combined
        /// with the name of Loading Image extension
        ///
        const char* extendedName;

        /// the path of asset bundle
        ///
        const char* filePath;

        /// the state of the app, if editing or not
        ///
        bool isEditing;

    } LoadingImageInfo;


    /// HUD extension type
    ///
    enum HUDType
    {
        HUDType_Text,
        HUDType_Image
    };

    /// Structure containing various HUD information.
    ///
    typedef struct HUDInfo
    {
        /// Id for the HUD extension
        ///
        uint64_t id;

        /// Associated viewport Id
        ///
        uint64_t viewportId;

        /// The name of the HUD extension instance
        ///
        const char* name;

        /// The name of the HUD extension in the container file
        ///
        const char* nameInFile;

        /// The vx file path of the HUD extension
        ///
        const char* filePath;

        /// HUD display text
        ///
        const char* text;

        /// HUD extension type
        ///
        enum HUDType type;

        /// HUD visibility
        ///
        bool visible;

    } HUDInfo;

    typedef struct VHLInfo
    {
        /// the content path of the VHL extension
        ///
        const char* contentPath;

        /// The name of the VHL extension in the file
        ///
        const char* vortexExtensionName;

        /// The file containing the VHL extension
        ///
        const char* vortexDocumentPath;

        /// The handle/pointer to the VHL extension
        ///
        uint64_t vhlHandle;

    } VHLInfo;

    typedef struct TracksAnimationInfo
    {
        /// Id for the TracksAnimation extension
        ///
        uint64_t id;

        /// the content path of the TracksAnimation extension
        ///
        const char* contentPath;

        const char* vortexExtensionName;

        const char* trackNodeContentPath;

        double* positions;

        double* rotations;

        uint16_t positionsCount;

    } TracksAnimationInfo;

    /// Structure containing a graphics spline information.
    ///
    typedef struct GraphicsSplineInfo
    {
        /// ID for the spline
        ///
        uint64_t id;

        /// The name of the spline instance
        ///
        const char* contentPath;

        /// The name of the spline in the container file
        ///
        const char* vortexExtensionName;

        /// The content path of the spline
        ///
        const char* vortexDocumentPath;

        /// Number of control points, each control point has a position,
        /// a quaternion rotation and a texture coordinate
        ///
        uint64_t controlPointsCount;

        /// Number of sections, each section contains a certain amount of control points
        ///
        uint64_t sectionsCount;

        /// Positions for the control points (3 doubles per control point)
        ///
        double* positions;

        /// Rotations for the control points (4 doubles per control point)
        ///
        double* rotations;

        /// Texture coordinates for the control points (1 double per control point)
        ///
        double* texCoords;

        /// Control points start indexes for each section
        ///
        uint32_t* sectionsControlPointsStartIndexes;

        /// Spline visibility
        ///
        bool visible;

    } GraphicsSplineInfo;

    /// Structure containing a graphics LiDAR information.
    ///
    typedef struct GraphicsLidarInfo
    {
        /// ID for the LiDAR
        ///
        uint64_t id;

        /// Number of lasers for the LiDAR
        ///
        uint64_t numberOfChannels;

        /// Number of points generated horizontally per-channel by the LiDAR
        ///
        uint64_t horizontalResolution;

        /// Laser range for the LiDAR
        ///
        double range;

        /// Number of rotations per second of the LiDAR
        ///
        double horizontalRotationFrequency;

        /// Minimum horizontal FOV for the LiDAR
        ///
        double horizontalFovStart;

        /// Maximum horizontal FOV for the LiDAR
        ///
        double horizontalFovLength;

        /// Minimum vertical FOV for the LiDAR
        ///
        double verticalFovUpper;

        /// Maximum vertical FOV for the LiDAR
        ///
        double verticalFovLower;

        /// Number of updates per second of the LiDAR
        ///
        double updateFrequency;

        /// Number of points from the latest point cloud generated by the LiDAR
        ///
        uint64_t pointCloudSize;

        /// Points data from the latest point cloud generated by the LiDAR
        ///
        const float* pointCloud;

        /// the world-space translation of the lidar
        double translation[3];

        /// the rotation of the lidar
        double rotationQuaternion[4];

        /// Enable/disable output as a distance field
        ///
        bool outputAsDistanceField;

        /// Enable/Disable visualization of the point cloud generated by the LiDAR
        ///
        bool pointCloudVisualization;

    } GraphicsLidarInfo;

    /// Structure containing a graphics height field information.
    ///
    typedef struct GraphicsHeightFieldInfo
    {
        /// ID for the height field
        ///
        uint64_t id;

        /// The name of the height field instance
        ///
        const char* name;

        /// The name of the height field in the container file
        ///
        const char* vortexExtensionName;

        /// The content path of the height field
        ///
        const char* vortexDocumentPath;

        /// Height field's world position
        ///
        double position[3];

        /// Height field's world rotation quaternion
        ///
        double rotation[4];

        /// Height field's dimensions
        ///
        double dimensions[3];

        /// Height field's cell size along x
        ///
        double cellSizeX;

        /// Height field's cell size along y
        ///
        double cellSizeY;

        /// Number of vertices in the x direction
        ///
        uint64_t numVerticeX;

        /// Number of vertices in the y direction
        ///
        uint64_t numVerticeY;

        /// Number of heights in the height field
        ///
        uint64_t heightsArraySize;

        /// Elevations along height field normal
        ///
        double* heights;

        /// Un-disturb/disturb flags for the heights
        ///
        uint16_t* heightsFlags;

        /// Semicolon separated list of the terrain node names the height field overlaps
        ///
        const char* terrainNodeNames;

        /// Terrain hole World position
        ///
        double terrainHolePosition[3];

        /// Terrain hole world rotation quaternion
        ///
        double terrainHoleRotation[4];

        /// Flag that indicates if the height field is attached to a soil bin
        ///
        bool soilBinEnabled;

    } GraphicsHeightFieldInfo;


    /// Structure containing a graphics deformable terrain tile information.
    ///
    typedef struct GraphicsDeformableTerrainTile
    {
        /// Tile coordinate in the X direction
        ///
        uint32_t tileX;

        /// Tile coordinate in the Y direction
        ///
        uint32_t tileY;

        /// Elevations along deformable terrain normal
        ///
        const float* heights;

        /// Un-disturb/disturb flags for the heights
        ///
        const uint16_t* heightsFlags;

    } GraphicsDeformableTerrainTile;

    /// Structure containing a graphics deformable terrain information.
    ///
    typedef struct GraphicsDeformableTerrainInfo
    {
        /// ID for the deformable terrain
        ///
        uint64_t id;

        /// The name of the deformable terrain instance
        ///
        const char* name;

        /// The name of the deformable terrain in the container file
        ///
        const char* vortexExtensionName;

        /// The content path of the deformable terrain
        ///
        const char* vortexDocumentPath;

        /// deformable terrain's world position
        ///
        double position[3];

        /// deformable terrain's world rotation quaternion
        ///
        double rotation[4];

        /// deformable terrain's dimensions
        ///
        double dimensions[3];

        /// Deformable terrain size in terms of tiles in the X direction
        ///
        uint64_t numTileX;

        /// Deformable terrain size in terms of tiles in the Y direction
        ///
        uint64_t numTileY;

        /// Tile's square resolution
        ///
        uint64_t tileResolution;

        /// deformable terrain's cell size
        ///
        double cellSize;

        /// Number of tiles contained in the tiles array
        ///
        uint64_t tilesArraySize;

        /// Array of terrain tiles
        ///
        GraphicsDeformableTerrainTile* tiles;

        /// Semicolon separated list of the terrain node names the deformable terrain overlaps
        ///
        const char* terrainNodeNames;

        /// Terrain hole World position
        ///
        double terrainHolePosition[3];

        /// Terrain hole world rotation quaternion
        ///
        double terrainHoleRotation[4];

    } GraphicsDeformableTerrainInfo;

    /// Structure containing a graphics grade quality sensor information.
    ///
    typedef struct GraphicsGradeQualitySensorInfo
    {
        /// ID for the grade quality sensor
        ///
        uint64_t id;

        /// The name of the grade quality sensor instance
        ///
        const char* name;

        /// The name of the grade quality sensor in the container file
        ///
        const char* vortexExtensionName;

        /// The content path of the grade quality sensor
        ///
        const char* vortexDocumentPath;

        /// Grade quality sensor's point cloud size
        ///
        uint64_t pointCloudSize;

        /// Grade quality sensor's point cloud
        ///
        double* pointCloud;

        /// Grade quality sensor's color interpolation factors
        ///
        double* colorInterpolationFactors;

        /// Grade quality sensor's point cloud visibility
        ///
        bool visible;

    } GraphicsGradeQualitySensorInfo;

    /// Structure containing a graphics depth camera information.
    ///
    typedef struct GraphicsDepthCameraInfo
    {
        /// ID for the depth camera
        ///
        uint64_t id;

        /// Camera image width
        ///
        uint64_t width;

        /// Camera image width
        ///
        uint64_t height;

        /// Camera field-of-view
        ///
        double fov;

        /// Camera framerate
        ///
        uint64_t framerate;

        /// Maximum depth that can be captured by the camera, in meters.
        ///
        double zMax;

        /// Count of depth images returned but the depthImagesPixels parameter.
        ///
        uint64_t depthImageCount;

        /// Captured depth images pixel data
        ///
        const float* depthImagesPixels;

        /// the world-space translation of the depth camera
        double translation[3];

        /// the rotation of the depth camera
        double rotationQuaternion[4];

    } GraphicsDepthCameraInfo;

    /// Structure containing a graphics color camera information.
    ///
    typedef struct GraphicsColorCameraInfo
    {
        /// ID for the color camera
        ///
        uint64_t id;

        /// Camera image width
        ///
        uint64_t width;

        /// Camera image width
        ///
        uint64_t height;

        /// Camera field-of-view
        ///
        double fov;

        /// Camera framerate
        ///
        uint64_t framerate;

        /// Count of color images returned but the colorImagesPixels parameter.
        ///
        uint64_t colorImagesCount;

        /// Captured color images pixel data
        ///
        const uint8_t* colorImagesPixels;

        /// the world-space translation of the color camera
        double translation[3];

        /// the rotation of the color camera
        double rotationQuaternion[4];

    } GraphicsColorCameraInfo;

    /// Structure containing graphics soil particles information.
    ///
    typedef struct GraphicsSoilParticlesInfo
    {
        /// ID for the soil particles generator
        ///
        uint64_t id;

        /// The name of the soil particles generator
        ///
        const char* name;

        /// The name of the soil particles generator in the container file
        ///
        const char* vortexExtensionName;

        /// The content path of the soil particles generator
        ///
        const char* vortexDocumentPath;

        /// Number of particles generated by the soil particles generator
        ///
        uint64_t particlesCount;

        /// Particles positions (3 floats per position)
        ///
        float* positions;

        /// Particles ids
        ///
        int* ids;

        /// Particles radii
        ///
        float* radii;

    } GraphicsSoilParticlesInfo;


    /// Structure containing graphics soil dust information.
    ///
    typedef struct GraphicsSoilDustInfo
    {
        /// ID for the soil dust generator
        ///
        uint64_t id;

        /// The name of the soil dust generator
        ///
        const char* name;

        /// The name of the soil dust generator in the container file
        ///
        const char* vortexExtensionName;

        /// The content path of the soil dust generator
        ///
        const char* vortexDocumentPath;

        /// Number of soil dust emitters
        ///
        uint64_t emittersCount;

        /// Soil dust emitters positions (3 doubles per position)
        ///
        double* positions;

        /// Soil dust emitters velocities (3 doubles per velocity)
        ///
        double* velocities;

        /// Soil dust emitters densities
        ///
        double* densities;

    } GraphicsSoilDustInfo;

    /// Graphic notification type
    ///
    enum GraphicNotificationType
    {
        /// A graphic object is added
        ///
        GraphicNotificationType_Add,

        /// A graphic object is removed
        ///
        GraphicNotificationType_Remove,

        /// A graphic object is updated
        ///
        GraphicNotificationType_Update
    };

    /// Terrain request structure, contains an array and numberOfBoundingBoxes, so we have a bounding box specified by minMaxPoints, which
    /// are flattened arrays of points, and numberOfBoundingBoxes which describes how many bounding boxes are in the request.
    ///
    typedef struct StreamedTerrainRequest
    {
        uint32_t numberOfBoundingBoxes;

        /// 6*numberOfBoundingBoxes size array, flattened array of points [minX1, minY1, minZ1, maxX1, maxY1, maxZ1, minX2, minY12, minZ2...]
        double* minMaxPoints;

    } StreamedTerrainRequest;

    /// Terrain heights structure contains one dimentional array that store a grid of actual heights.
    /// 
    /// An example shows how to access the heights and materials
    /// 
    ///    for (int cy = 0, vy = numHeightsY - 1; cy < numHeightsY; ++cy, --vy)
    ///    {
    ///        int offset = vy * numHeightsX;
    ///        for (int cx = 0; cx < numHeightsX; ++cx)
    ///        {
    ///           double height = terrainHeightField.heights[cx + offset]);
    ///        }
    ///    }
    ///    uint32_t nbCells = (numHeightsY - 1) * (numHeightsX - 1);
    ///    for (uint32_t cell = 0; cell < nbCells; ++cell)
    ///    {
    ///        int index = terrainHeightField.materialIndexes[cell];
    ///    }
    /// 
    typedef struct StreamedTerrainHeightField
    {
        /// Array of vortex material indexes, one material index per heightfield cell
        ///
        int* materialIndexes;

        /// Array of actual heightfield heights
        ///
        double* heights;

        /// Number of heights along x axis
        ///
        uint32_t numHeightsX;
        
        /// Number of heights along y axis
        ///
        uint32_t numHeightsY;

        /// Cell size along x axis
        ///
        double cellSizeX;

        /// Cell size along y axis
        ///
        double cellSizeY;

        /// World-space translation of the heights
        ///
        double translation[3];

        /// World-space rotation of the heights
        ///
        double rotationQuaternion[4];

    } StreamedTerrainHeightField;

    typedef struct StreamedBox
    {
        /// Unique Id of the object for reference
        ///
        int uniqueID;

        /// Material index applied to entire Box
        ///
        int materialIndex;

        /// World-space translation of the box
        ///
        double translation[3];

        /// World-space rotation of the box
        ///
        double rotationQuaternion[4];

        /// Box sizes
        ///
        double size[3];

    }StreamedBox;

    typedef struct StreamedSphere
    {
        /// Unique Id of the object for reference
        ///
        int uniqueID;

        /// Material index applied to the sphere
        ///
        int materialIndex;

        /// World-space translation of the sphere
        ///
        double translation[3];

        /// Sphere radius
        ///
        double radius;

    }StreamedSphere;

    /// Terrain response structure contains an array of StreamedTerrainHeightField and the array size is heightFieldCount
    ///
    typedef struct StreamedTerrainResponse
    {
        /// Array of StreamedTerrainHeightField
        ///
        StreamedTerrainHeightField* streamedTerrainHeightFields;

        /// Heighfield array size
        ///
        uint32_t heightFieldCount;

        /// Array of Streamed Boxes
        ///
        StreamedBox* boxes;

        /// Box array size
        ///
        uint32_t boxCount;

        /// Array of Streamed Spheres
        ///
        StreamedSphere* spheres;

        /// Sphere array size
        ///
        uint32_t sphereCount;

    } StreamedTerrainResponse;

    typedef struct MaterialTableInfo
    {
        /// ID for the material table.
        ///
        uint64_t id;

        /// The content path of the material table.
        ///
        const char* contentPath;

        ///  The list of material names, each name is separated by a semicolon.
        ///
        const char* materialNames;
    } MaterialTableInfo;

    /// Callback for receiving the additional plugins directory
    ///
    typedef void(*PluginsDirectoryDelegate)(const char* pluginsDirectory);

    /// Callback for receiving the Display, Camera and ViewportInfo data
    ///
    typedef void(*ViewportCallback)(GraphicNotificationType type, ViewportInfo* viewport);

    /// Callback for graphics gallery
    ///
    typedef void(*GraphicsGalleryCallback)(GraphicNotificationType type, GraphicsGalleryInfo* graphicsGallery);

    /// Callback for receiving the GraphicNodeInfo data.
    ///
    typedef void(*NodeCallback)(GraphicNotificationType type, GraphicNodeInfo* nodeInfo);

    /// Callback for receiving the LightInfo data.
    ///
    typedef void(*LightCallback)(GraphicNotificationType type, LightInfo* lightInfo);

    /// Callback for receiving the HookingDisplayInfo data.
    ///
    typedef void(*HookingDisplayCallback)(GraphicNotificationType type, HookingDisplayInfo* hookingDisplayInfo);

    /// Callback for receiving the LoadingImageInfo data.
    ///
    typedef void(*LoadingImageCallback)(GraphicNotificationType type, LoadingImageInfo* loadingImageInfo);

    /// Callback for receiving the HUDInfo data.
    ///
    typedef void(*HUDCallback)(GraphicNotificationType notificationType, HUDInfo* hudInfo);

    /// Callback for receiving the VHLInfo data.
    ///
    typedef void(*VHLCallback)(GraphicNotificationType notificationType, VHLInfo* vhlInfo);

    /// Callback for receiving the TracksAnimationInfo data.
    ///
    typedef void(*TracksAnimationCallback)(GraphicNotificationType notificationType, TracksAnimationInfo* tracksInfo);

    /// Callback for receiving the GraphicsSplineInfo data
    ///
    typedef void(*SplineCallback)(GraphicNotificationType type, GraphicsSplineInfo* graphicsSpline);

    /// Callback for receiving the GraphicsLidarInfo data
    ///
    typedef void(*LidarCallback)(GraphicNotificationType type, GraphicsLidarInfo* graphicsLidar);

    /// Callback for receiving the GraphicsDepthCameraInfo data
    ///
    typedef void(*DepthCameraCallback)(GraphicNotificationType type, GraphicsDepthCameraInfo* graphicsDepthCamera);

    /// Callback for receiving the GraphicsColorCameraInfo data
    ///
    typedef void(*ColorCameraCallback)(GraphicNotificationType type, GraphicsColorCameraInfo* graphicsColorCamera);

    /// Callback for receiving the GraphicsHeightFieldInfo data
    ///
    typedef void(*HeightFieldCallback)(GraphicNotificationType type, GraphicsHeightFieldInfo* graphicsHeightField);

    /// Callback for receiving the GraphicsDeformableTerrainInfo data
    ///
    typedef void(*DeformableTerrainCallback)(GraphicNotificationType type, GraphicsDeformableTerrainInfo* graphicsDeformableTerrain);

    /// Callback for receiving the GraphicsGradeQualitySensorInfo data
    ///
    typedef void(*GradeQualitySensorCallback)(GraphicNotificationType type, GraphicsGradeQualitySensorInfo* graphicsGradeQualitySensor);

    /// Callback for receiving the GraphicsSoilParticlesInfo data
    ///
    typedef void(*SoilParticlesCallback)(GraphicNotificationType type, GraphicsSoilParticlesInfo* graphicsSoilParticles);

    /// Callback for receiving the GraphicsSoilDustInfo data
    ///
    typedef void(*SoilDustCallback)(GraphicNotificationType type, GraphicsSoilDustInfo* graphicsSoilDust);

    /// Structure containing method pointers for all callbacks related to GraphicsIntegration.
    ///
    typedef struct GraphicsIntegrationCallbacks
    {
        /// Constructor to initialize all callbacks to nullptr
        ///
        GraphicsIntegrationCallbacks()
            : pluginsDirectoryNotification(nullptr)
            , viewportNotification(nullptr)
            , graphicsGalleryNotification(nullptr)
            , nodeNotification(nullptr)
            , lightNotification(nullptr)
            , hookingDisplayNotification(nullptr)
            , loadingImageNotification(nullptr)
            , HUDNotification(nullptr)
            , VHLNotification(nullptr)
            , tracksAnimationNotification(nullptr)
            , splineNotification(nullptr)
            , heightFieldNotification(nullptr)
            , deformableTerrainNotification(nullptr)
            , gradeQualitySensorNotification(nullptr)
            , soilParticlesNotification(nullptr)
            , soilDustNotification(nullptr)
            , lidarNotification(nullptr)
            , depthCameraNotification(nullptr)
            , colorCameraNotification(nullptr)
        {
        }

        /// Callback for the plugins directory notification.
        ///
        PluginsDirectoryDelegate pluginsDirectoryNotification;

        /// Callback for Viewport notifications.
        ///
        ViewportCallback viewportNotification;

        /// Callback for GraphicsGallery notifications
        ///
        GraphicsGalleryCallback graphicsGalleryNotification;

        /// Callback for receiving the GraphicNodeInfo data.
        ///
        NodeCallback nodeNotification;

        /// Callback for receiving the LightInfo data.
        ///
        LightCallback lightNotification;

        /// Callback for receiving the HookingDisplayInfo data.
        ///
        HookingDisplayCallback hookingDisplayNotification;

        /// Callback for receiving the HookingDisplayInfo data.
        ///
        LoadingImageCallback loadingImageNotification;

        /// Callback for receiving the HUDInfo data.
        ///
        HUDCallback HUDNotification;

        /// Callback for receiving the VHLInfo data.
        ///
        VHLCallback VHLNotification;

        /// Callback for receiving the TracksAnimation data.
        ///
        TracksAnimationCallback tracksAnimationNotification;

        /// Callback for receiving the GraphicsSplineInfo data.
        ///
        SplineCallback splineNotification;

        /// Callback for receiving the GraphicsHeightFieldInfo data.
        ///
        HeightFieldCallback heightFieldNotification;

        /// Callback for receiving the GraphicsDeformableTerrainInfo data.
        ///
        DeformableTerrainCallback deformableTerrainNotification;

        /// Callback for receiving the GraphicsGradeQualitySensorInfo data.
        ///
        GradeQualitySensorCallback gradeQualitySensorNotification;

        /// Callback for receiving the GraphicsSoilParticlesInfo data.
        ///
        SoilParticlesCallback soilParticlesNotification;

        /// Callback for receiving the GraphicsSoilDustInfo data.
        ///
        SoilDustCallback soilDustNotification;

        /// Callback for receiving the GraphicsLidarInfo data.
        ///
        LidarCallback lidarNotification;

        /// Callback for receiving the GraphicsDepthCameraInfo data.
        ///
        DepthCameraCallback depthCameraNotification;

        /// Callback for receiving the GraphicsColorCameraInfo data.
        ///
        ColorCameraCallback colorCameraNotification;
    } GraphicsIntegrationCallbacks;

#ifdef __cplusplus
}

#endif
