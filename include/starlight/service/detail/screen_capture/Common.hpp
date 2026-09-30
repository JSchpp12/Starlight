#pragma once

#include "starlight/wrappers/graphics/StarSemaphore.hpp"
#include "starlight/wrappers/graphics/StarTextures/Texture.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>
#include <string_view>

namespace star::service::detail::screen_capture::common
{
constexpr std::string_view ScreenCaptureServiceCalleeTypeName = "star::service::screen_capture::callee";

struct GatheredSemaphoreInfo
{
    star::Handle record;
    uint64_t currentSignalValue{0};
    uint64_t valueToSignal{0};
    vk::Semaphore *semaphore{nullptr};
};

struct InUseResourceInformation
{
    star::StarTextures::Texture targetTexture;
    GatheredSemaphoreInfo timelineSemaphoreForCopyDone;
    vk::Buffer buffer{VK_NULL_HANDLE};
    vk::Semaphore *binarySemaphoreForCopyDone{nullptr};
    star::StarSemaphore *targetTextureReadySemaphore{nullptr};
    vk::Queue queueToUse{VK_NULL_HANDLE};
};
} // namespace star::service::detail::screen_capture::common