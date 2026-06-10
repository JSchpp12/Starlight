#pragma once

#include "starlight/wrappers/graphics/StarSemaphore.hpp"
#include "starlight/wrappers/graphics/StarTextures/Texture.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <string_view>
#include <optional>

namespace star::service::detail::screen_capture::common
{
constexpr std::string_view ScreenCaptureServiceCalleeTypeName = "star::service::screen_capture::callee";

enum class RoutePath
{
    CopyImageToBufferDirect,
    BlitImageToRGBAThenCopy,
    none
};

struct GatheredSemaphoreInfo
{
    star::Handle record; 
    uint64_t currentSignalValue;
    uint64_t valueToSignal; 
    vk::Semaphore *semaphore = nullptr;
};

struct InUseResourceInformation
{
    star::StarTextures::Texture targetTexture;
    RoutePath path;
    GatheredSemaphoreInfo timelineSemaphoreForCopyDone;
    vk::Buffer buffer = VK_NULL_HANDLE;
    vk::Image targetBlitImage = VK_NULL_HANDLE;
    vk::Filter blitFilter;
    vk::Semaphore *binarySemaphoreForCopyDone = nullptr;
    vk::Semaphore *semaphoreForBlitDone = nullptr;
    star::StarSemaphore *targetTextureReadySemaphore{nullptr};
    vk::Queue queueToUse = VK_NULL_HANDLE;
};
} // namespace star::service::detail::screen_capture::common