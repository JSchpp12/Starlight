#pragma once

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
    uint64_t valueToSignal; 
    std::optional<uint64_t> *signaledValue = nullptr;
    vk::Semaphore *semaphore = nullptr;
};

struct InUseResourceInformation
{
    vk::Extent3D targetImageExtent;
    RoutePath path;
    GatheredSemaphoreInfo timelineSemaphoreForCopyDone;
    vk::Image targetImage = VK_NULL_HANDLE;
    vk::Buffer buffer = VK_NULL_HANDLE;
    vk::ImageLayout targetImageLayout;
    vk::Image targetBlitImage = VK_NULL_HANDLE;
    vk::Filter blitFilter;
    vk::Semaphore *targetTextureReadySemaphore = nullptr;
    vk::Semaphore *binarySemaphoreForCopyDone = nullptr;
    vk::Semaphore *semaphoreForBlitDone = nullptr;
    vk::Queue queueToUse = VK_NULL_HANDLE;
};
} // namespace star::service::detail::screen_capture::common