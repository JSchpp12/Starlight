cmake_minimum_required (VERSION 3.8)
find_package(Git)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)

project("Starlight")

find_library(TINY_OBJ_LOADER_LIB NAMES tinyobjloader)