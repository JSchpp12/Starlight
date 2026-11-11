#include "starlight/service/MaterialService.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/Image.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "device/managers/Material.hpp"
#include "managers/ManagerRenderResource.hpp"
#include "starlight/core/transfer/RegisterTransferWait.hpp"
#include "virtual/StarMaterial.hpp"

#include <cassert>
#include <exception>
#include <utility>
#include <vector>
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

    // One-shot setups are owned by the pass requests and drained by DeviceContext's pending-work cleanup. Anything
    // still in the retire queue was ungated (the owning pass was never triggered) or waiting on a producer; the
    // engine waits for device idleness before services shut down, so a published resource can be destroyed now.
    // A slot that is still pending belongs to its producer and the manager, and is left alone.
    if (m_device != nullptr && m_graphicsManagers != nullptr)
    {
        auto &imageManager = m_graphicsManagers->imageManager;
        auto &bufferManager = m_graphicsManagers->bufferManager;

        for (const auto &entry : m_retires)
        {
            if (entry.isImage)
            {
                if (imageManager.isReady(entry.handle))
                    imageManager.deleteRequest(*m_device, entry.handle);
            }
            else if (bufferManager.isReady(entry.handle))
            {
                bufferManager.deleteRequest(*m_device, entry.handle);
            }
        }
    }

    m_retires.clear();
}

