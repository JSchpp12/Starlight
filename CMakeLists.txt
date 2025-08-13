cmake_minimum_required (VERSION 3.26)

SET(STARLIGHT_NAME "starlight")
project(${STARLIGHT_NAME})

include(CheckIncludeFile)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
elseif(WIN32)
    set(CMAKE_FIND_LIBRARY_PREFIXES "")
endif()
enable_language(C CXX)

set(VULKAN_VERSION "1.4.313.0")
find_package(Vulkan ${VULKAN_VERSION})
if (NOT Vulkan_FOUND)
    message("Vulkan not found. Downloading...")
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/DownloadVulkan.cmake)
endif()

find_package(Stb REQUIRED)
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
find_package(Boost REQUIRED COMPONENTS container thread filesystem)
find_package(Ktx CONFIG REQUIRED)

# add all necessary dependencies
include(GNUInstallDirs)

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
    "src/StarEngine.cpp" 
    "src/Renderer.cpp"
    "src/SceneRenderer.cpp"
    "src/SwapChainRenderer.cpp"
    "src/virtual/StarObject.cpp"
    "src/virtual/ModulePlug/RenderResourceModifier.cpp"
    "src/virtual/ModulePlug/CommandBufferModifier.cpp"
    "src/virtual/ModulePlug/DescriptorModifier.cpp"
    "src/virtual/ModulePlug/ManagerPlug.cpp"
    "src/virtual/StarMesh.cpp"
    "src/virtual/StarRenderer.cpp" 
    "src/virtual/StarManager.cpp"
    "src/virtual/StarWindow.cpp" 
    "src/virtual/StarDevice.cpp"
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
    "src/wrappers/ScreenshotCommandBuffer.cpp"
    "src/wrappers/graphics/StarTextures/Texture.cpp"
    "src/wrappers/graphics/StarTextures/Resources.cpp"
    "src/wrappers/graphics/StarTextures/AllocatedResources.cpp"
    "src/wrappers/graphics/StarBuffers/Buffer.cpp"
    "src/wrappers/graphics/StarBuffers/Resources.cpp"
    "src/managers/ShaderManager.cpp" 
    "src/managers/ManagerRenderResource.cpp"
    "src/managers/LightManager.cpp"
    "src/managers/MapManager.cpp" 
    "src/managers/ManagerDescriptorPool.cpp"
    "src/managers/ManagerCommandBuffer.cpp"
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
    "src/common/Handle.cpp"
    "src/common/buffers/TransferRequest_GlobalInfo.cpp"
    "src/common/buffers/TransferRequest_InstanceModelInfo.cpp"
    "src/common/buffers/TransferRequest_InstanceNormalInfo.cpp"
    "src/common/buffers/TransferRequest_LightInfo.cpp"
    "src/common/buffers/TransferRequest_IndiciesInfo.cpp"
    "src/common/buffers/TransferRequest_VertInfo.cpp"
    "src/common/textures/TransferRequest_TextureFile.cpp"
    "src/common/textures/TransferRequest_CompressedTextureFile.cpp"
    "src/common/textures/SharedCompressedTexture.cpp"
    "src/common/controllers/ManagerController_RenderResource_TextureFile.cpp"
    "src/common/controllers/ManagerController_RenderResource_GlobalInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_InstanceModelInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_InstanceNormalInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_LightInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_IndicesInfo.cpp"
    "src/common/controllers/ManagerController_RenderResource_VertInfo.cpp"
    "src/common/materials/TextureMaterial.cpp"
    "src/common/helpers/GeometryHelpers.cpp"
    "src/common/helpers/FileHelpers.cpp"
    "src/internals/CommandBufferContainer.cpp"
    "src/internals/TransferWorker.cpp"
    "src/internals/ManagerStorageContainer.cpp"
    "src/job/Worker.cpp"
    "src/job/Task.cpp"
    "src/job/ImageWriteTask.cpp"
    "src/job/TaskManager.cpp"
    "src/job/TaskFactory.cpp"
    "src/job/ImageWriterWorker.cpp"
    "src/core/DeviceContext.cpp"
    "src/core/DeviceManager.cpp"
    "src/core/RenderingInstance.cpp"
)

