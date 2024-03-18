cmake_minimum_required (VERSION 3.8)
find_package(Git)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)

project("starlight")

find_package(Vulkan REQUIRED)

# add all necessary dependencies
include(GNUInstallDirs)

set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_headers")

set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_tools")

find_package("glm" REQUIRED)
find_package("glfw3" REQUIRED)

set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
add_subdirectory("extern/glslang")

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_reflect")

set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT OFF CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
add_subdirectory("extern/shaderc")

add_subdirectory("extern/tinyobjloader")

find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_subdirectory("extern/googletest") 

set(${PROJECT_NAME}_SOURCE 
    "src/virtual/StarObject.cpp"
    "src/Renderer.cpp"
    "src/BasicWindow.cpp" 
    "src/SwapChainRenderer.cpp"
    "src/common/Compiler.cpp"
    "src/common/Interactivity.cpp" 
    "src/common/KeyStates.cpp"
    "src/common/RuntimeUpdateTexture.cpp"
    "src/virtual/ModulePlug/RenderResourceModifier.cpp"
    "src/virtual/ModulePlug/RenderResourceModifierGeometry.cpp"
    "src/virtual/StarMesh.cpp"
    "src/virtual/StarRenderer.cpp" 
    "src/StarEngine.cpp" 
    "src/virtual/StarManager.cpp"
    "src/virtual/StarWindow.cpp" 
    "src/virtual/StarDevice.cpp"
    "src/virtual/StarScene.cpp"
    "src/wrappers/graphics/StarDescriptors.cpp"  
    "src/wrappers/graphics/StarBuffer.cpp"  
    "src/wrappers/graphics/StarGraphicsPipeline.cpp"  
    "src/wrappers/graphics/StarComputePipeline.cpp"
    "src/wrappers/graphics/StarTexture.cpp"  
    "src/wrappers/graphics/StarRenderPass.cpp"
    "src/wrappers/graphics/StarCommandBuffer.cpp"
    "src/managers/ShaderManager.cpp" 
    "src/wrappers/Allocator.cpp"
    "src/managers/ManagerBuffer.cpp"
    "src/managers/TextureManager.cpp"
    "src/managers/LightManager.cpp"
    "src/managers/ObjectManager.cpp"
    "src/managers/MapManager.cpp" 
    "src/managers/ManagerDescriptorPool.cpp"
    "src/templates/StarApplication.cpp"
    "src/modules/InteractionSystem.cpp"
    "src/modules/RenderResourceSystem.cpp"
    "src/modules/RenderMemorySystem.cpp"
    "src/systems/StarRenderGroup.cpp" 
    "src/systems/StarSystemRenderPointLight.cpp" 
    "src/virtual/StarShader.cpp" 
    "src/virtual/StarMaterial.cpp" 
    "src/virtual/StarCamera.cpp"
    "src/virtual/StarPipeline.cpp"
    "src/common/materials/BumpMaterial.cpp"
    "src/common/materials/VertColorMaterial.cpp"
    "src/common/materials/HeightDisplacementMaterial.cpp"
    "src/common/objects/BasicObject.cpp"
    "src/common/objects/Square.cpp"
    "src/common/objects/Grid.cpp"
    "src/common/objects/StarObjectInstance.cpp"
    "src/common/ConfigFile.cpp"
    "src/common/BasicCamera.cpp"
     "src/common/Texture.cpp"
    "src/virtual/StarSystemPipeline.cpp"
    "src/common/materials/TextureMaterial.cpp"
    "src/common/helpers/GeometryHelpers.cpp"
)


set(${PROJECT_NAME}_HEADERS
    "include/StarEngine.hpp"
    "include/BasicWindow.hpp" 
    "include/SwapChainRenderer.hpp"
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
    "include/virtual/ModulePlug/RenderResourceModifier.hpp"
    "include/virtual/ModulePlug/RenderResourceModifierGeometry.hpp"
    "include/virtual/StarManager.hpp"
    "include/virtual/StarRenderer.hpp" 
    "include/virtual/StarWindow.hpp" 
    "include/virtual/StarDevice.hpp"
    "include/virtual/StarObject.hpp"
    "include/virtual/StarScene.hpp"
    "include/virtual/StarMesh.hpp"
    "include/virtual/StarCamera.hpp"
    "include/common/StarMaterialMesh.hpp"
    "include/wrappers/graphics/StarDescriptors.hpp"
    "include/wrappers/graphics/StarBuffer.hpp"
    "include/wrappers/graphics/StarTexture.hpp"
    "include/wrappers/graphics/StarGraphicsPipeline.hpp"
    "include/wrappers/graphics/StarComputePipeline.hpp"
    "include/wrappers/graphics/StarRenderPass.hpp"
    "include/wrappers/graphics/StarCommandBuffer.hpp"
    "include/wrappers/Allocator.hpp"
    "include/managers/ShaderManager.hpp"
    "include/managers/TextureManager.hpp"
    "include/managers/LightManager.hpp"
    "include/managers/ObjectManager.hpp"
    "include/managers/MapManager.hpp"
    "include/managers/ManagerDescriptorPool.hpp"
    "include/modules/InteractionSystem.hpp"
    "include/modules/RenderResourceSystem.hpp"
    "include/modules/RenderMemorySystem.hpp"
    "include/systems/StarRenderGroup.hpp"
    "include/virtual/StarMaterial.hpp"
    "include/virtual/StarSystemPipeline.hpp"
    "include/virtual/StarPipeline.hpp"
    #header only
    "include/common/entities/Light.hpp"
    "include/virtual/StarEntity.hpp"
    "include/virtual/StarShader.hpp"
    "include/enums/Enums.hpp"
    "include/enums/Key.hpp"
    "include/common/Handle.hpp"
    "include/common/BufferHandle.hpp"
    "include/structs/LightBufferObject.hpp"
    "include/structs/Vertex.hpp"
    "include/structs/Color.hpp"
    "include/structs/Ray.hpp"
    "include/templates/StarMemoryManager.hpp"
    "include/templates/StarApplication.hpp"
    "include/templates/StarResourceContainer.hpp"
    "include/templates/FileResourceManager.hpp"
    "include/templates/FileResourceContainer.hpp"
    "include/common/ConfigFile.hpp"
    "include/common/Texture.hpp"
    "include/common/helpers/DebugHelpers.hpp"
    "include/common/helpers/FileHelpers.hpp"
    "include/common/helpers/Time.hpp"
    "include/common/helpers/CastHelpers.hpp"
    "include/common/VulkanVertex.hpp"

)

add_library(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCE};${${PROJECT_NAME}_HEADERS}")

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${Vulkan_LIBRARIES}
        shaderc
        tinyobjloader
        glfw
        spirv-reflect-static
        GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "include/"
        "include/virtual/"
        "include/virtual/ModulePlug"
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
        "include/builders/"
        "include/controllers/"
        "include/modules/"
        "include/systems/"
        "include/renderers/"
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Systems
        ${CMAKE_CURRENT_SOURCE_DIR}/external/shaderc/libshaderc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvreflect
        #${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjectloader
        "extern/tinyobjloader"
        ${glm_SOURCE_DIR}
        ${STB_DIRECTORY}
)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")