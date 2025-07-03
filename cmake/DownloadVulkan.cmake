# cmake/DownloadVulkan.cmake
include(FetchContent)

# Detect system and architecture
execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND uname -s OUTPUT_VARIABLE OS OUTPUT_STRIP_TRAILING_WHITESPACE)

# Map architecture and OS
if(${ARCH} STREQUAL "x86_64")
    set(ARCH "x86_64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}")
endif()

if(${OS} STREQUAL "Linux")
    # Detect distro
    execute_process(
        COMMAND bash -c "source /etc/os-release && echo $ID"
        OUTPUT_VARIABLE DISTRO
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(DISTRO MATCHES "ubuntu|debian")
        set(DISTRO "ubuntu")
    elseif(DISTRO MATCHES "rocky|rhel|centos")
        set(DISTRO "centos") # use centos-compatible package
    else()
        message(FATAL_ERROR "Unsupported Linux distro: ${DISTRO}")
    endif()
else()
    message(FATAL_ERROR "Unsupported OS: ${OS}")
endif()

# Set SDK version and URL (e.g., Vulkan SDK 1.3.261.1)

set(VULKAN_SDK_URL "https://sdk.lunarg.com/sdk/download/${VULKAN_VERSION}/linux/vulkan-sdk-${VULKAN_VERSION}-${DISTRO}.tar.gz?Human=true")
set(VULKAN_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../libs/vulkan-sdk")
set(VULKAN_SDK_TAR "${CMAKE_CURRENT_LIST_DIR}/../libs/vulkan-sdk.tar.gz")
set(VULKAN_SDK_EXTRACT_DIR "${CMAKE_CURRENT_LIST_DIR}/../libs/")

if (NOT EXISTS ${VULKAN_SDK_EXTRACT_DIR})
    file(DOWNLOAD
        "${VULKAN_SDK_URL}"
        "${VULKAN_SDK_TAR}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )

    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Vulkan SDK from ${VULKAN_SDK_URL}")
    endif()

    # Extract .tar.gz into VULKAN_SDK_EXTRACT_DIR
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${VULKAN_SDK_TAR}"
        WORKING_DIRECTORY "${VULKAN_SDK_EXTRACT_DIR}"
    )
else()
    message("Vulkan SDK previously downloaded")
endif()

# Use the SDK
file(GLOB SDK_DIR "${VULKAN_SDK_EXTRACT_DIR}/${VULKAN_VERSION}")
list(GET SDK_DIR 0 VULKAN_SDK_ROOT)

set(ENV{VULKAN_SDK} "${VULKAN_SDK_ROOT}/x86_64")
list(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")

find_package(Vulkan ${VULKAN_VERSION} REQUIRED)
