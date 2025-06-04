cmake_minimum_required (VERSION 3.27)
find_package(Git)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
if (WIN32)
    set(CMAKE_FIND_LIBRARY_PREFIXES "")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif()

project("starlight")

enable_language(C CXX)

find_package(Vulkan REQUIRED)
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
find_package(glfw3 CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS container thread filesystem)
find_package(ktx CONFIG REQUIRED COMPONENTS vulkan)

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

add_subdirectory("extern/googletest") 

set(${PROJECT_NAME}_SOURCE 
    "src/StarEngine.cpp" 
    "src/Renderer.cpp"
    "src/SceneRenderer.cpp"
    "src/SwapChainRenderer.cpp"
    "src/virtual/StarSystemPipeline.cpp"
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
    "src/wrappers/graphics/StarBuffer.cpp"  
    "src/wrappers/graphics/StarGraphicsPipeline.cpp"  
    "src/wrappers/graphics/StarComputePipeline.cpp"
    "src/wrappers/graphics/StarRenderPass.cpp"
    "src/wrappers/graphics/StarCommandBuffer.cpp"
    "src/wrappers/graphics/StarShaderInfo.cpp"
    "src/wrappers/graphics/SharedFence.cpp"
    "src/wrappers/graphics/StarQueueFamily.cpp"
    "src/wrappers/graphics/StarTexture.cpp"
    "src/wrappers/graphics/StarQueue.cpp"
    "src/wrappers/graphics/StarCommandPool.cpp"
    "src/wrappers/Allocator.cpp"
    "src/wrappers/ScreenshotCommandBuffer.cpp"
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
)

set(${PROJECT_NAME}_HEADERS
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
    "include/virtual/StarSystemPipeline.hpp"
    "include/virtual/StarPipeline.hpp"
    "include/virtual/TransferRequest_Texture.hpp"
    "include/virtual/TransferRequest_Buffer.hpp"
    "include/virtual/TransferRequest_Memory.hpp"
    "include/virtual/ManagerController_Controller.hpp"
    "include/virtual/ManagerController_RenderResource_Buffer.hpp"
    "include/virtual/ManagerController_RenderResource_Texture.hpp"
    "include/wrappers/graphics/StarDescriptorBuilders.hpp"
    "include/wrappers/graphics/StarBuffer.hpp"
    "include/wrappers/graphics/StarGraphicsPipeline.hpp"
    "include/wrappers/graphics/StarComputePipeline.hpp"
    "include/wrappers/graphics/StarRenderPass.hpp"
    "include/wrappers/graphics/StarCommandBuffer.hpp"
    "include/wrappers/graphics/SharedFence.hpp"
    "include/wrappers/ScreenshotCommandBuffer.hpp"
    "include/wrappers/graphics/StarShaderInfo.hpp"
    "include/wrappers/graphics/StarQueueFamily.hpp"
    "include/wrappers/graphics/StarTexture.hpp"
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

add_library(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCE};${${PROJECT_NAME}_HEADERS}")

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Vulkan::Vulkan
        shaderc
        tinyobjloader::tinyobjloader
        glfw
        spirv-reflect-static
        GPUOpen::VulkanMemoryAllocator
        nlohmann_json::nlohmann_json
        Boost::thread
        Boost::container
        Boost::filesystem
        KTX::ktx
    )

set(INCLUDE_DIRS
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
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvreflect
        ${glm_SOURCE_DIR}
        ${Stb_INCLUDE_DIR}
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /Zi 
        /MP
        /EHsc
    )
    target_link_options(${PROJECT_NAME} PRIVATE 
        /DEBUG
    )
endif()

find_program(CPPCHECK_EXECUTABLE cppcheck)
if (CPPCHECK_EXECUTABLE)
	set(CPPCHECK_FILES "")
    set(CPPCHECK_INCLUDES "")

	foreach(FILE "${${PROJECT_NAME}_SOURCE}")
		file(TO_CMAKE_PATH "${FILE}" UNIX_PATH)
		list(APPEND CPPCHECK_FILES "${UNIX_PATH}")
	endforeach()

    foreach(DIR ${INCLUDE_DIRS})
        list(APPEND CPPCHECK_INCLUDES "-I" "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
    endforeach()

	add_custom_target("${PROJECT_NAME}_cppcheck"
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

	add_dependencies("${PROJECT_NAME}_cppcheck" ${CMAKE_PROJECT_NAME})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    DEBUG_POSTFIX "d"
)
