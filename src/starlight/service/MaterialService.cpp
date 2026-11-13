#include "starlight/service/MaterialService.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/managers/Image.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "device/managers/Material.hpp"
#include "managers/ManagerRenderResource.hpp"
#include "starlight/core/transfer/ReadinessCondition.hpp"
#include "starlight/core/transfer/RegisterTransferWait.hpp"
#include "starlight/event/MaterialSwapRolledBack.hpp"
#include "virtual/StarMaterial.hpp"
#include "wrappers/graphics/StarShaderInfo.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>
#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace
{
/// @brief Startup may wait for the conversion pipeline's first build.
constexpr std::chrono::milliseconds kInitialGateTimeout{30000};
} // namespace

namespace star::service
{
static star::Handle ResourceHandleOf(const star::StarShaderInfo::ShaderInfo &info)
{
    if (info.textureInfo.has_value())
        return info.textureInfo->handle;

    if (info.bufferInfo.has_value() && info.bufferInfo->handle.has_value())
        return *info.bufferInfo->handle;

    return star::Handle{};
}

static star::StarShaderInfo::ShaderInfo ToShaderInfo(const star::Handle &resource,
                                                     const star::core::device::manager::GraphicsContainer &managers)
{
    if (resource.getType() == managers.imageManager.getHandleType())
        return star::StarShaderInfo::TextureInfo{resource, vk::ImageLayout::eShaderReadOnlyOptimal};

    return star::StarShaderInfo::BufferInfo{resource};
}

static bool IsCompatibleIncomingHandle(const star::Handle &handle,
                                       const star::core::device::manager::GraphicsContainer &managers)
{
    if (!handle.isInitialized())
        return false;

    return handle.getType() == managers.imageManager.getHandleType() ||
           handle.getType() == managers.bufferManager.getHandleType();
}

MaterialService::MaterialService(MaterialService &&other) noexcept
    : m_defaultOnFailure(other.m_defaultOnFailure), m_listenForSet(*this), m_listenForMake(*this),
      m_listenForFrame(*this), m_listenForWaitResolved(*this), m_context(other.m_context), m_device(other.m_device),
      m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus), m_evtBus(other.m_evtBus),
      m_transactions(std::move(other.m_transactions)), m_retires(std::move(other.m_retires)),
      m_commitBuffer(std::move(other.m_commitBuffer)), m_commitCount(other.m_commitCount), m_listenersInitialized(false)
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

    m_commitBuffer.resize(5);
    m_commitCount = 0;

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
    m_transactions.clear();
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

    if (prepareSucceeded && !cmd.conditions.empty() && !star::WaitForReadiness(cmd.conditions, kInitialGateTimeout))
    {
        // The initial material has no previous resource to keep, and MakeMaterial's reply has no failure channel.
        // Release what the definition owns and fail loudly rather than record a definition whose dependencies are
        // unready.

        STAR_THROW("MaterialService: initial material readiness gate failed; aborting");
    }

    if (prepareSucceeded && cmd.commandBuffer.isInitialized())
    {
        // The pass must not submit its timeline waits until these transfer resources are CPU-published.
        for (const auto &wait : cmd.waits)
            core::transfer::RegisterTransferWait(*m_context, cmd.commandBuffer, wait.key, wait.semaphore.vkSemaphore,
                                                 wait.semaphore.signalValue, wait.stages);

        auto &record = m_context->getManagerCommandBuffer().m_manager.get(cmd.commandBuffer);
        // Initial material setup has no rollback transaction; the pass's one-shot setup owns the definition.
        record.add(std::move(definition));
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
    auto *material = materials.getMaterial(cmd.target.material);
    if (material == nullptr)
    {
        core::logging::warning("MaterialService: SetMaterialResource for an unknown material");
        cmd.getReply().set(
            std::unexpected(exception::SwapException{.reason = exception::SwapException::Reason::UnknownMaterial,
                                                     .detail = "SetMaterialResource for an unknown material"}));
        return;
    }

    if (!IsCompatibleIncomingHandle(cmd.incoming.handle, *m_graphicsManagers))
    {
        core::logging::warning("MaterialService: incoming material resource has an unsupported handle type; rejecting");
        cmd.getReply().set(std::unexpected(
            exception::SwapException{.reason = exception::SwapException::Reason::IncompatibleResource,
                                     .detail = "incoming material resource has an unsupported handle type"}));
        return;
    }

    // Prepare first: the incoming resource's one-shot work is required by the owning pass.
    setup::RenderResourceSetup::ResourcePrepDefinition definition;
    bool prepareSucceeded = true;
    std::string prepareError;
    try
    {
        if (cmd.prep.prepare)
            cmd.prep.prepare(*m_context, definition);
    }
    catch (const std::exception &e)
    {
        prepareSucceeded = false;
        prepareError = e.what();
        core::logging::warning("MaterialService: resource preparation failed: ", e.what());
    }

    if (!prepareSucceeded)
    {
        // The transfer may have submitted GPU work. Wait through the raw device (DeviceContext::waitIdle() would
        // tear down the task manager mid-run), then free the abandoned per-make resources and the resource record.
        if (m_device != nullptr)
            m_device->getVulkanDevice().waitIdle();

        definition.releaseAssumeDeviceIdle(*m_context);
        deleteIncomingResource(cmd.incoming);

        cmd.getReply().set(std::unexpected(exception::SwapException{
            .reason = exception::SwapException::Reason::PreparationFailed, .detail = std::move(prepareError)}));
        return;
    }

    const Handle pass = cmd.recordingCommandBuffer;
    const auto previousInfo =
        material->getResourceInfo(cmd.target.setIndex, cmd.target.bindingIndex, cmd.target.frameInFlightIndex);
    const Handle previousResource = previousInfo.has_value() ? ResourceHandleOf(*previousInfo) : Handle{};
    const Handle newResource = cmd.incoming.handle;

    std::vector<Handle> conditionResources;
    conditionResources.reserve(cmd.incoming.readiness.size());
    for (const auto &condition : cmd.incoming.readiness)
        conditionResources.push_back(condition.resource);

    std::optional<core::graphics::SemaphoreInfo> gate;
    if (pass.isInitialized())
    {
        const auto recordingInfo = core::graphics::GetRecordingSubmissionInfo(*m_context, pass);
        if (recordingInfo.gate.semaphore != VK_NULL_HANDLE && recordingInfo.gate.signalValue != 0)
            gate = recordingInfo.gate;
    }

    const auto onFailure = cmd.onFailure != nullptr ? cmd.onFailure : m_defaultOnFailure;

    if (!pass.isInitialized())
    {
        // error state, target pass must always be initialized
        cmd.getReply().set(std::unexpected(
            exception::SwapException{.reason = exception::SwapException::Reason::UnknownCommandBufferPass,
                                     .detail = "SetMaterialResource for an unknown material"}));
        return;
    }

    const auto definitionId = m_context->getManagerCommandBuffer().m_manager.get(pass).add(std::move(definition));

    // Material conditions are gated by the owning pass; the transaction handler decides rollback vs terminate.
    core::transfer::RegisterPassConditions(*m_context, pass, std::move(cmd.incoming.readiness),
                                           ReadinessFailurePolicy::KeepPrevious);

    m_transactions.push_back(SwapTransaction{.target = cmd.target,
                                             .pass = pass,
                                             .newResource = newResource,
                                             .previousResource = previousResource,
                                             .onFailure = onFailure,
                                             .gate = gate,
                                             .definition = definitionId,
                                             .conditionResources = std::move(conditionResources)});

    cmd.getReply().set(
        std::expected<command::material::SwapStatus, exception::SwapException>{command::material::SwapStatus::Pending});
}

