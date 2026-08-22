#pragma once

#include "starlight/core/device/DeviceContext.hpp"
#include "starlight/core/device/managers/ManagerCommandBuffer.hpp"
#include "starlight/wrappers/graphics/StarCommandBuffer.hpp"

#include <star_common/special_types/SpecialHandleTypes.hpp>

#include <cassert>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace star::core::helper::command_buffer
{

/// @brief Record a one-time command buffer on the queue associated with `type`, submit it, and block until the device
/// has finished executing it.
/// @tparam RecordFn
/// @param device
/// @param commandBufferManager
/// @param type
/// @param record
template <typename RecordFn>
void SingleTimeCommands(core::device::StarDevice &device,
                        core::device::manager::ManagerCommandBuffer &commandBufferManager, const star::Queue_Type type,
                        RecordFn &&record)
{
    const auto *info = commandBufferManager.getInUseInfoForType(type);
    assert(info != nullptr && info->queue != nullptr &&
           "No command pool/queue has been prepared for the requested queue type");

    StarCommandBuffer buffer{device.getVulkanDevice(), 1, &info->pool, type, /*initFences=*/true,
                             /*initSemaphores=*/false};
    buffer.begin(0, vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    auto finish = [&]() {
        buffer.submit(0, info->queue->getVulkanQueue());
        buffer.wait();
    };

    try
    {
        std::forward<RecordFn>(record)(buffer.buffer(0));
        buffer.buffer(0).end();
        finish();
    }
    catch (...)
    {
        // Leave the recording state cleanly even if end() itself is unhappy, then drain whatever was recorded before
        // the failure
        try
        {
            buffer.buffer(0).end();
        }
        catch (...)
        {
        }
        try
        {
            finish();
        }
        catch (...)
        {
        }
        throw;
    }
}

/// @brief Convenience overload that pulls the device and command buffer manager off a DeviceContext. Prefer this at
/// call sites that already hold a DeviceContext&.
/// @tparam RecordFn
/// @param context
/// @param type
/// @param record
template <typename RecordFn>
void SingleTimeCommands(core::device::DeviceContext &context, const star::Queue_Type type, RecordFn &&record)
{
    SingleTimeCommands(context.getDevice(), context.getManagerCommandBuffer().m_manager, type,
                       std::forward<RecordFn>(record));
}
} // namespace star::core::helper::command_buffer
