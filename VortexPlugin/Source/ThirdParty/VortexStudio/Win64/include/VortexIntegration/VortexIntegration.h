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

#include <VortexIntegration/VortexIntegrationLibrary.h>
#include <VortexIntegration/Structs.h>

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

/// @section Application
/// 
/// Creates a VxApplication by applying the provided setup document and material table.
///
/// @param setupDocument               An absolute path to the setup document (.vxc) to apply on the VxApplication.
/// @param materialTable               An absolute path to the material table document (.vxmaterials) to add to the VxApplication.
/// @param dataStore                   An absolute path to the data provider of the VxApplication.
/// @param logFilepath                 An optional absolute path to use as log filepath if the path is not defined in the .vxc.
/// @param terrainProviderInfo         An optional terrain provider hook description.
///
/// @return True if the VxApplication was successfully created.
///
VORTEXINTEGRATION_SYMBOL bool VortexCreateApplication(const char* setupDocument, const char* materialTable, const char* dataStore, const char* optionalLogFilepath, const VortexTerrainProviderInfo* terrainProviderInfo);

/// Creates a VxApplication with the VortexApplicationParameters extracted from the command line arguments.
/// This is the VortexCreateApplication function to use when running in a multi-node simulator launched from the Vortex Director.
/// The command line arguments will contain all the parameters needed.
///
/// @param argc        The command line arguments count.
/// @param argv        The command line arguments.
///
/// @return True if the VxApplication was successfully created.
///
VORTEXINTEGRATION_SYMBOL bool VortexCreateApplicationWithCommandLineArgs(int argc, const char* argv[]);

/// Updates the VxApplication.
///
VORTEXINTEGRATION_SYMBOL bool VortexUpdateApplication();

/// Destroys the VxApplication.
///
VORTEXINTEGRATION_SYMBOL void VortexDestroyApplication();

/// Returns true if the VxApplication is created.
///
VORTEXINTEGRATION_SYMBOL bool VortexApplicationCreated();

/// @section Utilities
/// 
/// Returns the application mode.
///
VORTEXINTEGRATION_SYMBOL VortexApplicationMode VortexGetApplicationMode();

/// Changes the application mode asynchronously. The application mode will change when possible by default.
/// It is possible to wait for the mode to actually change by using the waitForModeToBeApplied parameter.
///
/// @param waitForModeToBeApplied If true, the VxApplication be be stepped until the requested mode is applied.
///
/// @return True if the requested application mode is valid in the current context.
///
VORTEXINTEGRATION_SYMBOL bool VortexSetApplicationMode(VortexApplicationMode mode, bool waitForModeToBeApplied);

/// Determines if the current application is the master
/// 
VORTEXINTEGRATION_SYMBOL bool VortexIsMaster();

/// Determines whether the paused flag is set on the simulation.
/// 
VORTEXINTEGRATION_SYMBOL bool VortexIsPaused();

/// Pauses the simulation.
/// It will take effect at the next application update.
/// 
VORTEXINTEGRATION_SYMBOL void VortexPause(bool pause);

/// Makes the simulation do one step and then pause.
/// 
VORTEXINTEGRATION_SYMBOL void VortexStepOnce();

/// Resets the simulation time.
///
VORTEXINTEGRATION_SYMBOL void VortexResetSimulationTime();

/// Returns the simulation frame rate set in the setup document applied to the VxApplication.
///
VORTEXINTEGRATION_SYMBOL double VortexGetSimulationFrameRate();

/// Returns the simulation time step in seconds ( the inverse of the frame rate )
/// 
VORTEXINTEGRATION_SYMBOL double VortexGetSimulationTimeStep();

/// Get the current simulation time in seconds
/// 
VORTEXINTEGRATION_SYMBOL double VortexGetSimulationTime();

/// Gets the current frame index
/// 
VORTEXINTEGRATION_SYMBOL unsigned int VortexGetFrame();

/// Returns whether or not the application has a valid license.
///
VORTEXINTEGRATION_SYMBOL bool VortexHasValidLicense();

/// Returns the synchronisation mode set in the setup document applied to the VxApplication.
///
VORTEXINTEGRATION_SYMBOL VortexSynchronizationMode VortexGetSynchronizationMode();

/// Set the main(index 0) VortexTerrainProviderInfo for the current Vortex application.
///
VORTEXINTEGRATION_SYMBOL bool VortexSetTerrainProviderInfo(const VortexTerrainProviderInfo* terrainProviderInfo);

