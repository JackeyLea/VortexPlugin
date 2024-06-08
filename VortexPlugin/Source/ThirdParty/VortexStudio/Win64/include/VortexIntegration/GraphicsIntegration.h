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

#include <VortexIntegration/VortexIntegrationLibrary.h>
#include <VortexIntegration/VortexIntegrationTypes.h>

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

    /// Register the different callbacks for the graphics integration.
    /// Those callbacks should be implemented in the target rendering engine
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsRegisterCallbacks(GraphicsIntegrationCallbacks* graphicsIntegrationCallbacks);

    /// Send additional plugins directory.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendAdditionalPluginsDirectory(const char* directory);

    /// Send the ViewportInfo when a Viewport is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendViewportNotification(GraphicNotificationType type, ViewportInfo* viewport);

    /// Send the GraphicsGalleryInfo when a Gallery is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendGalleryNotification(GraphicNotificationType type, GraphicsGalleryInfo* graphicsGallery);

    /// Send the GraphicNodeInfo when a GraphicNode is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendNodeNotification(GraphicNotificationType type, GraphicNodeInfo* nodeInfo);

    /// Send the LightInfo when a controlled Light is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendLightNotification(GraphicNotificationType type, LightInfo* lightInfo);

    /// Send the HookingDisplayInfo when a hooking display is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendHookingDisplayNotification(GraphicNotificationType type, HookingDisplayInfo* hookInfo);

    /// Send the LoadingImageInfo when a loading image is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendLoadingImageNotification(GraphicNotificationType type, LoadingImageInfo* imageInfo);

    /// Send the HUDInfo when a HUD extension is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendHUDNotification(GraphicNotificationType notificationType, HUDInfo* hudInfo);

    /// Send the VHLInfo when a VHL extension is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendVHLNotification(GraphicNotificationType notificationType, VHLInfo* vhlInfo);

    /// Send the TracksAnimationInfo when a Tracks Animation extension is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendTracksAnimationNotification(GraphicNotificationType notificationType, TracksAnimationInfo* tracksInfo);

    /// Send the GraphicsSplineInfo when a spline is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendSplineNotification(GraphicNotificationType type, GraphicsSplineInfo* splineInfo);

    /// Send the GraphicsLidarInfo when a LiDAR is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendLidarNotification(GraphicNotificationType type, GraphicsLidarInfo* lidarInfo);

    /// Send the GraphicsDepthCameraInfo when a Depth Camera is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendDepthCameraNotification(GraphicNotificationType type, GraphicsDepthCameraInfo* depthCameraInfo);
   
    /// Send the GraphicsColorCameraInfo when a Color Camera is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendColorCameraNotification(GraphicNotificationType type, GraphicsColorCameraInfo* colorCameraInfo);

    /// Send the GraphicsHeightFieldInfo when a height field is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendHeightFieldNotification(GraphicNotificationType type, GraphicsHeightFieldInfo* heightFieldInfo);

    /// Send the GraphicsDeformableTerrainInfo when a deformable terrain is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendDeformableTerrainNotification(GraphicNotificationType type, GraphicsDeformableTerrainInfo* deformableTerrainInfo);

    /// Send the GraphicsGradeQualitySensorInfo when a grade quality sensor is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendGradeQualitySensorNotification(GraphicNotificationType type, GraphicsGradeQualitySensorInfo* gradeQualitySensorInfo);

    /// Send the GraphicsSoilParticlesInfo when a soil particles generator is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendSoilParticlesNotification(GraphicNotificationType type, GraphicsSoilParticlesInfo* soilParticlesInfo);

    /// Send the GraphicsSoilDustInfo when a soil dust generators is added, removed or updated.
    ///
    VORTEXINTEGRATION_SYMBOL void GraphicsSendSoilDustNotification(GraphicNotificationType type, GraphicsSoilDustInfo* soilDustInfo);

#ifdef __cplusplus
}
#endif
