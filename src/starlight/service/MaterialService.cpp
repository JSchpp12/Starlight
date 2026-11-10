#include "starlight/service/MaterialService.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/Image.hpp"
#include "device/managers/Material.hpp"
#include "virtual/StarMaterial.hpp"

#include <cassert>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace star::service
{
MaterialService::MaterialService(MaterialService &&other) noexcept
    : m_listenForSet(*this), m_listenForMake(*this), m_listenForFrame(*this), m_context(other.m_context),
      m_device(other.m_device), m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus),
      m_evtBus(other.m_evtBus), m_retires(std::move(other.m_retires)), m_listenersInitialized(false)
{
    assert(!other.m_listenersInitialized && "MaterialService must be moved before init");

    other.m_context = nullptr;
    other.m_device = nullptr;
    other.m_graphicsManagers = nullptr;
    other.m_cmdBus = nullptr;
    other.m_evtBus = nullptr;
}

void MaterialService::setInitParameters(InitParameters &params)
{
    m_context = &params.context;
    m_device = &params.device;
    m_graphicsManagers = &params.graphicsManagers;
    m_cmdBus = &params.commandBus;
    m_evtBus = &params.eventBus;
}

void MaterialService::init()
{
    assert(m_cmdBus != nullptr && m_evtBus != nullptr && "MaterialService not initialized");

    if (m_listenersInitialized)
        return;

    initListeners(*m_cmdBus);
    initListeners(*m_evtBus);
    m_listenersInitialized = true;
}

void MaterialService::shutdown()
{
    if (m_listenersInitialized && m_cmdBus != nullptr && m_evtBus != nullptr)
    {
        cleanupListeners(*m_cmdBus);
        cleanupListeners(*m_evtBus);
        m_listenersInitialized = false;
    }

    // One-shot setups are owned by the pass requests and drained by DeviceContext's pending-work cleanup.
    m_retires.clear();
}

void MaterialService::onMakeMaterial(command::material::MakeMaterial &cmd)
{
    assert(m_graphicsManagers != nullptr);
    assert(m_context != nullptr && "MaterialService requires a DeviceContext to place one-shot work");

    // Prepare first: the definition is owned by the pass request, never by the material.
    setup::RenderResourceSetup::ResourcePrepDefinition definition;
    if (cmd.prep.prepare)
        cmd.prep.prepare(*m_context, definition);

    if (cmd.commandBuffer.isInitialized())
    {
        auto &request = m_context->getManagerCommandBuffer().m_manager.get(cmd.commandBuffer);

        for (const auto &wait : cmd.waits)
        {
            if (wait.semaphore.vkSemaphore == VK_NULL_HANDLE)
                continue;

            request.oneTimeWaitSemaphoreInfo.insert(wait.key, wait.semaphore.vkSemaphore, wait.stages,
                                                    wait.semaphore.signalValue);
        }

        request.add(std::move(definition));
    }
    else
    {
        definition.cleanupRender(*m_context);
    }

    const Handle handle =
        m_graphicsManagers->materialManager.submit(core::device::manager::MaterialRequest{std::move(cmd.material)});
    cmd.getReply().set(handle);
}

void MaterialService::onSetMaterialResource(command::material::SetMaterialResource &cmd)
{
    assert(m_graphicsManagers != nullptr);
    assert(m_context != nullptr && "MaterialService requires a DeviceContext to place one-shot work");

    auto &materials = m_graphicsManagers->materialManager;
    auto *material = materials.getMaterial(cmd.material);
    if (material == nullptr)
        return;

    material->setResourceHandle(cmd.setIndex, cmd.bindingIndex, std::move(cmd.newHandle), cmd.frameInFlightIndex);

    // The swap command carries the owning render phase's command buffer; its one-time wait info is consumed by the
    // submission that records that phase.
    if (m_context != nullptr && cmd.recordingCommandBuffer.isInitialized() && !cmd.waits.empty())
    {
        auto &request = m_context->getManagerCommandBuffer().m_manager.get(cmd.recordingCommandBuffer);
        for (const auto &wait : cmd.waits)
        {
            if (wait.semaphore.vkSemaphore == VK_NULL_HANDLE)
                continue;

            request.oneTimeWaitSemaphoreInfo.insert(wait.key, wait.semaphore.vkSemaphore, wait.stages,
                                                    wait.semaphore.signalValue);
        }
    }

    // Prepare and hand the work to the owning pass's request; clean it immediately if no pass was reported.
    if (m_context != nullptr)
    {
        setup::RenderResourceSetup::ResourcePrepDefinition definition;
        if (cmd.prep.prepare)
            cmd.prep.prepare(*m_context, definition);

        if (cmd.recordingCommandBuffer.isInitialized())
        {
            m_context->getManagerCommandBuffer().m_manager.get(cmd.recordingCommandBuffer).add(std::move(definition));
        }
        else
        {
            definition.cleanupRender(*m_context);
        }
    }
}

void MaterialService::onStartOfNextFrame(const event::StartOfNextFrame &, bool &)
{
    drainRetires();
}

void MaterialService::drainRetires()
{
    if (m_device == nullptr || m_graphicsManagers == nullptr)
        return;

    const vk::Device device = m_device->getVulkanDevice();
    auto &imageManager = m_graphicsManagers->imageManager;
    auto &bufferManager = m_graphicsManagers->bufferManager;

    for (auto it = m_retires.begin(); it != m_retires.end();)
    {
        const auto &gate = it->gate;
        if (gate.semaphore == VK_NULL_HANDLE || gate.signalValue == 0 ||
            device.getSemaphoreCounterValue(gate.semaphore) < gate.signalValue)
        {
            ++it;
            continue;
        }

        if (it->isImage)
            imageManager.deleteRequest(*m_device, it->handle);
        else
            bufferManager.deleteRequest(*m_device, it->handle);

        it = m_retires.erase(it);
    }
}

void MaterialService::initListeners(core::CommandBus &bus)
{
    m_listenForSet.init(bus);
    m_listenForMake.init(bus);
}

void MaterialService::initListeners(common::EventBus &bus)
{
    m_listenForFrame.init(bus);
}

void MaterialService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForSet.cleanup(bus);
    m_listenForMake.cleanup(bus);
}

void MaterialService::cleanupListeners(common::EventBus &bus)
{
    m_listenForFrame.cleanup(bus);
}
} // namespace star::service