/// Set the VortexTerrainProviderInfo for the current Vortex application.
///
VORTEXINTEGRATION_SYMBOL bool VortexSetTerrainProviderInfoAtIndex(uint32_t index, const VortexTerrainProviderInfo* terrainProviderInfo);

/// Set the VortexTerrainProviderInfo list size for the current Vortex application.
/// When not set, default size is 1.
///
VORTEXINTEGRATION_SYMBOL bool VortexSetTerrainProviderInfoListSize(uint32_t size);

/// Get the VortexTerrainProviderInfo for the current Vortex application as output argument.
///
/// Returns false if none available.
///
VORTEXINTEGRATION_SYMBOL bool VortexGetTerrainProviderInfo(VortexTerrainProviderInfo* outInfo);

/// Get the VortexTerrainProviderInfo at index for the current Vortex application as output argument.
///
/// Returns false if not available.
///
VORTEXINTEGRATION_SYMBOL bool VortexGetTerrainProviderInfoAtIndex(uint32_t index, VortexTerrainProviderInfo* outInfo);

/// Caller must preallocate the "materials" array and set size accordingly
/// size will be updated to the actual number of available materials
/// however no more than the given size will be copied to "materials"
///
VORTEXINTEGRATION_SYMBOL void VortexGetAvailableMaterials(VortexMaterial* materials, uint32_t* size);

/// To use when the Vortex application was created with some VortexTerrainProviderInfo (see VortexCreateApplication()).
/// The function allows to know if the passed ID is associated to a part of the terrain that still exists.
/// The terrain provider flushes some parts of the terrain when not needed. Consequently, some parts of the terrain
/// that were already sent to Vortex might not exist anymore.
///
/// @param sourceId Refers to VortexTerrainProviderObject::uniqueID.
///
/// @return True if the terrain already contains the passed VortexTerrainProviderObject::uniqueID.
///
VORTEXINTEGRATION_SYMBOL bool VortexContainsTerrain(int32_t sourceId);

/// To use when using multiple VortexTerrainProviderInfo.
/// The function allows to know if the passed ID is associated to a part of the terrain that still exists.
/// The terrain provider flushes some parts of the terrain when not needed. Consequently, some parts of the terrain
/// that were already sent to Vortex might not exist anymore.
///
/// @param providerIndex Refers to VortexTerrainProviderInfo index.
/// @param sourceId Refers to VortexTerrainProviderObject::uniqueID.
///
/// @return True if the terrain already contains the passed VortexTerrainProviderObject::uniqueID.
///
VORTEXINTEGRATION_SYMBOL bool VortexTerrainProviderContainsId(uint32_t providerIndex, int32_t sourceId);

/// @section Content

/// Loads a mechanism and add it to the application at the given position
///
/// @param mechanismFile      An absolute path to the ".vxmechanism" file to load.
/// @param translation        The initial translation to apply to the loaded mechanism (x, y, z).
/// @param rotationQuaternion The initial rotation (as a quaternion) to apply to the loaded mechanism (w, x, y, z).
///
/// @return A VXOBJECT_HANDLE on the loaded mechanism.
///
VORTEXINTEGRATION_SYMBOL VortexObjectHandle VortexLoadMechanism(const char* mechanismFile, const double translation[3], const double rotationQuaternion[4]);

/// Loads a scene and add it to the application
///
/// @param sceneFile      An absolute path to the ".vxscene" file to load.
///
/// @return A VXOBJECT_HANDLE on the loaded scene.
///
VORTEXINTEGRATION_SYMBOL VortexObjectHandle VortexLoadScene(const char* sceneFile);

/// Finds an object handle from a loaded object
///
/// @param objectHandle A VXOBJECT_HANDLE on the object to search from e.g. scene handle
/// @param childName name of the object to search for e.g. mechanism name within a scene
///
/// @return A VXOBJECT_HANDLE on the found mechanism.
///
VORTEXINTEGRATION_SYMBOL VortexObjectHandle VortexGetChildByName(VortexObjectHandle handle, const char* childName);

/// Position a mechanism in the application
///
/// @param mechanismHandle    A VXOBJECT_HANDLE on the mechanism to position.
/// @param translation        The initial translation to apply to the loaded mechanism (x, y, z).
/// @param rotationQuaternion The initial rotation (as a quaternion) to apply to the loaded mechanism (w, x, y, z).
///
/// @return True if the operation was successful
///
VORTEXINTEGRATION_SYMBOL bool VortexSetWorldTransform(VortexObjectHandle mechanismHandle, const double translation[3], const double rotationQuaternion[4]);