void MaterialService::onMakeMaterial(command::material::MakeMaterial &cmd)
{
    assert(m_graphicsManagers != nullptr);
    assert(m_context != nullptr && "MaterialService requires a DeviceContext to place one-shot work");

    // Prepare first: the definition is owned by the pass request, never by the material. A failed prep must not
    // register a transfer wait: a failed transfer never signals its timeline, which would hang the pass submission.
    setup::RenderResourceSetup::ResourcePrepDefinition definition;
    bool prepareSucceeded = true;
    try
    {
        if (cmd.prep.prepare)
            cmd.prep.prepare(*m_context, definition);
    }
    catch (const std::exception &e)
    {
        prepareSucceeded = false;
        core::logging::warning("MaterialService: material resource preparation failed: ", e.what());
    }

    if (prepareSucceeded && cmd.commandBuffer.isInitialized())
    {
        // The pass must not submit its timeline waits until these transfer resources are CPU-published.
        for (const auto &wait : cmd.waits)
            core::transfer::RegisterTransferWait(*m_context, cmd.commandBuffer, wait.key, wait.semaphore.vkSemaphore,
                                                 wait.semaphore.signalValue, wait.stages);

        m_context->getManagerCommandBuffer().m_manager.get(cmd.commandBuffer).add(std::move(definition));
    }
    else
    {
        // No pass will record this work (or the prep failed). cleanupRender assumes device idleness at teardown only,
        // so wait explicitly before releasing while the app is running.
        if (m_device != nullptr)
            m_device->getVulkanDevice().waitIdle();
        definition.releaseAssumeDeviceIdle(*m_context);
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
    {
        core::logging::warning("MaterialService: SetMaterialResource for an unknown material");
        cmd.getReply().set(false);
        return;
    }

    // Retirement tracks the material's primary resource (set 0 / binding 0). A secondary binding (e.g. a bump map)
    // needs per-binding roles; swapping one must not retire the primary resource.
    const bool isPrimaryBinding = cmd.setIndex == 0 && cmd.bindingIndex == 0;
    const Handle previous = isPrimaryBinding ? material->getResourceHandle() : Handle{};

    // Prepare first: the incoming resource's one-shot work is required both for validation and for the pass.
    setup::RenderResourceSetup::ResourcePrepDefinition definition;
    bool prepareSucceeded = true;
    try
    {
        if (cmd.prep.prepare)
            cmd.prep.prepare(*m_context, definition);
    }
    catch (const std::exception &e)
    {
        prepareSucceeded = false;
        core::logging::warning("MaterialService: resource preparation failed: ", e.what());
    }

    // Validate before rebinding so a failed transfer keeps the old resource bound.
    if (!prepareSucceeded || !validateIncomingResource(cmd.newHandle, cmd.waits))
    {
        core::logging::warning("MaterialService: incoming resource is not usable; keeping the previous resource");

        // The transfer may have submitted GPU work. Wait through the raw device (DeviceContext::waitIdle() would
        // tear down the task manager mid-run), then free the abandoned per-make resources and the resource record.
        if (m_device != nullptr)
            m_device->getVulkanDevice().waitIdle();

        definition.releaseAssumeDeviceIdle(*m_context);
        deleteIncomingResource(cmd.newHandle);

        cmd.getReply().set(false);
        return;
    }

    material->setResourceHandle(cmd.setIndex, cmd.bindingIndex, std::move(cmd.newHandle), cmd.frameInFlightIndex);

    // The old resource's last GPU use precedes this recording submission, so it is safe to queue against that pass's
    // gate as soon as the material stops referencing it.
    if (isPrimaryBinding)
    {
        const Handle current = material->getResourceHandle();
        if (previous.isInitialized() && !previous.isSameElementAs(current))
            queueRetire(previous, cmd.recordingCommandBuffer);
    }

    // The swap command carries the owning render phase's command buffer; its one-time wait info is consumed by the
    // submission that records that phase. The pass must not submit its timeline waits until these transfer resources
    // are CPU-published.
    if (cmd.recordingCommandBuffer.isInitialized())
    {
        for (const auto &wait : cmd.waits)
            core::transfer::RegisterTransferWait(*m_context, cmd.recordingCommandBuffer, wait.key,
                                                 wait.semaphore.vkSemaphore, wait.semaphore.signalValue, wait.stages);

        m_context->getManagerCommandBuffer().m_manager.get(cmd.recordingCommandBuffer).add(std::move(definition));
    }
    else
    {
        // No pass can record the one-shot work; release it once the transfer has retired.
        if (m_device != nullptr)
            m_device->getVulkanDevice().waitIdle();
        definition.releaseAssumeDeviceIdle(*m_context);
    }

    cmd.getReply().set(true);
}

bool MaterialService::validateIncomingResource(const StarShaderInfo::ShaderInfo &info,
                                               const std::vector<setup::TransferWait> &waits)
{
    // A transfer a pass must wait on may have failed independently of the material's own resource (the GPU skybox path
    // binds a synchronously created texture while the conversion inputs arrive through a buffer transfer). A failed
    // buffer resolves to a blank record, so reject the swap before the material ever binds it.
    for (const auto &wait : waits)
    {
        if (!wait.key.isInitialized() || wait.key.getType() != m_graphicsManagers->bufferManager.getHandleType())
            continue;

        auto &bufferManager = m_graphicsManagers->bufferManager;
        bufferManager.waitForReady(wait.key);

        const auto *record = bufferManager.get(wait.key);
        if (record == nullptr || record->getBuffer() == nullptr ||
            record->getBuffer()->getVulkanBuffer() == VK_NULL_HANDLE)
            return false;
    }

    if (info.textureInfo.has_value())
    {
        const auto &textureInfo = *info.textureInfo;
        if (!textureInfo.handle.isInitialized())
            return textureInfo.texture != nullptr;

        auto &imageManager = m_graphicsManagers->imageManager;
        imageManager.waitForReady(textureInfo.handle);

        const auto *record = imageManager.get(textureInfo.handle);
        if (record == nullptr || record->getTexture() == nullptr)
            return false;

        const auto *texture = record->getTexture();
        if (texture->getVulkanImage() == VK_NULL_HANDLE)
            return false;

        return texture->getImageView().get(texture->getBaseFormat()) != VK_NULL_HANDLE;
    }

    if (info.bufferInfo.has_value())
    {
        const auto &bufferInfo = *info.bufferInfo;
        if (bufferInfo.buffer.has_value())
            return *bufferInfo.buffer != nullptr;

        if (!bufferInfo.handle.has_value())
            return false;

        const Handle handle = *bufferInfo.handle;
        if (handle.getType() == m_graphicsManagers->bufferManager.getHandleType())
        {
            auto &bufferManager = m_graphicsManagers->bufferManager;
            bufferManager.waitForReady(handle);

            const auto *record = bufferManager.get(handle);
            return record != nullptr && record->getBuffer() != nullptr &&
                   record->getBuffer()->getVulkanBuffer() != VK_NULL_HANDLE;
        }

        ManagerRenderResource::waitForReady(m_context->getDeviceID(), handle);
        return ManagerRenderResource::getBuffer(m_context->getDeviceID(), handle).getVulkanBuffer() != VK_NULL_HANDLE;
    }

    return false;
}

void MaterialService::deleteIncomingResource(const StarShaderInfo::ShaderInfo &info)
{
    if (info.textureInfo.has_value())
    {
        const auto &textureInfo = *info.textureInfo;
        if (!textureInfo.handle.isInitialized())
            return;

        auto &imageManager = m_graphicsManagers->imageManager;
        if (imageManager.isReady(textureInfo.handle))
            imageManager.deleteRequest(*m_device, textureInfo.handle);
        return;
    }

    if (!info.bufferInfo.has_value() || !info.bufferInfo->handle.has_value())
        return;

    const Handle handle = *info.bufferInfo->handle;
    if (handle.getType() != m_graphicsManagers->bufferManager.getHandleType())
        return;

    auto &bufferManager = m_graphicsManagers->bufferManager;
    if (bufferManager.isReady(handle))
        bufferManager.deleteRequest(*m_device, handle);
}

void MaterialService::onStartOfNextFrame(const event::StartOfNextFrame &, bool &keepAlive)
{
    drainRetires();

    keepAlive = true;
}

void MaterialService::queueRetire(const Handle &resource, const Handle &pass)
{
    if (m_graphicsManagers == nullptr)
        return;

    RetireEntry entry{};
    entry.handle = resource;

    if (resource.getType() == m_graphicsManagers->imageManager.getHandleType())
    {
        entry.isImage = true;
    }
    else if (resource.getType() == m_graphicsManagers->bufferManager.getHandleType())
    {
        entry.isImage = false;
    }
    else
    {
        core::logging::warning("MaterialService: replaced resource has an unknown handle type; it will not be retired");
        return;
    }

    // Capture the gate now: once StartOfNextFrame has run, CommandOrderService has cleared the pass triggers and
    // GetPassInfo can no longer report the recording submission. An ungated entry is held for the teardown backstop.
    if (pass.isInitialized() && m_context != nullptr)
    {
        const auto gate = core::graphics::GetRecordingSubmissionInfo(*m_context, pass).gate;
        if (gate.semaphore != VK_NULL_HANDLE && gate.signalValue != 0)
            entry.gate = gate;
    }

    if (!entry.gate)
        core::logging::warning(
            "MaterialService: no recording gate available for a retired resource; held until teardown");

    m_retires.push_back(std::move(entry));
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
        if (!it->gate || device.getSemaphoreCounterValue(it->gate->semaphore) < it->gate->signalValue)
        {
            ++it;
            continue;
        }

        if (it->isImage)
        {
            // A slot whose producer has not published yet still belongs to that producer; deleting it would strand
            // the texture it later creates. Retry next frame.
            if (!imageManager.isReady(it->handle))
            {
                ++it;
                continue;
            }

            imageManager.deleteRequest(*m_device, it->handle);
        }
        else
        {
            if (!bufferManager.isReady(it->handle))
            {
                ++it;
                continue;
            }

            bufferManager.deleteRequest(*m_device, it->handle);
        }

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
