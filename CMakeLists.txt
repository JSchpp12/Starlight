
cmake_minimum_required (VERSION 3.26)

SET(STARLIGHT_NAME "starlight")
project(${STARLIGHT_NAME}
    LANGUAGES CXX C
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

find_package(StarlightCommon REQUIRED)

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
if (NOT ${HEADLESS})
    message("Enabling windowing support.")
    add_subdirectory("extern/glfw")
else()
    message("Disabling windowing support.")
endif()

find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS container thread filesystem log log_setup)
find_package(Ktx CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(absl CONFIG REQUIRED)

# add all necessary dependencies
include(GNUInstallDirs)

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_reflect")

set(SHADERC_ENABLE_GLSLC OFF CACHE BOOL "Disable building glslc" FORCE)
set(SHADERC_ENABLE_RTTI ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
add_subdirectory("extern/shaderc")

set(${STARLIGHT_NAME}_SOURCE
    "src/StarEngine.cpp" 
    "src/data_structure/dynamic/ThreadSharedObjectPool.cpp"
    "src/graphics/PipelineFactory.cpp"
    "src/core/logging/LoggingFactory.cpp"
    "src/core/renderer/LightRenderer.cpp"
    "src/core/renderer/RendererBase.cpp"
    "src/core/renderer/Renderer.cpp"
    "src/core/renderer/SwapChainRenderer.cpp"
    "src/core/renderer/RenderingTargetInfo.cpp"
    "src/core/renderer/RenderingContext.cpp"
    "src/core/renderer/CaptureCapableRenderer.cpp"
    "src/virtual/StarObject.cpp"
    "src/virtual/ModulePlug/RenderResourceModifier.cpp"
    "src/virtual/ModulePlug/DescriptorModifier.cpp"
    "src/virtual/ModulePlug/ManagerPlug.cpp"
    "src/virtual/StarMesh.cpp"
    "src/virtual/StarManager.cpp"
    "src/virtual/StarWindow.cpp" 
    "src/virtual/StarScene.cpp"
    "src/virtual/StarShader.cpp" 
    "src/virtual/StarMaterial.cpp" 
    "src/virtual/StarCamera.cpp"
    "src/virtual/StarPipeline.cpp"
    "src/virtual/StarEntity.cpp"
    "src/virtual/TransferRequest_Texture.cpp"
    "src/virtual/TransferRequest_Memory.cpp"
    "src/virtual/TransferRequest_Buffer.cpp"
    "src/virtual/ManagerController_Controller.cpp"
    "src/virtual/ManagerController_RenderResource_Buffer.cpp"
    "src/virtual/ManagerController_RenderResource_Texture.cpp"
    "src/wrappers/graphics/StarDescriptorBuilders.cpp"  
    "src/wrappers/graphics/StarGraphicsPipeline.cpp"  
    "src/wrappers/graphics/StarComputePipeline.cpp"
    "src/wrappers/graphics/StarRenderPass.cpp"
    "src/wrappers/graphics/StarCommandBuffer.cpp"
    "src/wrappers/graphics/StarShaderInfo.cpp"
    "src/wrappers/graphics/StarQueueFamily.cpp"
    "src/wrappers/graphics/StarQueue.cpp"
    "src/wrappers/graphics/StarCommandPool.cpp"
    "src/wrappers/Allocator.cpp"
    "src/wrappers/graphics/StarTextures/FormatInfo.cpp"
    "src/wrappers/graphics/StarTextures/Texture.cpp"
    "src/wrappers/graphics/StarTextures/Resources.cpp"
    "src/wrappers/graphics/StarTextures/AllocatedResources.cpp"
    "src/wrappers/graphics/policies/GenericImageCreateAllocatePolicy.cpp"
    "src/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.cpp"
    "src/wrappers/graphics/StarBuffers/Buffer.cpp"
    "src/wrappers/graphics/StarBuffers/Resources.cpp"
    "src/managers/MapManager.cpp" 
    "src/managers/ManagerDescriptorPool.cpp"
    "src/core/HandleContainer.cpp"
    "src/core/LinearHandleContainer.cpp"
    "src/core/MappedHandleContainer.cpp"
    "src/core/ManagedHandleContainer.cpp"
    "src/core/device/managers/ManagerBase.cpp"
    "src/core/device/managers/ManagerCommandBuffer.cpp"
    "src/core/device/managers/Shader.cpp"
    "src/core/device/managers/Pipeline.cpp"
    "src/core/device/managers/Manager.cpp"
    "src/core/device/managers/ManagerEventBusTies.cpp"
    "src/core/device/managers/TaskCreatedResourceManager.cpp"
    "src/core/device/managers/GraphicsContainer.cpp"
    "src/core/device/managers/Semaphore.cpp"
    "src/core/device/managers/Fence.cpp"
    "src/core/device/system/EventBus.cpp"
    "src/core/device/system/SubscriberCallbackInfo.cpp"
    "src/core/device/system/event/ShaderCompiled.cpp"
    "src/core/device/system/event/ManagerRequest.cpp"
    "src/core/device/system/event/StartOfNextFrame.cpp"
    "src/event/TriggerScreenshot.cpp"
    "src/core/service/Service.cpp"
    "src/service/ScreenCaptureFactory.cpp"
    "src/service/ScreenCapture.cpp"
    "src/service/detail/screen_capture/WorkerControllerPolicies.cpp"
    "src/service/detail/screen_capture/CreateDependenciesPolicies.cpp"
    "src/service/detail/screen_capture/CopyDirectorPolicy.cpp"
    "src/service/detail/screen_capture/CalleeRenderDependencies.cpp"
    "src/service/detail/screen_capture/DeviceInfo.cpp"
    "src/service/detail/screen_capture/GPUSynchronizationInfo.cpp"
    "src/service/detail/screen_capture/CapabilityCache.cpp"
    "src/service/detail/screen_capture/CopyRouter.cpp"
    "src/service/detail/screen_capture/CopyPlan.cpp"
    "src/service/detail/screen_capture/PerExtentResources.cpp"
    "src/service/detail/screen_capture/CopyResourceContainer.cpp"
    "src/service/detail/screen_capture/BlitCmdPolicy.cpp"
    "src/service/detail/screen_capture/CopyCmdPolicy.cpp"
    "src/service/detail/screen_capture/ExecuteCmdBuffer.cpp"
    "src/core/service/InitParameters.cpp"
    "src/managers/ManagerRenderResource.cpp"
    "src/templates/StarApplication.cpp"
    "src/modules/InteractionSystem.cpp"
    "src/modules/RenderResourceSystem.cpp"
    "src/systems/StarRenderGroup.cpp" 
    "src/common/Compiler.cpp"
    "src/common/Interactivity.cpp" 
    "src/common/KeyStates.cpp"
    "src/common/RuntimeUpdateTexture.cpp"
    "src/common/ThreadSharedResource.cpp"
    "src/common/materials/BumpMaterial.cpp"
    "src/common/materials/VertColorMaterial.cpp"
    "src/common/materials/HeightDisplacementMaterial.cpp"
    "src/common/objects/BasicObject.cpp"
    "src/common/objects/Square.cpp"
    "src/common/objects/Grid.cpp"
    "src/common/objects/StarObjectInstance.cpp"
    "src/common/ConfigFile.cpp"
    "src/common/BasicCamera.cpp"
    "src/common/buffers/TransferRequest_GlobalInfo.cpp"
    "src/common/buffers/TransferRequest_InstanceModelInfo.cpp"
    "src/common/buffers/TransferRequest_InstanceNormalInfo.cpp"
    "src/common/buffers/TransferRequest_LightInfo.cpp"
    "src/common/buffers/TransferRequest_LightList.cpp"
    "src/common/buffers/TransferRequest_IndiciesInfo.cpp"
    "src/common/buffers/TransferRequest_VertInfo.cpp"
    "src/common/textures/TransferRequest_TextureFile.cpp"
    "src/common/textures/TransferRequest_CompressedTextureFile.cpp"
    "src/common/textures/TransferRequest_TextureData.cpp"
    "src/common/textures/SharedCompressedTexture.cpp"
    "src/common/controllers/ManagerController_RenderResource_TextureFile.cpp"
    "src/common/controllers/ManagerController_RenderResource_GlobalInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_InstanceModelInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_InstanceNormalInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_LightInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_LightList.cpp"
    "src/common/controllers/ManagerController_RenderResource_IndicesInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_VertInfo.cpp"
    "src/common/materials/TextureMaterial.cpp"
    "src/common/helpers/GeometryHelpers.cpp"
    "src/common/helpers/FileHelpers.cpp"
    "src/internals/CommandBufferContainer.cpp"
    "src/job/worker/Worker.cpp"
    "src/job/worker/DefaultWorker.cpp"
    "src/job/worker/default_worker/detail/ThreadTaskHandlingPolicies.cpp"
    "src/job/TransferWorker.cpp"
    "src/internals/ManagerStorageContainer.cpp"
    "src/job/TaskContainer.cpp"
    "src/job/complete_tasks/CompleteTask.cpp"
    "src/job/complete_tasks/TaskFactory.cpp"
    "src/job/complete_tasks/BuildPipeline.cpp"
    "src/job/complete_tasks/CompileShader.cpp"
    "src/job/tasks/Task.cpp"
    "src/job/tasks/TaskFactory.cpp"
    "src/job/tasks/WriteImageToDisk.cpp"
    "src/job/tasks/CompileShader.cpp"
    "src/job/tasks/BuildPipeline.cpp"
    "src/job/TaskManager.cpp"
    "src/job/ImageWriterWorker.cpp"
    "src/job/FrameScheduler.cpp"
    
    "src/core/device/StarDevice.cpp"
    "src/core/device/DeviceContext.cpp"
    "src/core/device/FrameInFlightTracking.cpp"
    "src/core/SystemContext.cpp"
    "src/core/RenderingInstance.cpp"
    "src/core/RenderingSurface.cpp"
    "src/core/SwapChainSupportDetails.cpp"
    "src/core/graphics/shader/BasicIncluder.cpp"
)

set(${STARLIGHT_NAME}_TARGET_SOURCE ${${STARLIGHT_NAME}_SOURCE} PARENT_SCOPE)

set(${STARLIGHT_NAME}_HEADERS
    "include/StarEngine.hpp"
    "include/data_structure/dynamic/ThreadSharedObjectPool.hpp"
    "include/graphics/PipelineFactory.hpp"
    "include/core/logging/LoggingFactory.hpp"
    "include/core/renderer/RendererBase.hpp"
    "include/core/renderer/LightRenderer.hpp"
    "include/core/renderer/Renderer.hpp"
    "include/core/renderer/SwapChainRenderer.hpp"
    "include/core/renderer/RenderingTargetInfo.hpp"
    "include/core/renderer/RenderingContext.hpp"
    "include/core/renderer/CaptureCapableRenderer.hpp"
    "include/common/ThreadSharedResource.hpp"
    "include/common/Compiler.hpp"
    "include/common/Interactivity.hpp"
    "include/common/KeyStates.hpp"
    "include/common/materials/BumpMaterial.hpp"
    "include/common/materials/VertColorMaterial.hpp"
    "include/common/materials/TextureMaterial.hpp"
    "include/common/objects/BasicObject.hpp"
    "include/common/objects/Square.hpp"
    "include/common/objects/StarObjectInstance.hpp"
    "include/common/BasicCamera.hpp"
    "include/common/RuntimeUpdateTexture.hpp"
    "include/common/buffers/TransferRequest_GlobalInfo.hpp"
    "include/common/buffers/TransferRequest_InstanceModelInfo.hpp"
    "include/common/buffers/TransferRequest_InstanceNormalInfo.hpp"
    "include/common/buffers/TransferRequest_IndicesInfo.hpp"
    "include/common/buffers/TransferRequest_VertInfo.hpp"
    "include/common/buffers/TransferRequest_LightInfo.hpp"
    "include/common/buffers/TransferRequest_LightList.hpp"
    "include/common/textures/TransferRequest_TextureFile.hpp"
    "include/common/textures/TransferRequest_CompressedTextureFile.hpp"
    "include/common/textures/TransferRequest_TextureData.hpp"
    "include/common/textures/SharedCompressedTexture.hpp"
    "include/common/controllers/ManagerController_RenderResource_TextureFile.hpp"
    "include/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_InstanceModelInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_InstanceNormalInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_LightInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_LightList.hpp"
    "include/common/controllers/ManagerController_RenderResource_IndicesInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_VertInfo.hpp"
    "include/virtual/ModulePlug/RenderResourceModifier.hpp"
    "include/virtual/ModulePlug/DescriptorModifier.hpp"
    "include/virtual/ModulePlug/ManagerPlug.hpp"
    "include/virtual/StarManager.hpp"
    "include/virtual/StarWindow.hpp" 
    "include/virtual/StarObject.hpp"
    "include/virtual/StarScene.hpp"
    "include/virtual/StarMesh.hpp"
    "include/virtual/StarCamera.hpp"
    "include/virtual/StarMaterial.hpp"
    "include/virtual/StarPipeline.hpp"
    "include/virtual/TransferRequest_Texture.hpp"
    "include/virtual/TransferRequest_Buffer.hpp"
    "include/virtual/TransferRequest_Memory.hpp"
    "include/virtual/ManagerController_Controller.hpp"
    "include/virtual/ManagerController_RenderResource_Buffer.hpp"
    "include/virtual/ManagerController_RenderResource_Texture.hpp"
    "include/wrappers/graphics/StarDescriptorBuilders.hpp"
    "include/wrappers/graphics/StarGraphicsPipeline.hpp"
    "include/wrappers/graphics/StarComputePipeline.hpp"
    "include/wrappers/graphics/StarRenderPass.hpp"
    "include/wrappers/graphics/StarCommandBuffer.hpp"
    "include/wrappers/graphics/StarShaderInfo.hpp"
    "include/wrappers/graphics/StarQueueFamily.hpp"
    "include/wrappers/graphics/StarQueue.hpp"
    "include/wrappers/graphics/StarCommandPool.hpp"
    "include/wrappers/Allocator.hpp"
    "include/managers/MapManager.hpp"
    "include/managers/ManagerDescriptorPool.hpp"
    "include/core/device/managers/ManagerBase.hpp"
    "include/core/device/managers/ManagerCommandBuffer.hpp"
    "include/core/device/managers/Shader.hpp"
    "include/core/device/managers/Pipeline.hpp"
    "include/core/device/managers/Manager.hpp"
    "include/core/device/managers/TaskCreatedResourceManager.hpp"
    "include/core/device/managers/ManagerEventBusTies.hpp"
    "include/core/device/managers/Semaphore.hpp"
    "include/core/device/managers/Fence.hpp"
    "include/core/device/managers/GraphicsContainer.hpp"
    "include/core/device/system/EventBus.hpp"
    "include/core/device/system/SubscriberCallbackInfo.hpp"
    "include/core/device/system/event/ShaderCompiled.hpp"
    "include/core/device/system/event/ManagerRequest.hpp"
    "include/core/device/system/event/StartOfNextFrame.hpp"
    "include/event/TriggerScreenshot.hpp"
    "include/core/graphics/shader/BasicIncluder.hpp"
    "include/service/Service.hpp"
    "include/service/ScreenCapture.hpp"
    "include/service/ScreenCaptureFactory.hpp"
    "include/service/detail/screen_capture/WorkerControllerPolicies.hpp"
    "include/service/detail/screen_capture/CreateDependenciesPolicies.hpp"
    "include/service/detail/screen_capture/CopyDirectorPolicy.hpp"
    "include/service/detail/screen_capture/CalleeRenderDependencies.hpp"
    "include/service/detail/screen_capture/DeviceInfo.hpp"
    "include/service/detail/screen_capture/GPUSynchronizationInfo.hpp"
    "include/service/detail/screen_capture/CapabilityCache.hpp"
    "include/service/detail/screen_capture/CopyRouter.hpp"
    "include/service/detail/screen_capture/CopyPlan.hpp"
    "include/service/detail/screen_capture/PerExtentResources.hpp"
    "include/service/detail/screen_capture/CopyResourcesContainer.hpp"
    "include/service/detail/screen_capture/Common.hpp"
    "include/service/detail/screen_capture/BlitCmdPolicy.hpp"
    "include/service/detail/screen_capture/CopyCmdPolicy.hpp"
    "include/service/detail/screen_capture/ExecuteCmdBuffer.hpp"
    "include/service/InitParameters.hpp"
    "include/managers/ManagerRenderResource.hpp"
    "include/modules/InteractionSystem.hpp"
    "include/modules/RenderResourceSystem.hpp"
    "include/systems/StarRenderGroup.hpp"
    "include/internals/CommandBufferContainer.hpp"
    "include/internals/ManagerStorageContainer.hpp"
    "include/virtual/StarEntity.hpp"
    "include/wrappers/graphics/StarTextures/FormatInfo.hpp"
    "include/wrappers/graphics/StarTextures/Texture.hpp"
    "include/wrappers/graphics/policies/GenericImageCreateAllocatePolicy.hpp"
    "include/wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"
    "include/wrappers/graphics/StarTextures/Resources.hpp"
    "include/wrappers/graphics/StarTextures/AllocatedResources.hpp"
    "include/wrappers/graphics/StarBuffers/Buffer.hpp"
    "include/wrappers/graphics/StarBuffers/Resources.hpp"
    "include/job/TransferWorker.hpp"
    "include/job/worker/DefaultWorker.hpp"
    "include/job/worker/Worker.hpp"
    "include/job/worker/default_worker/detail/ThreadTaskHandlingPolicies.hpp"
    "include/job/TaskContainer.hpp"
    "include/job/complete_tasks/CompleteTask.hpp"
    "include/job/complete_tasks/TaskFactory.hpp"
    "include/job/complete_tasks/BuildPipeline.hpp"
    "include/job/complete_tasks/CompileShader.hpp"
    "include/job/tasks/Task.hpp"
    "include/job/tasks/TaskFactory.hpp"
    "include/job/tasks/WriteImageToDisk.hpp"
    "include/job/tasks/CompileShader.hpp"
    "include/job/tasks/BuildPipeline.hpp"
    "include/job/TaskManager.hpp"
    "include/job/ImageWriterWorker.hpp"
    "include/job/FrameScheduler.hpp"
    "include/core/HandleContainer.hpp"
    "include/core/MappedHandleContainer.hpp"
    "include/core/LinearHandleContainer.hpp"
    "include/core/ManagedHandleContainer.hpp"
    "include/core/device/StarDevice.hpp"
    "include/core/device/DeviceContext.hpp"
    "include/core/device/FrameInFlightTracking.hpp"
    "include/core/SystemContext.hpp"
    "include/core/RenderingInstance.hpp"
    "include/core/RenderingSurface.hpp"
    "include/core/SwapChainSupportDetails.hpp"
    #header only
    "include/common/entities/Light.hpp"
    "include/virtual/StarShader.hpp"
    "include/enums/Enums.hpp"
    "include/common/BufferHandle.hpp"
    "include/structs/LightBufferObject.hpp"
    "include/structs/Vertex.hpp"
    "include/structs/Color.hpp"
    "include/structs/Ray.hpp"
    "include/templates/StarApplication.hpp"
    "include/templates/StarResourceContainer.hpp"
    "include/templates/FileResourceManager.hpp"
    "include/templates/FileResourceContainer.hpp"
    "include/common/ConfigFile.hpp"
    "include/common/helpers/DebugHelpers.hpp"
    "include/common/helpers/FileHelpers.hpp"
    "include/common/helpers/Time.hpp"
    "include/common/helpers/CastHelpers.hpp"
    "include/common/VulkanVertex.hpp"
)
set(${STARLIGHT_NAME}_TARGET_HEADERS ${${STARLIGHT_NAME}_HEADERS} PARENT_SCOPE)

add_library(${STARLIGHT_NAME} "${${STARLIGHT_NAME}_SOURCE};${${STARLIGHT_NAME}_HEADERS}")

if (NOT ${HEADLESS})
    set(${STARLIGHT_NAME}_INCLUDE_DIRS
        "include/"
        "include/virtual/"
        "include/internals"
        "include/virtual/ModulePlug/"
        "include/wrappers/"
        "include/wrappers/graphics/"
        "include/structs/"
        "include/enums/"
        "include/templates/"
        "include/managers/"
        "include/common/"
        "include/common/entities/"
        "include/common/materials/"
        "include/common/helpers/"
        "include/common/objects/"
        "include/common/buffers/"
        "include/common/textures"
        "include/common/controllers/"
        "include/builders/"
        "include/controllers/"
        "include/modules/"
        "include/systems/"
        "include/structs/"
        "include/renderers/" 
        "include/job/"
        "include/core/"
        ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvreflect
        ${glm_SOURCE_DIR}
        ${Stb_INCLUDE_DIR}
        "extern/glfw/include/"
    )

    target_link_libraries(${STARLIGHT_NAME}
        PUBLIC
            Starlight::StarlightCommon
            glslang
            SPIRV-Tools-opt
            SPIRV-Tools
            shaderc_util
            shaderc
            tinyobjloader::tinyobjloader
            spirv-reflect-static
            GPUOpen::VulkanMemoryAllocator
            Vulkan::Vulkan
            nlohmann_json::nlohmann_json
            Boost::log
            Boost::log_setup
            Boost::thread
            Boost::container
            Boost::filesystem
            KTX::ktx
            absl::hash
            glfw
        )
else()
    set(${STARLIGHT_NAME}_INCLUDE_DIRS
        "include/"
        "include/virtual/"
        "include/internals"
        "include/virtual/ModulePlug/"
        "include/wrappers/"
        "include/wrappers/graphics/"
        "include/structs/"
        "include/enums/"
        "include/templates/"
        "include/managers/"
        "include/common/"
        "include/common/entities/"
        "include/common/materials/"
        "include/common/helpers/"
        "include/common/objects/"
        "include/common/buffers/"
        "include/common/textures"
        "include/common/controllers/"
        "include/builders/"
        "include/controllers/"
        "include/modules/"
        "include/systems/"
        "include/structs/"
        "include/renderers/" 
        ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvreflect
        ${glm_SOURCE_DIR}
        ${Stb_INCLUDE_DIR}
    )

    target_link_libraries(${STARLIGHT_NAME}
        PUBLIC
            Vulkan::Vulkan
            shaderc
            tinyobjloader::tinyobjloader
            spirv-reflect-static
            GPUOpen::VulkanMemoryAllocator
            nlohmann_json::nlohmann_json
            Boost::log
            Boost::log_setup
            Boost::thread
            Boost::container
            Boost::filesystem
            KTX::ktx
            absl::hash
        )
endif()

set(${STARLIGHT_NAME}_TARGET_INCLUDE_DIRS ${${STARLIGHT_NAME}_INCLUDE_DIRS} PARENT_SCOPE)

target_include_directories(${STARLIGHT_NAME}
    PUBLIC
        ${${STARLIGHT_NAME}_INCLUDE_DIRS}
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

if (NOT ${HEADLESS})
    target_compile_definitions(${STARLIGHT_NAME} PRIVATE ENABLE_PRESENTATION)
endif()