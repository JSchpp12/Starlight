#pragma once

#include <starlight/wrappers/graphics/StarSemaphore.hpp>

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

namespace star::command::material
{
/// @brief One-time transfer wait a material's recording pass must observe before sampling/using a resource.
struct MaterialTransferWait
{
    Handle key;
    star::StarSemaphore semaphore;
    vk::PipelineStageFlags stages =
        vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
};
} // namespace star::command::material