/// Unloads the provided mechanism from the application
///
/// @param mechanismHandle A VXOBJECT_HANDLE on the mechanism to unload.
/// 
/// @return True if the associated mechanism could be successfully unloaded from the application.
///
VORTEXINTEGRATION_SYMBOL bool VortexUnloadMechanism(VortexObjectHandle mechanismHandle);

/// Unloads the provided scene from the application
///
/// @param sceneHandle A VXOBJECT_HANDLE on the scene to unload.
/// 
/// @return True if the associated scene could be successfully unloaded from the application.
///
VORTEXINTEGRATION_SYMBOL bool VortexUnloadScene(VortexObjectHandle sceneHandle);

/// Create the default offscreen context
/// allows extensions such as the depth sensor to produce meaningful data
/// from graphic galleries without rendering to screen
/// a graphics module must be present
/// 
/// @return True if the offscreen context was created.
/// 
VORTEXINTEGRATION_SYMBOL bool VortexCreateDefaultOffscreenGraphics();

/// Destroy the default offscreen context
///
/// @return True if the offscreen context was destroyed.
///
VORTEXINTEGRATION_SYMBOL bool VortexDestroyDefaultOffscreenGraphics();

/// @section Content accessor
///
/// Set the value of a input
/// 
///
VORTEXINTEGRATION_SYMBOL bool VortexSetInputBoolean(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, bool value);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputInt(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, int32_t value);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputReal(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, double value);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputVector2(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, const double value[2]);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputVector3(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, const double value[3]);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputVector4(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, const double value[4]);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputMatrix(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, const double translation[3], const double rotationQuaternion[4]);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputString(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, const char* value);
VORTEXINTEGRATION_SYMBOL bool VortexSetInputExtensionPointer(VortexObjectHandle objectHandle, const char* interfaceName, const char* inputName, VortexObjectHandle value);

/// Return the value of a output
/// 
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputBoolean(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, bool* value);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputInt(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, int32_t* value);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputReal(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double* value);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputVector2(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double value[2]);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputVector3(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double value[3]);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputVector4(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double value[4]);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputMatrix(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double translation[3], double rotationQuaternion[4]);

/// Caller must preallocate value and set size accordingly
/// size will be updated to the actual size of the string including \0
/// however no more than the given size will be copied to value
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputString(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, char* value, uint32_t* size);

/// For a output field of type VxSim::VxExtension*
/// Return the output world transform of the IMobile interface of the VxSim::VxExtension*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputIMobile(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double translation[3], double rotationQuaternion[4]);

/// For a output field of type VxSim::VxExtension*
/// Return the output VxGraphicsPlugins::HeightFieldDataContainer of the VxSim::VxExtension*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputHeightField(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, VortexHeightField* heightField);
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputTiledHeightField(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, VortexTiledHeightField* tiledHeightField, bool onlyModified);

/// For a output field of type VxSim::VxExtension*
/// Return the output VxGraphicsPlugins::ParticlesDataContainer of the VxSim::VxExtension*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputParticles(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, VortexParticles* particles);

/// For a output field of type VxSim::VxExtension*
/// Return the underlying data of a GraphicsMesh from the VxSim::VxExtension*
/// Caller must preallocate meshDataArray and set meshDataCount accordingly
/// meshDataCount will be updated to the actual number of meshes present
/// however no more than the given meshDataCount will be copied to meshDataArray
/// pass a null Array to get only the Count
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputGraphicsMesh(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, VortexMeshData* meshDataArray, uint32_t* meshDataCount);

/// For a output field of type VxSim::VxExtension*
/// Return the underlying data of a GraphicsMaterial from the VxSim::VxExtension*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputGraphicsMaterial(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, VortexMaterialData* materialData);

/// Caller must preallocate value and set size accordingly
/// size will be updated to the actual size of the value
/// however no more than the given size will be copied to value
///
VORTEXINTEGRATION_SYMBOL bool VortexGetOutputDepthSensor(VortexObjectHandle objectHandle, const char* interfaceName, const char* outputName, double* values, uint32_t* size);

