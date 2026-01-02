#pragma once

#include <string_view>
#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture::common
{
constexpr std::string_view ScreenCaptureServiceCalleeTypeName = "star::service::screen_capture::callee";

enum class RoutePath
{
    CopyImageToBufferDirect,
    BlitImageToRGBAThenCopy,
    none
};

struct InUseResourceInformation
{
    vk::Extent3D targetImageExtent;
    uint64_t numTimesFrameProcessed;
    RoutePath path;
    vk::Image targetImage = VK_NULL_HANDLE;
    vk::Buffer buffer = VK_NULL_HANDLE;
    vk::ImageLayout targetImageLayout;
    vk::Image targetBlitImage = VK_NULL_HANDLE;
    vk::Filter blitFilter;
    vk::Semaphore *targetTextureReadySemaphore = nullptr;
    vk::Semaphore *timelineSemaphoreForCopyDone = nullptr;
    vk::Semaphore *semaphoreForBlitDone = nullptr;
    vk::Queue queueToUse = VK_NULL_HANDLE;
};
} // namespace star::service::detail::screen_capture::common