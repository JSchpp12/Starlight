
cmake_minimum_required (VERSION 3.26)

SET(STARLIGHT_NAME "starlight")
project(${STARLIGHT_NAME}
    LANGUAGES C CXX
)

include(CheckIncludeFile)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
elseif(WIN32)
    set(CMAKE_FIND_LIBRARY_PREFIXES "")
else() 
    set(ENABLE_RTTI ON CACHE BOOL "" FORCE)
endif()

find_package(starlight_common REQUIRED)

set(VULKAN_VERSION "1.4.313.0")
find_package(Vulkan ${VULKAN_VERSION} REQUIRED)

set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_headers")

set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_tools")
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
add_subdirectory("extern/glslang")

find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS container thread filesystem log log_setup)
find_package(Ktx CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(absl CONFIG REQUIRED)

# add all necessary dependencies
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_reflect")

set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
add_subdirectory("extern/shaderc")

set(${STARLIGHT_NAME}_SOURCE
    "src/starlight/StarEngine.cpp" 
    "src/starlight/data_structure/dynamic/ThreadSharedObjectPool.cpp"
    "src/starlight/graphics/PipelineFactory.cpp"
    "src/starlight/core/logging/LoggingFactory.cpp"
    "src/starlight/core/renderer/LightRenderer.cpp"
    "src/starlight/core/renderer/RendererBase.cpp"
    "src/starlight/core/renderer/DefaultRenderer.cpp"
    "src/starlight/core/renderer/RenderingTargetInfo.cpp"
    "src/starlight/core/renderer/RenderingContext.cpp"
    "src/starlight/core/renderer/CaptureCapableRenderer.cpp"
    "src/starlight/virtual/StarObject.cpp"
    "src/starlight/virtual/ModulePlug/ManagerPlug.cpp"
    "src/starlight/virtual/StarMesh.cpp"
    "src/starlight/virtual/StarManager.cpp"
    "src/starlight/virtual/StarScene.cpp"
    "src/starlight/virtual/StarShader.cpp" 
    "src/starlight/virtual/StarMaterial.cpp" 
    "src/starlight/virtual/StarCamera.cpp"
    "src/starlight/virtual/StarPipeline.cpp"
    "src/starlight/virtual/StarEntity.cpp"
    "src/starlight/virtual/TransferRequest_Texture.cpp"
    "src/starlight/virtual/TransferRequest_Memory.cpp"
    "src/starlight/virtual/TransferRequest_Buffer.cpp"
    "src/starlight/virtual/ManagerController_Controller.cpp"
    "src/starlight/virtual/ManagerController_RenderResource_Buffer.cpp"
    "src/starlight/virtual/ManagerController_RenderResource_Texture.cpp"
    "src/starlight/wrappers/graphics/StarDescriptorBuilders.cpp"  
    "src/starlight/wrappers/graphics/StarGraphicsPipeline.cpp"  
    "src/starlight/wrappers/graphics/StarComputePipeline.cpp"
    "src/starlight/wrappers/graphics/StarRenderPass.cpp"
    "src/starlight/wrappers/graphics/StarCommandBuffer.cpp"
    "src/starlight/wrappers/graphics/StarShaderInfo.cpp"
    "src/starlight/wrappers/graphics/StarQueueFamily.cpp"
    "src/starlight/wrappers/graphics/StarQueue.cpp"
    "src/starlight/wrappers/graphics/StarCommandPool.cpp"
    "src/starlight/wrappers/Allocator.cpp"
    "src/starlight/wrappers/graphics/StarTextures/FormatInfo.cpp"
    "src/starlight/wrappers/graphics/StarTextures/Texture.cpp"
    "src/starlight/wrappers/graphics/StarTextures/Resources.cpp"
    "src/starlight/wrappers/graphics/StarTextures/AllocatedResources.cpp"
    "src/starlight/wrappers/graphics/policies/GenericImageCreateAllocatePolicy.cpp"
    "src/starlight/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.cpp"
    "src/starlight/wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.cpp"
    "src/starlight/wrappers/graphics/policies/CreateDescriptorsOnEventPolicy.cpp"
    "src/starlight/wrappers/graphics/StarBuffers/Buffer.cpp"
    "src/starlight/wrappers/graphics/StarBuffers/Resources.cpp"
    "src/starlight/wrappers/graphics/QueueFamilyIndices.cpp"
    "src/starlight/managers/MapManager.cpp" 
    "src/starlight/managers/ManagerRenderResource.cpp"
    "src/starlight/core/HandleContainer.cpp"
    "src/starlight/core/LinearHandleContainer.cpp"
    "src/starlight/core/MappedHandleContainer.cpp"
    "src/starlight/core/ManagedHandleContainer.cpp"
    "src/starlight/core/device/managers/ManagerBase.cpp"
    "src/starlight/core/device/managers/ManagerCommandBuffer.cpp"
    "src/starlight/core/device/managers/Shader.cpp"
    "src/starlight/core/device/managers/Pipeline.cpp"
    "src/starlight/core/device/managers/Manager.cpp"
    "src/starlight/core/device/managers/ManagerEventBusTies.cpp"
    "src/starlight/core/device/managers/TaskCreatedResourceManager.cpp"
    "src/starlight/core/device/managers/GraphicsContainer.cpp"
    "src/starlight/core/device/managers/Semaphore.cpp"
    "src/starlight/core/device/managers/Fence.cpp"
    "src/starlight/core/device/managers/DescriptorPool.cpp"
    "src/starlight/core/device/managers/Image.cpp"
    "src/starlight/core/device/managers/Queue.cpp"
    "src/starlight/core/device/system/event/ShaderCompiled.cpp"
    "src/starlight/core/device/system/event/ManagerRequest.cpp"
    "src/starlight/event/ConsumeDescriptorRequests.cpp"
    "src/starlight/event/EnginePhaseComplete.cpp"
    "src/starlight/event/TriggerScreenshot.cpp"
    "src/starlight/event/RenderReadyForFinalization.cpp"
    "src/starlight/event/PrepForNextFrame.cpp"
    "src/starlight/event/FrameComplete.cpp"
    "src/starlight/event/RegisterMainGraphicsRenderer.cpp"
    "src/starlight/event/StartOfNextFrame.cpp"
    "src/starlight/policy/ListenForPrepForNextFramePolicy.cpp"
    "src/starlight/policy/ListenForFrameCompletePolicy.cpp"
    "src/starlight/policy/ListenForEnginePhaseCompletePolicy.cpp"
    "src/starlight/policy/ListenForRegisterMainGraphicsRendererPolicy.cpp"
    "src/starlight/policy/ListenForRenderReadyForFinalization.cpp"
    "src/starlight/policy/ListenForStartOfNextFramePolicy.cpp"
    "src/starlight/core/service/Service.cpp"
    "src/starlight/service/FrameInFlightControllerService.cpp"
    "src/starlight/service/ScreenCaptureFactory.cpp"
    "src/starlight/service/ScreenCapture.cpp"
    "src/starlight/service/detail/screen_capture/WorkerControllerPolicies.cpp"
    "src/starlight/service/detail/screen_capture/CreateDependenciesPolicies.cpp"
    "src/starlight/service/detail/screen_capture/CopyDirectorPolicy.cpp"
    "src/starlight/service/detail/screen_capture/CalleeRenderDependencies.cpp"
    "src/starlight/service/detail/screen_capture/DeviceInfo.cpp"
    "src/starlight/service/detail/screen_capture/GPUSynchronizationInfo.cpp"
    "src/starlight/service/detail/screen_capture/CapabilityCache.cpp"
    "src/starlight/service/detail/screen_capture/CopyRouter.cpp"
    "src/starlight/service/detail/screen_capture/CopyPlan.cpp"
    "src/starlight/service/detail/screen_capture/PerExtentResources.cpp"
    "src/starlight/service/detail/screen_capture/CopyResourceContainer.cpp"
    "src/starlight/service/detail/screen_capture/BlitCmdPolicy.cpp"
    "src/starlight/service/detail/screen_capture/CopyCmdPolicy.cpp"
    "src/starlight/service/detail/screen_capture/ExecuteCmdBuffer.cpp"
    "src/starlight/core/waiter/sync_renderer/Factory.cpp"
    "src/starlight/core/waiter/sync_renderer/SyncTargetRendererTimeline.cpp"
    "src/starlight/core/waiter/sync_renderer/SyncTargetRendererBinary.cpp"
    "src/starlight/core/waiter/sync_renderer/SyncTargetRenderer.cpp"
    "src/starlight/service/HeadlessRenderResultWriteService.cpp"
    "src/starlight/core/service/InitParameters.cpp"
    "src/starlight/templates/StarApplication.cpp"
    "src/starlight/systems/StarRenderGroup.cpp" 
    "src/starlight/common/Compiler.cpp"
    "src/starlight/common/RuntimeUpdateTexture.cpp"
    "src/starlight/common/ThreadSharedResource.cpp"
    "src/starlight/common/materials/BumpMaterial.cpp"
    "src/starlight/common/materials/VertColorMaterial.cpp"
    "src/starlight/common/materials/HeightDisplacementMaterial.cpp"
    "src/starlight/common/objects/BasicObject.cpp"
    "src/starlight/common/objects/Square.cpp"
    "src/starlight/common/objects/Grid.cpp"
    "src/starlight/common/objects/StarObjectInstance.cpp"
    "src/starlight/common/ConfigFile.cpp"
    "src/starlight/common/buffers/TransferRequest_GlobalInfo.cpp"
    "src/starlight/common/buffers/TransferRequest_InstanceModelInfo.cpp"
    "src/starlight/common/buffers/TransferRequest_InstanceNormalInfo.cpp"
    "src/starlight/common/buffers/TransferRequest_LightInfo.cpp"
    "src/starlight/common/buffers/TransferRequest_LightList.cpp"
    "src/starlight/common/buffers/TransferRequest_IndiciesInfo.cpp"
    "src/starlight/common/buffers/TransferRequest_VertInfo.cpp"
    "src/starlight/common/textures/TransferRequest_TextureFile.cpp"
    "src/starlight/common/textures/TransferRequest_CompressedTextureFile.cpp"
    "src/starlight/common/textures/TransferRequest_TextureData.cpp"
    "src/starlight/common/textures/SharedCompressedTexture.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_TextureFile.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_GlobalInfo.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_InstanceModelInfo.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_InstanceNormalInfo.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_LightInfo.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_LightList.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_IndicesInfo.cpp"
    "src/starlight/common/controllers/ManagerController_RenderResource_VertInfo.cpp"
    "src/starlight/common/materials/TextureMaterial.cpp"
    "src/starlight/common/helpers/GeometryHelpers.cpp"
    "src/starlight/common/helpers/FileHelpers.cpp"
    "src/starlight/internals/CommandBufferContainer.cpp"
    "src/starlight/job/worker/Worker.cpp"
    "src/starlight/job/worker/DefaultWorker.cpp"
    "src/starlight/job/worker/default_worker/detail/ThreadTaskHandlingPolicies.cpp"
    "src/starlight/job/TransferWorker.cpp"
    "src/starlight/internals/ManagerStorageContainer.cpp"
    "src/starlight/job/TaskContainer.cpp"
    "src/starlight/job/complete_tasks/CompleteTask.cpp"
    "src/starlight/job/complete_tasks/TaskFactory.cpp"
    "src/starlight/job/complete_tasks/BuildPipeline.cpp"
    "src/starlight/job/complete_tasks/CompileShader.cpp"
    "src/starlight/job/tasks/Task.cpp"
    "src/starlight/job/tasks/TaskFactory.cpp"
    "src/starlight/job/tasks/WriteImageToDisk.cpp"
    "src/starlight/job/tasks/CompileShader.cpp"
    "src/starlight/job/tasks/BuildPipeline.cpp"
    "src/starlight/job/TaskManager.cpp"
    "src/starlight/job/ImageWriterWorker.cpp"
    "src/starlight/job/FrameScheduler.cpp"
    "src/starlight/core/device/StarDevice.cpp"
    "src/starlight/core/device/DeviceContext.cpp"
    "src/starlight/core/SystemContext.cpp"
    "src/starlight/core/RenderingInstance.cpp"
    "src/starlight/core/SwapChainSupportDetails.cpp"
    "src/starlight/core/graphics/shader/BasicIncluder.cpp"
    "src/starlight/policy/DefaultEngineInitPolicy.cpp"
    "src/starlight/policy/EngineExitAfterNumberOfFrames.cpp"
    "src/starlight/policy/DefaultEngineLoopPolicy.cpp"
    "src/starlight/core/renderer/HeadlessRenderer.cpp"
)

set(${STARLIGHT_NAME}_TARGET_SOURCE ${${STARLIGHT_NAME}_SOURCE} PARENT_SCOPE)

set(${STARLIGHT_NAME}_HEADERS
    "include/starlight/StarEngine.hpp"
    "include/starlight/data_structure/dynamic/ThreadSharedObjectPool.hpp"
    "include/starlight/graphics/PipelineFactory.hpp"
    "src/starlight/core/sysinfo/SystemInfo.hpp"
    "include/starlight/core/logging/LoggingFactory.hpp"
    "include/starlight/core/renderer/RendererBase.hpp"
    "include/starlight/core/renderer/LightRenderer.hpp"
    "include/starlight/core/renderer/DefaultRenderer.hpp"
    "include/starlight/core/renderer/RenderingTargetInfo.hpp"
    "include/starlight/core/renderer/RenderingContext.hpp"
    "include/starlight/core/renderer/CaptureCapableRenderer.hpp"
    "include/starlight/common/ThreadSharedResource.hpp"
    "include/starlight/common/Compiler.hpp"
    "include/starlight/common/materials/BumpMaterial.hpp"
    "include/starlight/common/materials/VertColorMaterial.hpp"
    "include/starlight/common/materials/TextureMaterial.hpp"
    "include/starlight/common/objects/BasicObject.hpp"
    "include/starlight/common/objects/Square.hpp"
    "include/starlight/common/objects/StarObjectInstance.hpp"
    "include/starlight/common/RuntimeUpdateTexture.hpp"
    "include/starlight/common/buffers/TransferRequest_GlobalInfo.hpp"
    "include/starlight/common/buffers/TransferRequest_InstanceModelInfo.hpp"
    "include/starlight/common/buffers/TransferRequest_InstanceNormalInfo.hpp"
    "include/starlight/common/buffers/TransferRequest_IndicesInfo.hpp"
    "include/starlight/common/buffers/TransferRequest_VertInfo.hpp"
    "include/starlight/common/buffers/TransferRequest_LightInfo.hpp"
    "include/starlight/common/buffers/TransferRequest_LightList.hpp"
    "include/starlight/common/textures/TransferRequest_TextureFile.hpp"
    "include/starlight/common/textures/TransferRequest_CompressedTextureFile.hpp"
    "include/starlight/common/textures/TransferRequest_TextureData.hpp"
    "include/starlight/common/textures/SharedCompressedTexture.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_TextureFile.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_InstanceModelInfo.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_InstanceNormalInfo.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_LightInfo.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_LightList.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_IndicesInfo.hpp"
    "include/starlight/common/controllers/ManagerController_RenderResource_VertInfo.hpp"
    "include/starlight/virtual/ModulePlug/ManagerPlug.hpp"
    "include/starlight/virtual/StarManager.hpp"
    "include/starlight/virtual/StarObject.hpp"
    "include/starlight/virtual/StarScene.hpp"
    "include/starlight/virtual/StarMesh.hpp"
    "include/starlight/virtual/StarCamera.hpp"
    "include/starlight/virtual/StarMaterial.hpp"
    "include/starlight/virtual/StarPipeline.hpp"
    "include/starlight/virtual/TransferRequest_Texture.hpp"
    "include/starlight/virtual/TransferRequest_Buffer.hpp"
    "include/starlight/virtual/TransferRequest_Memory.hpp"
    "include/starlight/virtual/ManagerController_Controller.hpp"
    "include/starlight/virtual/ManagerController_RenderResource_Buffer.hpp"
    "include/starlight/virtual/ManagerController_RenderResource_Texture.hpp"
    "include/starlight/wrappers/graphics/StarDescriptorBuilders.hpp"
    "include/starlight/wrappers/graphics/StarGraphicsPipeline.hpp"
    "include/starlight/wrappers/graphics/StarComputePipeline.hpp"
    "include/starlight/wrappers/graphics/StarRenderPass.hpp"
    "include/starlight/wrappers/graphics/StarCommandBuffer.hpp"
    "include/starlight/wrappers/graphics/StarShaderInfo.hpp"
    "include/starlight/wrappers/graphics/StarQueueFamily.hpp"
    "include/starlight/wrappers/graphics/StarQueue.hpp"
    "include/starlight/wrappers/graphics/StarCommandPool.hpp"
    "include/starlight/wrappers/Allocator.hpp"
    "include/starlight/wrappers/graphics/QueueFamilyIndices.hpp"
    "include/starlight/managers/MapManager.hpp"
    "include/starlight/core/device/managers/ManagerBase.hpp"
    "include/starlight/core/device/managers/ManagerCommandBuffer.hpp"
    "include/starlight/core/device/managers/Shader.hpp"
    "include/starlight/core/device/managers/Pipeline.hpp"
    "include/starlight/core/device/managers/Manager.hpp"
    "include/starlight/core/device/managers/TaskCreatedResourceManager.hpp"
    "include/starlight/core/device/managers/ManagerEventBusTies.hpp"
    "include/starlight/core/device/managers/Semaphore.hpp"
    "include/starlight/core/device/managers/Fence.hpp"
    "include/starlight/core/device/managers/GraphicsContainer.hpp"
    "include/starlight/core/device/managers/DescriptorPool.hpp"
    "include/starlight/core/device/managers/Image.hpp"
    "include/starlight/core/device/managers/Queue.hpp"
    "include/starlight/core/device/system/event/ShaderCompiled.hpp"
    "include/starlight/core/device/system/event/ManagerRequest.hpp"
    "include/starlight/event/ConsumeDescriptorRequests.hpp"
    "include/starlight/event/EnginePhaseComplete.hpp"
    "include/starlight/event/TriggerScreenshot.hpp"
    "include/starlight/event/RenderReadyForFinalization.hpp"
    "include/starlight/event/PrepForNextFrame.hpp"
    "include/starlight/event/FrameComplete.hpp"
    "include/starlight/event/RegisterMainGraphicsRenderer.hpp"
    "include/starlight/event/StartOfNextFrame.hpp"
    "include/starlight/policy/ListenForFrameCompletePolicy.hpp"
    "include/starlight/policy/ListenForPrepForNextFramePolicy.hpp"
    "include/starlight/policy/ListenForEnginePhaseCompletePolicy.hpp"
    "include/starlight/policy/ListenForRegisterMainGraphicsRendererPolicy.hpp"
    "include/starlight/policy/ListenForStartOfNextFramePolicy.hpp"
    "include/starlight/core/graphics/shader/BasicIncluder.hpp"
    "include/starlight/service/Service.hpp"
    "include/starlight/service/FrameInFlightControllerService.hpp"
    "include/starlight/service/ScreenCapture.hpp"
    "include/starlight/service/ScreenCaptureFactory.hpp"
    "include/starlight/service/detail/screen_capture/WorkerControllerPolicies.hpp"
    "include/starlight/service/detail/screen_capture/CreateDependenciesPolicies.hpp"
    "include/starlight/service/detail/screen_capture/CopyDirectorPolicy.hpp"
    "include/starlight/service/detail/screen_capture/CalleeRenderDependencies.hpp"
    "include/starlight/service/detail/screen_capture/DeviceInfo.hpp"
    "include/starlight/service/detail/screen_capture/GPUSynchronizationInfo.hpp"
    "include/starlight/service/detail/screen_capture/CapabilityCache.hpp"
    "include/starlight/service/detail/screen_capture/CopyRouter.hpp"
    "include/starlight/service/detail/screen_capture/CopyPlan.hpp"
    "include/starlight/service/detail/screen_capture/PerExtentResources.hpp"
    "include/starlight/service/detail/screen_capture/CopyResourcesContainer.hpp"
    "include/starlight/service/detail/screen_capture/Common.hpp"
    "include/starlight/service/detail/screen_capture/BlitCmdPolicy.hpp"
    "include/starlight/service/detail/screen_capture/CopyCmdPolicy.hpp"
    "include/starlight/service/detail/screen_capture/ExecuteCmdBuffer.hpp"
    "include/starlight/core/waiter/sync_renderer/Factory.hpp"
    "include/starlight/core/waiter/sync_renderer/SyncTargetRendererTimeline.hpp"
    "include/starlight/core/waiter/sync_renderer/SyncTargetRendererBinary.hpp"
    "include/starlight/core/waiter/sync_renderer/SyncTargetRenderer.hpp"
    "include/starlight/service/InitParameters.hpp"
    "include/starlight/service/HeadlessRenderResultWriteService.hpp"
    "include/starlight/managers/ManagerRenderResource.hpp"
    "include/starlight/systems/StarRenderGroup.hpp"
    "include/starlight/internals/CommandBufferContainer.hpp"
    "include/starlight/internals/ManagerStorageContainer.hpp"
    "include/starlight/virtual/StarEntity.hpp"
    "include/starlight/wrappers/graphics/StarTextures/FormatInfo.hpp"
    "include/starlight/wrappers/graphics/StarTextures/Texture.hpp"
    "include/starlight/wrappers/graphics/policies/GenericImageCreateAllocatePolicy.hpp"
    "include/starlight/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"
    "include/starlight/wrappers/graphics/policies/SubmitDescriptorRequestsPolicy.hpp"
    "include/starlight/wrappers/graphics/policies/CreateDescriptorsOnEventPolicy.hpp"
    "include/starlight/wrappers/graphics/StarTextures/Resources.hpp"
    "include/starlight/wrappers/graphics/StarTextures/AllocatedResources.hpp"
    "include/starlight/wrappers/graphics/StarBuffers/Buffer.hpp"
    "include/starlight/wrappers/graphics/StarBuffers/Resources.hpp"
    "include/starlight/job/TransferWorker.hpp"
    "include/starlight/job/worker/DefaultWorker.hpp"
    "include/starlight/job/worker/Worker.hpp"
    "include/starlight/job/worker/default_worker/detail/ThreadTaskHandlingPolicies.hpp"
    "include/starlight/job/TaskContainer.hpp"
    "include/starlight/job/complete_tasks/CompleteTask.hpp"
    "include/starlight/job/complete_tasks/TaskFactory.hpp"
    "include/starlight/job/complete_tasks/BuildPipeline.hpp"
    "include/starlight/job/complete_tasks/CompileShader.hpp"
    "include/starlight/job/tasks/Task.hpp"
    "include/starlight/job/tasks/TaskFactory.hpp"
    "include/starlight/job/tasks/WriteImageToDisk.hpp"
    "include/starlight/job/tasks/CompileShader.hpp"
    "include/starlight/job/tasks/BuildPipeline.hpp"
    "include/starlight/job/TaskManager.hpp"
    "include/starlight/job/ImageWriterWorker.hpp"
    "include/starlight/job/FrameScheduler.hpp"
    "include/starlight/core/HandleContainer.hpp"
    "include/starlight/core/MappedHandleContainer.hpp"
    "include/starlight/core/LinearHandleContainer.hpp"
    "include/starlight/core/ManagedHandleContainer.hpp"
    "include/starlight/core/device/StarDevice.hpp"
    "include/starlight/core/device/DeviceContext.hpp"
    "include/starlight/core/SystemContext.hpp"
    "include/starlight/core/RenderingInstance.hpp"
    "include/starlight/core/SwapChainSupportDetails.hpp"
    "include/starlight/policy/DefaultEngineInitPolicy.hpp"
    "include/starlight/policy/EngineExitAfterNumberOfFrames.hpp"
    "include/starlight/policy/DefaultEngineLoopPolicy.hpp"
    "include/starlight/policy/ListenForRenderReadyForFinalization.hpp"
    "include/starlight/core/Exceptions.hpp"
    "include/starlight/core/renderer/HeadlessRenderer.hpp"
    #header only
    "include/starlight/util/PhysicalDeviceLogging.hpp"
    "include/starlight/common/entities/Light.hpp"
    "include/starlight/virtual/StarShader.hpp"
    "include/starlight/enums/Enums.hpp"
    "include/starlight/common/BufferHandle.hpp"
    "include/starlight/structs/LightBufferObject.hpp"
    "include/starlight/structs/Vertex.hpp"
    "include/starlight/structs/Color.hpp"
    "include/starlight/structs/Ray.hpp"
    "include/starlight/templates/StarApplication.hpp"
    "include/starlight/templates/StarResourceContainer.hpp"
    "include/starlight/templates/FileResourceManager.hpp"
    "include/starlight/templates/FileResourceContainer.hpp"
    "include/starlight/common/ConfigFile.hpp"
    "include/starlight/common/helpers/FileHelpers.hpp"
    "include/starlight/common/helpers/Time.hpp"
    "include/starlight/common/VulkanVertex.hpp"
)
set(${STARLIGHT_NAME}_TARGET_HEADERS ${${STARLIGHT_NAME}_HEADERS} PARENT_SCOPE)

add_library(${STARLIGHT_NAME} "${${STARLIGHT_NAME}_SOURCE};${${STARLIGHT_NAME}_HEADERS}")

set(${STARLIGHT_NAME}_INCLUDE_DIRS
    "include/starlight/"
    "include/starlight/virtual/"
    "include/starlight/internals"
    "include/starlight/virtual/ModulePlug/"
    "include/starlight/wrappers/"
    "include/starlight/wrappers/graphics/"
    "include/starlight/structs/"
    "include/starlight/enums/"
    "include/starlight/templates/"
    "include/starlight/managers/"
    "include/starlight/common/"
    "include/starlight/common/entities/"
    "include/starlight/common/materials/"
    "include/starlight/common/helpers/"
    "include/starlight/common/objects/"
    "include/starlight/common/buffers/"
    "include/starlight/common/textures"
    "include/starlight/common/controllers/"
    "include/starlight/builders/"
    "include/starlight/controllers/"
    "include/starlight/modules/"
    "include/starlight/systems/"
    "include/starlight/structs/"
    "include/starlight/renderers/" 
    "include/starlight/job/"
    "include/starlight/core/"
)

target_link_libraries(${STARLIGHT_NAME}
    PUBLIC
        Starlight::starlight_common
        nlohmann_json::nlohmann_json
        Vulkan::Vulkan
        shaderc_combined
        GPUOpen::VulkanMemoryAllocator
    PRIVATE
        tinyobjloader::tinyobjloader
        GPUOpen::VulkanMemoryAllocator
        Boost::log
        Boost::log_setup
        Boost::thread
        Boost::container
        Boost::filesystem
        KTX::ktx
        absl::hash
)

set(${STARLIGHT_NAME}_TARGET_INCLUDE_DIRS ${${STARLIGHT_NAME}_INCLUDE_DIRS} PARENT_SCOPE)

target_include_directories(${STARLIGHT_NAME}
    PUBLIC
        # --- Build interface (source-tree paths) ---
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/virtual>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/internals>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/virtual/ModulePlug>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/wrappers>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/wrappers/graphics>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/structs>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/enums>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/templates>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/managers>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/objects>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/buffers>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/entities>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/controllers>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/job>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/systems>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/materials>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/textures>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/starlight/common/helpers>
        # --- Install interface    # --- Install interface (install-tree paths) ---
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${glm_SOURCE_DIR}>
        $<BUILD_INTERFACE:${Stb_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (MSVC)
    target_compile_options(${STARLIGHT_NAME} PRIVATE
        /Zi 
        /MP
        /EHsc
    )
    target_link_options(${STARLIGHT_NAME} PRIVATE 
        /DEBUG
    )
endif()

find_program(CPPCHECK_EXECUTABLE cppcheck)
if (CPPCHECK_EXECUTABLE)
	set(CPPCHECK_FILES "")
    set(CPPCHECK_INCLUDES "")

	foreach(FILE "${${STARLIGHT_NAME}_SOURCE}")
		file(TO_CMAKE_PATH "${FILE}" UNIX_PATH)
    list(APPEND CPPCHECK_FILES "${UNIX_PATH}")
	endforeach()

    foreach(DIR ${${STARLIGHT_NAME}_INCLUDE_DIRS})
        list(APPEND CPPCHECK_INCLUDES "-I" "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
    endforeach()

	add_custom_target("${STARLIGHT_NAME}_cppcheck"
		COMMAND ${CPPCHECK_EXECUTABLE} 
			--enable=all 
            --suppress=missingIncludeSystem
			-q
			--inline-suppr
			--language=c++
			--std=c++20
			--template=gcc
            ${CPPCHECK_INCLUDES}
			${CPPCHECK_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		COMMENT "Running Cppcheck"
		VERBATIM
	)

	add_dependencies("${STARLIGHT_NAME}_cppcheck" ${CMAKE_PROJECT_NAME})
endif()

set_target_properties(${STARLIGHT_NAME} PROPERTIES 
    DEBUG_POSTFIX "d"
)

add_library(Starlight::starlight ALIAS starlight)

include(GNUInstallDirs)

install(TARGETS ${STARLIGHT_NAME} shaderc_combined
    EXPORT starlightTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll/.so with runtime on Windows
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so/.dylib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .a/.lib
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install your public headers under include/starlight/ (adjust if you want a different prefix)

# Install the whole include directory so the same subfolders exist:
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/starlightConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/starlightConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/starlight
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/starlightConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT starlightTargets
    FILE starlightTargets.cmake
    NAMESPACE Starlight::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/starlight
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/starlightConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/starlightConfigVersion.cmake
       DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/starlight
)