/// Call this function to verify if a Vortex field exists and is of the expected data type.
///
VORTEXINTEGRATION_SYMBOL VortexFieldStatus VortexGetFieldStatus(VortexObjectHandle objectHandle, const char* interfaceName, const char* fieldName, VortexFieldType fieldType, VortexDataType dataType);

/// @section Graphics Content accessor
///
/// Return an array of all VxGraphics::GraphicsNode*
/// Caller must preallocate array and set size accordingly
/// nodeHandleCount will be updated to the actual size of the returned array
/// however no more than the given size will be copied to array
/// pass a null Array to get only the Count
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsNodeHandles(VortexObjectHandle objectHandle, VortexObjectHandle* nodeHandleArray, uint32_t* nodeHandleCount);

/// Return the graphic node data for a handle of type VxGraphics::GraphicsNode*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicNodeData(VortexObjectHandle nodeHandle, VortexGraphicNodeData* graphicNodeData);

/// Get the parent transform of the given graphic node
///
/// @param nodeHandle         A pointer to a graphic node.
/// @param translation        The translation of the parent node.
/// @param scale              The rotation of the parent node.
/// @param rotation           The rotation of the parent node.
///
VORTEXINTEGRATION_SYMBOL void VortexGetParentTransform(VortexObjectHandle nodeHandle, double translation[3], double scale[3], double rotationQuaternion[4]);

/// Return an array of all VxGraphics::Geometry*
/// Caller must preallocate array and set size accordingly
/// meshHandleCount will be updated to the actual size of the returned array
/// however no more than the given size will be copied to array
/// pass a null Array to get only the Count
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsMeshHandles(VortexObjectHandle objectHandle, VortexObjectHandle* meshHandleArray, uint32_t* meshHandleCount);

/// Return the underlying mesh data for a handle of type VxGraphics::GraphicsNode*
/// Caller must preallocate meshDataArray and set meshDataCount accordingly
/// meshDataCount will be updated to the actual number of meshes present
/// however no more than the given meshDataCount will be copied to meshDataArray
/// pass a null Array to get only the Count
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsMeshData(VortexObjectHandle meshHandle, VortexMeshData* meshDataArray, uint32_t* meshDataCount);

/// Return an array of all VxGraphics::Texture*
/// Caller must preallocate array and set size accordingly
/// textureHandleCount will be updated to the actual size of the returned array
/// however no more than the given size will be copied to array
/// pass a null Array to get only the Count
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsTextureHandles(VortexObjectHandle objectHandle, VortexObjectHandle* textureHandleArray, uint32_t* textureHandleCount);

/// Return the underlying texture data pointer for a handle of type VxGraphics::Texture*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsTextureData(VortexObjectHandle textureHandle, VortexTextureData* textureData);

/// Copy into textureData the uncompressed texture for a handle of type VxGraphics::Texture*
/// Caller must preallocate VortexTextureData.bytes and set VortexTextureData.byteCount accordingly
/// VortexTextureData.byteCount will be updated to the actual number of bytes in texture
/// however no more than the given VortexTextureData.byteCount will be copied to VortexTextureData.bytes
/// pass a null VortexTextureData.bytes to get only the Count
/// format is always Uncompressed BGRA
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsTextureCopy(VortexObjectHandle textureHandle, VortexTextureData* textureData);

/// Return an array of all VxGraphics::GraphicsMaterial*
/// Caller must preallocate array and set size accordingly
/// materialHandleCount will be updated to the actual size of the returned array
/// however no more than the given size will be copied to array
/// pass a null Array to get only the Count
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsMaterialHandles(VortexObjectHandle objectHandle, VortexObjectHandle* materialHandleArray, uint32_t* materialHandleCount);

/// Return the underlying material data for a handle of type VxGraphics::GraphicsNode*
///
VORTEXINTEGRATION_SYMBOL bool VortexGetGraphicsMaterialData(VortexObjectHandle materialHandle, VortexMaterialData* materialData);

/// Initialize Terrain Provider Info object
///
VORTEXINTEGRATION_SYMBOL void VortexTerrainProviderInfoInit(VortexTerrainProviderInfo* info);

///  Sets the remote access parameters.
///
/// @param enabled        Toggles the Remote debugger server.
/// @param listeningPort  Remote debugger listening port. The id must be in the range (0, 65535).
///                       0 means Windows will automatically assign a port.
/// 
VORTEXINTEGRATION_SYMBOL void VortexSetRemoteAccessParameters(bool enabled, int listeningPort);

#ifdef __cplusplus
}
#endif
