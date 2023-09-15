cmake_minimum_required (VERSION 3.8)
find_package(Git)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)

project("starlight")
#TODO: add ability to rely on installed libraries 
#THIS IS NEEDED FOR NOW...
option(BUILD_LIBRARIES "Build external libraries" ON)

option(UPDATE_SUBMODULES "If building external libraries, check for updates and existence of needed submodules" ON)
option(PULL_SUBMODULE "Check and pull updates for git submodules if needed" ON)

if (GIT_FOUND)
	if(GIT_SUBMODULE AND PULL_SUBMODULE)
		message(STATUS "Submodule update")
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
		endif()
	endif()

	if(PREP_EXTERNAL_PACKAGES)
		message("Preparing requirenments for external dependencies")
		message("Preparing dependencies for spirvtools")
		find_library(EFFCEE_LIB NAME effcee HINTS ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvtools/external)
		if (NOT EFFCEE_LIB AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/spirvtools/external/effcee")
			message("effcee is required for spirvtools and was not found, fetching...")
			execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/google/effcee.git
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external/spirvtools/external"
				RESULT_VARIABLE GIT_EFFCEE_RESULT
			)
			if (NOT GIT_EFFCEE_RESULT EQUAL "0")
				message(FATAL_ERROR "effcee is needed for building spirvtools, fetch into required dir")
			endif()
		endif()

		find_library(RE2_LIB NAME re2 HINTS ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvtools/external)
		if (NOT RE2_LIB AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/spirvtools/external/re2")
			message("re2 is required for spirvtools and was not found, fetching...")
			execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/google/re2.git
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/external/spirvtools/external"
				RESULT_VARIABLE GIT_RE2_RESULT
			)
			if (NOT GIT_RE2_RESULT EQUAL "0")
				message(FATAL_ERROR "re2 is needed for building spirvtools, fetch into required dir")
			endif()
		endif()
	endif()
	message("Prep Complete")
endif()

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

set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
add_subdirectory("extern/shaderc")

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
add_subdirectory("extern/spirv_reflect")

add_subdirectory("extern/tinyobjloader")

set(${PROJECT_NAME}_SOURCE 
    "src/Renderer.cpp"
    "src/BasicDevice.cpp" 
    "src/BasicWindow.cpp" 
    "src/BasicRenderer.cpp"
    "src/common/Compiler.cpp"
    "src/virtual/StarRenderer.cpp" 
    "src/StarEngine.cpp" 
    "src/virtual/StarManager.cpp"
    "src/virtual/StarWindow.cpp" 
    "src/virtual/StarDevice.cpp"
    "src/wrappers/graphics/StarDescriptors.cpp"  
    
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
)

set(${PROJECT_NAME}_HEADERS
    "include/StarEngine.hpp"
    "include/BasicDevice.hpp" 
    "include/BasicWindow.hpp" 
    "include/BasicRenderer.hpp"
    "include/common/Compiler.hpp"
    "include/virtual/StarManager.hpp"
    "include/virtual/StarRenderer.hpp" 
    "include/virtual/StarWindow.hpp" 
    "include/virtual/StarDevice.hpp"
    "include/wrappers/graphics/StarDescriptors.hpp"
    "include/managers/ShaderManager.hpp"
    "include/managers/TextureManager.hpp"
    "include/managers/LightManager.hpp"
    "include/managers/ObjectManager.hpp"
    "include/managers/MapManager.hpp"
    "include/builders/SceneBuilder.hpp"
    "include/controllers/CameraController.hpp"
    "include/controllers/OptionsController.hpp"
    "include/modules/InteractionSystem.hpp"
    #header only
    "include/entities/Light.hpp"
    "include/virtual/StarEntity.hpp"
    "include/virtual/Shader.hpp"
    "include/enums/Enums.hpp"
    "include/structs/Handle.hpp"
    "include/structs/LightBufferObject.hpp"
    "include/structs/Material.hpp"
    "include/structs/Mesh.hpp"
    "include/structs/Triangle.hpp"
    "include/entities/GameObject.hpp"
    "include/templates/StarMemoryManager.hpp"
    "include/templates/StarApplication.hpp"
    "include/templates/StarResourceContainer.hpp"
    "include/templates/FileResourceManager.hpp"
    "include/templates/FileResourceContainer.hpp"
    "include/common/ConfigFile.hpp"
    "include/common/Camera.hpp"
    "include/common/Interactivity.hpp"
    "include/common/Texture.hpp"
    "include/common/RenderOptions.hpp"
    "include/common/helpers/DebugHelpers.hpp"
    "include/common/helpers/FileHelpers.hpp"
    "include/common/helpers/Time.hpp"

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