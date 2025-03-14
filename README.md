# Starlight Engine

A rendering framework used to create starlight applications

## Required Dependencies

### Included as submodules
In most starlight applications, the templates utilize vcpkg to install the required dependencies.
The following packages are required:

1. GLM
2. GLFW
3. Vulkan Memory Allocator
4. Stb
5. Spirv-Tools
6. Spirv-Headers
7. glslang
8. shaderc

### External Dependencies
The following dependencies are requried to be provided at build time. Typical starlight applications will include these through VCPKG.

1.Vulkan Memory Allocator
2. nlohmann_json
3. Boost
4. glm
5. tinyobjloader

## Other Requirenments

1. CMAKE 3.27.9
2. Vulkan SDK 1.3.290