set(${STARLIGHT_NAME}_TARGET_SOURCE ${${STARLIGHT_NAME}_SOURCE} PARENT_SCOPE)

set(${STARLIGHT_NAME}_HEADERS
    "include/StarEngine.hpp"
    "include/SceneRenderer.hpp"
    "include/SwapChainRenderer.hpp"
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
    "include/common/textures/TransferRequest_TextureFile.hpp"
    "include/common/textures/TransferRequest_CompressedTextureFile.hpp"
    "include/common/textures/SharedCompressedTexture.hpp"
    "include/common/controllers/ManagerController_RenderResource_TextureFile.hpp"
    "include/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_InstanceModelInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_InstanceNormalInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_LightInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_IndicesInfo.hpp"
    "include/common/controllers/ManagerController_RenderResource_VertInfo.hpp"
    "include/virtual/ModulePlug/RenderResourceModifier.hpp"
    "include/virtual/ModulePlug/CommandBufferModifier.hpp"
    "include/virtual/ModulePlug/DescriptorModifier.hpp"
    "include/virtual/ModulePlug/ManagerPlug.hpp"
    "include/virtual/StarManager.hpp"
    "include/virtual/StarRenderer.hpp" 
    "include/virtual/StarWindow.hpp" 
    "include/virtual/StarDevice.hpp"
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
    "include/wrappers/ScreenshotCommandBuffer.hpp"
    "include/wrappers/graphics/StarShaderInfo.hpp"
    "include/wrappers/graphics/StarQueueFamily.hpp"
    "include/wrappers/graphics/StarQueue.hpp"
    "include/wrappers/graphics/StarCommandPool.hpp"
    "include/wrappers/Allocator.hpp"
    "include/managers/ShaderManager.hpp"
    "include/managers/LightManager.hpp"
    "include/managers/MapManager.hpp"
    "include/managers/ManagerDescriptorPool.hpp"
    "include/managers/ManagerCommandBuffer.hpp"
    "include/managers/ManagerRenderResource.hpp"
    "include/modules/InteractionSystem.hpp"
    "include/modules/RenderResourceSystem.hpp"
    "include/systems/StarRenderGroup.hpp"
    "include/internals/CommandBufferContainer.hpp"
    "include/internals/TransferWorker.hpp"
    "include/internals/ManagerStorageContainer.hpp"
    "include/virtual/StarEntity.hpp"
    "include/wrappers/graphics/StarTextures/Texture.hpp"
    "include/wrappers/graphics/StarTextures/Resources.hpp"
    "include/wrappers/graphics/StarTextures/AllocatedResources.hpp"
    "include/wrappers/graphics/StarBuffers/Buffer.hpp"
    "include/wrappers/graphics/StarBuffers/Resources.hpp"
    "include/job/Worker.hpp"
    "include/job/Task.hpp"
    "include/job/ImageWriteTask.hpp"
    "include/job/TaskManager.hpp"
    "include/job/ImageWriterWorker.hpp"
    "include/job/TaskFactory.hpp"
    "include/core/DeviceContext.hpp"
    "include/core/DeviceManager.hpp"
    "include/core/RenderingInstance.hpp"
    #header only
    "include/common/entities/Light.hpp"
    "include/virtual/StarShader.hpp"
    "include/enums/Enums.hpp"
    "include/common/Handle.hpp"
    "include/common/BufferHandle.hpp"
    "include/structs/LightBufferObject.hpp"
    "include/structs/Vertex.hpp"
    "include/structs/Color.hpp"
    "include/structs/Ray.hpp"
    "include/structs/RenderingTargetInfo.hpp"
    "include/templates/StarMemoryManager.hpp"
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
            Vulkan::Vulkan
            shaderc
            tinyobjloader::tinyobjloader
            spirv-reflect-static
            GPUOpen::VulkanMemoryAllocator
            nlohmann_json::nlohmann_json
            Boost::thread
            Boost::container
            Boost::filesystem
            KTX::ktx
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
            Boost::thread
            Boost::container
            Boost::filesystem
            KTX::ktx
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