void MaterialService::deleteIncomingResource(const command::material::set_material_resource::IncomingResource &incoming)
{
    const Handle handle = incoming.handle;
    if (!handle.isInitialized())
        return;

    auto &imageManager = m_graphicsManagers->imageManager;
    auto &bufferManager = m_graphicsManagers->bufferManager;

    if (handle.getType() == imageManager.getHandleType())
    {
        if (imageManager.isReady(handle))
            imageManager.deleteRequest(*m_device, handle);
        return;
    }

    if (handle.getType() == bufferManager.getHandleType())
    {
        if (bufferManager.isReady(handle))
            bufferManager.deleteRequest(*m_device, handle);
        return;
    }

    core::logging::warning("MaterialService: cannot delete incoming resource with an unknown handle type");
}

void MaterialService::onStartOfNextFrame(const event::StartOfNextFrame &, bool &keepAlive)
{
    finalizeTransactions();
    flushCommits();
    drainRetires();

    keepAlive = true;
}

void MaterialService::onPassCPURecordingWaitResolved(const event::PassCPURecordingWaitResolved &event, bool &keepAlive)
{
    keepAlive = true;

    const auto intersects = [](const std::vector<Handle> &resources, const Handle &resource) {
        return std::find(resources.begin(), resources.end(), resource) != resources.end();
    };

    const auto touched = [&](const std::vector<Handle> &resources, const SwapTransaction &transaction) {
        if (transaction.newResource.isInitialized() && intersects(resources, transaction.newResource))
            return true;

        return std::any_of(transaction.conditionResources.begin(), transaction.conditionResources.end(),
                           [&](const Handle &resource) { return intersects(resources, resource); });
    };

    for (const auto &result : event.getResults())
    {
        for (auto it = m_transactions.begin(); it != m_transactions.end();)
        {
            if (it->pass != result.pass)
            {
                ++it;
                continue;
            }

            if (touched(result.failedResources, *it))
            {
                it->onFailure(*this, *it);
                it = m_transactions.erase(it);
                continue;
            }

            if (touched(result.pendingResources, *it))
            {
                // The pass skipped this frame; keep the previous resource until a later result arrives.
                it->sawResult = true;
                it->pending = true;
                ++it;
                continue;
            }

            // Every resource this swap depends on was ready; commit the new binding.
            commitSwap(*it);
            it = m_transactions.erase(it);
        }
    }

    flushCommits();
}

