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
    : m_listenForCreate(*this), m_listenForSet(*this), m_listenForFrame(*this), m_context(other.m_context),
      m_device(other.m_device), m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus),
      m_evtBus(other.m_evtBus), m_retires(std::move(other.m_retires)),
      m_orphanedSetups(std::move(other.m_orphanedSetups)), m_listenersInitialized(false)
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

    // The one-shot setups are not recorded yet (CommandOrderService injection is the next step); clean them while the
    // device is alive.
    if (m_context != nullptr)
    {
        for (auto &setup : m_orphanedSetups)
            setup.cleanupRender(*m_context);
    }

    m_orphanedSetups.clear();
    m_retires.clear();
}

void MaterialService::onCreateMaterial(command::material::CreateMaterial &cmd)
{
    assert(m_graphicsManagers != nullptr);

    const Handle handle =
        m_graphicsManagers->materialManager.submit(core::device::manager::MaterialRequest{std::move(cmd.material)});

    // TODO: transfer-wait ownership moves onto the material; nothing consumes CreateMaterial::waits right now.
    cmd.getReply().set(handle);
}

void MaterialService::onSetMaterialResource(command::material::SetMaterialResource &cmd)
{
    assert(m_graphicsManagers != nullptr);

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

    // TODO: hand the setup to CommandOrderService for recording and retire the old resource against the owning
    // render phase's gate. Until then the setup is retained and cleaned at shutdown.
    m_orphanedSetups.push_back(std::move(cmd.setup));
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
    m_listenForCreate.init(bus);
    m_listenForSet.init(bus);
}

void MaterialService::initListeners(common::EventBus &bus)
{
    m_listenForFrame.init(bus);
}

void MaterialService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForCreate.cleanup(bus);
    m_listenForSet.cleanup(bus);
}

void MaterialService::cleanupListeners(common::EventBus &bus)
{
    m_listenForFrame.cleanup(bus);
}
} // namespace star::service
