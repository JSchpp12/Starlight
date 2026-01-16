#pragma once

#include "starlight/core/device/managers/CommandPool.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/wrappers/graphics/StarCommandBuffer.hpp"
#include "starlight/core/device/managers/ManagerCommandBuffer.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/special_types/SpecialHandleTypes.hpp>

#include <vulkan/vulkan.hpp>

namespace star::core::helper
{
static inline StarCommandBuffer BeginSingleTimeCommands(
    core::device::StarDevice &device, common::EventBus &eventBus, const core::device::manager::CommandPool &poolManager,
    core::device::manager::ManagerCommandBuffer &commandBufferManger, const star::Queue_Type &type)
{
    auto &pool = commandBufferManger.getInUseInfoForType(type)->pool;
    StarCommandBuffer buffer{device.getVulkanDevice(), 1, &pool, type, true, false};

    vk::CommandBufferBeginInfo beginInfo =
        vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    buffer.begin(0, beginInfo);

    return buffer;
}

static inline void EndSingleTimeCommands(star::StarQueue &targetQueue, StarCommandBuffer commandBuffer)
{
    commandBuffer.buffer().end();

    commandBuffer.submit(0, targetQueue.getVulkanQueue());

    commandBuffer.wait();
}

} // namespace star::core::helper