void MaterialService::commitSwap(SwapTransaction &transaction)
{
    auto *material = m_graphicsManagers->materialManager.getMaterial(transaction.target.material);
    if (material == nullptr)
    {
        // The material no longer exists; treat the swap as rolled back so the app still gets a terminal event.
        RollbackSwap(*this, transaction);
        return;
    }

    material->setResourceHandle(transaction.target.setIndex, transaction.target.bindingIndex,
                                ToShaderInfo(transaction.newResource, *m_graphicsManagers),
                                transaction.target.frameInFlightIndex);

    recordCommit(transaction.target.material, transaction.newResource);

    if (transaction.previousResource.isInitialized())
        queueRetire(transaction.previousResource, transaction.gate);
}

void MaterialService::recordCommit(const Handle &material, const Handle &newResource)
{
    if (m_commitCount >= m_commitBuffer.size())
        m_commitBuffer.resize(m_commitBuffer.empty() ? 5 : m_commitBuffer.size() * 2);

    m_commitBuffer[m_commitCount++] = {.material = material, .newResource = newResource};
}

void MaterialService::flushCommits()
{
    if (m_commitCount == 0)
        return;

    if (m_evtBus != nullptr)
    {
        m_evtBus->emit(event::MaterialSwapCommitted{
            std::span<const MaterialSwapCommitInfo>{m_commitBuffer.data(), m_commitCount}});
    }

    m_commitCount = 0;
}

void MaterialService::RollbackSwap(MaterialService &service, SwapTransaction &transaction)
{
    if (transaction.pass.isInitialized())
    {
        service.m_context->getManagerCommandBuffer()
            .m_manager.get(transaction.pass)
            .abandon(transaction.definition, transaction.gate.value_or(core::graphics::SemaphoreInfo{}));
    }

    if (transaction.newResource.isInitialized())
        service.queueRetire(transaction.newResource, transaction.gate);

    if (service.m_evtBus != nullptr)
        service.m_evtBus->emit(
            event::MaterialSwapRolledBack{transaction.target.material, transaction.previousResource});
}

void MaterialService::TerminateSwap(MaterialService &, SwapTransaction &)
{
    core::logging::error("MaterialService: material swap failed with a Terminate policy; aborting");
    std::terminate();
}

void MaterialService::finalizeTransactions()
{
    for (auto it = m_transactions.begin(); it != m_transactions.end();)
    {
        if (it->pending)
        {
            // Waiting for a later pass result; keep the previous resource alive.
            ++it;
            continue;
        }

        // No result arrived (the pass was not submitted this frame); treat it as committed.
        commitSwap(*it);
        it = m_transactions.erase(it);
    }
}

void MaterialService::queueRetire(const Handle &resource, std::optional<core::graphics::SemaphoreInfo> gate)
{
    if (m_graphicsManagers == nullptr || !resource.isInitialized())
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

    if (gate.has_value())
        entry.gate = gate;
    else
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
    m_listenForWaitResolved.init(bus);
}

void MaterialService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForSet.cleanup(bus);
    m_listenForMake.cleanup(bus);
}

void MaterialService::cleanupListeners(common::EventBus &bus)
{
    m_listenForFrame.cleanup(bus);
    m_listenForWaitResolved.cleanup(bus);
}
} // namespace star::service
