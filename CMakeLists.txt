cmake_minimum_required (VERSION 3.8)
find_package(Git)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)

project("starlight")

find_package(Vulkan REQUIRED)

# add all necessary dependencies
include(GNUInstallDirs)

set(STB_DIRECTORY ${PROJECT_SOURCE_DIR}/extern/stb)
if (NOT IS_DIRECTORY ${STB_DIRECTORY})
	message(FATAL_ERROR "Stb is needed for compilation. Pull copy into external directory.")
endif()

add_subdirectory("extern/googletest")

set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_headers")

set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_tools")

set(GLFW_STANDALONE OFF CACHE BOOL "" FORCE)
add_subdirectory("extern/glfw")
add_subdirectory("extern/glm")

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

set(${PROJECT_NAME}_SOURCE 
    "src/Renderer.cpp"
    "src/BasicDevice.cpp" 
    "src/BasicWindow.cpp" 
    "src/BasicRenderer.cpp"
    "src/common/Compiler.cpp"
    "src/common/KeyStates.cpp"
    "src/virtual/StarRenderer.cpp" 
    "src/StarEngine.cpp" 
    "src/entities/GameObject.cpp"
    "src/virtual/StarManager.cpp"
    "src/virtual/StarWindow.cpp" 
    "src/virtual/StarDevice.cpp"
    "src/wrappers/graphics/StarDescriptors.cpp"  
    "src/wrappers/graphics/StarBuffer.cpp"  
    "src/wrappers/graphics/StarPipeline.cpp"  
    "src/wrappers/graphics/StarTexture.cpp"  
    "src/managers/ShaderManager.cpp" 
    "src/managers/TextureManager.cpp"
    "src/managers/LightManager.cpp"
    "src/managers/ObjectManager.cpp"
    "src/builders/SceneBuilder.cpp"
     "src/managers/MaterialManager.cpp" 
     "src/managers/MapManager.cpp" 
     "src/controllers/CameraController.cpp" 
     "src/controllers/OptionsController.cpp"
     "src/modules/InteractionSystem.cpp"
     "src/systems/StarSystemRenderObj.cpp" 
     "src/wrappers/graphics/StarPipeline.cpp" 
     "src/renderers/StarRenderObject.cpp" 
     "src/renderers/StarRenderMesh.cpp"
     "src/systems/StarSystemRenderPointLight.cpp" 
     "src/renderers/StarRenderMaterial.cpp" 
     "src/virtual/StarShader.cpp" "src/common/Interactivity.cpp")


set(${PROJECT_NAME}_HEADERS
    "include/StarEngine.hpp"
    "include/BasicDevice.hpp" 
    "include/BasicWindow.hpp" 
    "include/BasicRenderer.hpp"
    "include/common/Compiler.hpp"
    "include/common/Interactivity.hpp"
    "include/common/KeyStates.hpp"
    "include/virtual/StarManager.hpp"
    "include/virtual/StarRenderer.hpp" 
    "include/virtual/StarWindow.hpp" 
    "include/virtual/StarDevice.hpp"
    "include/wrappers/graphics/StarDescriptors.hpp"
    "include/wrappers/graphics/StarBuffer.hpp"
    "include/wrappers/graphics/StarTexture.hpp"
    "include/wrappers/graphics/StarPipeline.hpp"
    "include/managers/ShaderManager.hpp"
    "include/managers/TextureManager.hpp"
    "include/managers/LightManager.hpp"
    "include/managers/ObjectManager.hpp"
    "include/managers/MapManager.hpp"
    "include/builders/SceneBuilder.hpp"
    "include/controllers/CameraController.hpp"
    "include/controllers/OptionsController.hpp"
    "include/modules/InteractionSystem.hpp"
    "include/systems/StarSystemRenderObj.hpp"
    "include/renderers/StarRenderObject.hpp"
    "include/renderers/StarRenderMesh.hpp"
    "include/renderers/StarRenderMaterial.hpp"
    #header only
    "include/entities/Light.hpp"
    "include/virtual/StarEntity.hpp"
    "include/virtual/StarShader.hpp"
    "include/enums/Enums.hpp"
    "include/enums/Key.hpp"
    "include/structs/Handle.hpp"
    "include/structs/LightBufferObject.hpp"
    "include/structs/Material.hpp"
    "include/structs/Mesh.hpp"
    "include/structs/Triangle.hpp"
    "include/structs/Vertex.hpp"
    "include/entities/GameObject.hpp"
    "include/templates/StarMemoryManager.hpp"
    "include/templates/StarApplication.hpp"
    "include/templates/StarResourceContainer.hpp"
    "include/templates/FileResourceManager.hpp"
    "include/templates/FileResourceContainer.hpp"
    "include/common/ConfigFile.hpp"
    "include/common/Camera.hpp"
    "include/common/Texture.hpp"
    "include/common/RenderOptions.hpp"
    "include/common/helpers/DebugHelpers.hpp"
    "include/common/helpers/FileHelpers.hpp"
    "include/common/helpers/Time.hpp"
    "include/common/VulkanVertex.hpp"
    "include/common/Scene.hpp"
)

add_library(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCE};${${PROJECT_NAME}_HEADERS}")

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${Vulkan_LIBRARIES}
        shaderc
        tinyobjloader
        glfw
        spirv-reflect-static
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "include/"
        "include/virtual/"
        "include/wrappers/"
        "include/wrappers/graphics/"
        "include/structs/"
        "include/enums/"
        "include/entities/"
        "include/templates/"
        "include/managers/"
        "include/common/"
        "include/common/helpers/"
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