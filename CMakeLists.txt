cmake_minimum_required (VERSION 3.8)
find_package(Git)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)

project("Starlight")

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
    "src/main.cpp"
    "src/Renderer.cpp"
    "src/BasicDevice.cpp" 
    "src/BasicWindow.cpp" 
    "src/ManagerShader.cpp"  
    "src/BasicRenderer.cpp"
    "src/virtual/StarRenderer.cpp" 
    "src/virtual/StarEngine.cpp" 
    "src/virtual/StarManager.cpp"
    "src/virtual/StarWindow.cpp" 
    "src/virtual/StarApp.cpp"
    "src/virtual/StarDevice.cpp"
)

set(${PROJECT_NAME}_HEADERS
    "include/BasicRenderer.hpp"
    "include/BasicDevice.hpp" 
    "include/BasicWindow.hpp" 
    "include/virtual/StarManager.hpp"
    "include/virtual/StarRenderer.hpp" 
    "include/virtual/StarWindow.hpp" 
    "include/virtual/StarEngine.hpp" 
    "include/virtual/StarApp.hpp"
    "include/virtual/StarDevice.hpp"
)

add_executable(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCE};${${PROJECT_NAME}_HEADERS}")

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${Vulkan_LIBRARIES}
        shaderc
        tinyobjloader
        glfw
        spirv-reflect-static
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${STB_DIRECTORY}
        "include/"
        "include/virtual/"
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Systems
        ${CMAKE_CURRENT_SOURCE_DIR}/external/shaderc/libshaderc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/spirvreflect
        #${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjectloader
        "extern/tinyobjloader"
        ${GLM_HEADER_DIR}
)