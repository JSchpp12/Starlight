#include "managers/ManagerRenderResource.hpp"

#include "job/TransferWorker.hpp"
#include "job/tasks/TransferTask.hpp"
#include "starlight/command/transfer/SubmitTransferTask.hpp"

std::unordered_map<star::Handle, star::core::device::StarDevice *, star::HandleHash>
    star::ManagerRenderResource::devices;
std::unordered_map<star::Handle, std::set<boost::atomic<bool> *>, star::HandleHash>
    star::ManagerRenderResource::highPriorityRequestCompleteFlags;
std::unordered_map<star::Handle,
                   std::unique_ptr<star::core::ManagedHandleContainer<
                       star::ManagerRenderResource::FinalizedResourceRequest<star::StarBuffers::Buffer>, 3500>>,
                   star::HandleHash>
    star::ManagerRenderResource::bufferStorage;
std::unordered_map<star::Handle,
                   std::unique_ptr<star::core::ManagedHandleContainer<
                       star::ManagerRenderResource::FinalizedResourceRequest<star::StarTextures::Texture>, 2000>>,
                   star::HandleHash>
    star::ManagerRenderResource::textureStorage;
star::core::CommandBus *star::ManagerRenderResource::s_cmdBus = nullptr;

void star::ManagerRenderResource::init(const Handle &deviceID, star::core::device::StarDevice *device,
                                       star::core::CommandBus &cmdBus)
{
    devices.insert(std::make_pair(deviceID, std::move(device)));
    bufferStorage.insert(std::make_pair(
        deviceID,
        std::make_unique<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>, 3500>>(
            common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::BufferTypeName))));
    textureStorage.insert(std::make_pair(
        deviceID,
        std::make_unique<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>, 2000>>(
            common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::TextureTypeName))));

    highPriorityRequestCompleteFlags.insert(std::make_pair(deviceID, std::set<boost::atomic<bool> *>()));

    s_cmdBus = &cmdBus;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID)
{
    Handle newBufferHandle = bufferStorage.at(deviceID)->insert(FinalizedResourceRequest<star::StarBuffers::Buffer>());

    bufferStorage.at(deviceID)->get(newBufferHandle).cpuWorkDoneByTransferThread.store(true);

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID,
                                                     std::unique_ptr<star::TransferRequest::Buffer> newRequest,
                                                     vk::Semaphore *consumingQueueCompleteSemaphore,
                                                     const bool &isHighPriority, uint32_t *outTransferQueueFamilyIndex)
{
    assert(devices.contains(deviceID) && "Device has not been properly initialized");

    Handle newBufferHandle = bufferStorage.at(deviceID)->insert(FinalizedResourceRequest<star::StarBuffers::Buffer>());
    auto &newFull = bufferStorage.at(deviceID)->get(newBufferHandle);

    newFull.cpuWorkDoneByTransferThread.store(false);

    auto request = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &newFull.cpuWorkDoneByTransferThread, std::move(newRequest), newFull.resource);

    command::transfer::SubmitTransferTask cmd{job::tasks::transfer::CreateTransferTask(
        job::tasks::transfer::TransferPayload{isHighPriority ? job::tasks::transfer::TransferPriority::High
                                                             : job::tasks::transfer::TransferPriority::Standard,
                                              std::move(request)})};

    s_cmdBus->submit(cmd);
    const auto result = cmd.getReply().get();

    if (isHighPriority)
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull.cpuWorkDoneByTransferThread);

    if (outTransferQueueFamilyIndex != nullptr)
        *outTransferQueueFamilyIndex = result.queueFamilyIndex;

    newFull.gpuWorkDoneSignaledInfo = result.semaphore;
    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID,
                                                     std::unique_ptr<star::TransferRequest::Texture> newRequest,
                                                     vk::Semaphore *consumingQueueCompleteSemaphore,
                                                     const bool &isHighPriority, uint32_t *outTransferQueueFamilyIndex)
{
    assert(s_cmdBus != nullptr);

    Handle newHandle = textureStorage.at(deviceID)->insert(FinalizedResourceRequest<star::StarTextures::Texture>());
    auto &newFull = textureStorage.at(deviceID)->get(newHandle);

    newFull.cpuWorkDoneByTransferThread.store(false);

    auto request = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &newFull.cpuWorkDoneByTransferThread, std::move(newRequest), newFull.resource);

    command::transfer::SubmitTransferTask cmd{job::tasks::transfer::CreateTransferTask(
        job::tasks::transfer::TransferPayload{isHighPriority ? job::tasks::transfer::TransferPriority::High
                                                             : job::tasks::transfer::TransferPriority::Standard,
                                              std::move(request)})};

    s_cmdBus->submit(cmd);
    const auto result = cmd.getReply().get(); // synchronous: already populated

    if (isHighPriority)
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull.cpuWorkDoneByTransferThread);

    if (outTransferQueueFamilyIndex != nullptr)
        *outTransferQueueFamilyIndex = result.queueFamilyIndex;

    newFull.gpuWorkDoneSignaledInfo = result.semaphore;
    return newHandle;
}

void star::ManagerRenderResource::frameUpdate(const Handle &deviceID, const uint8_t &frameInFlightIndex)
{
    (void)frameInFlightIndex;
    for (auto &request : highPriorityRequestCompleteFlags.at(deviceID))
    {
        if (!request->load())
        {
            request->wait(false);
        }
    }

    highPriorityRequestCompleteFlags.at(deviceID).clear();
}

void star::ManagerRenderResource::updateRequest(const Handle &deviceID,
                                                std::unique_ptr<TransferRequest::Buffer> newRequest,
                                                const star::Handle &handle,
                                                std::optional<core::graphics::SemaphoreInfo> waitInfo,
                                                const bool &isHighPriority, uint32_t *outTransferQueueFamilyIndex)
{
    auto &container = bufferStorage.at(deviceID)->get(handle);

    if (!container.cpuWorkDoneByTransferThread.load())
    {
        container.cpuWorkDoneByTransferThread.wait(false);
    }
    container.cpuWorkDoneByTransferThread.store(false);

    auto request = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &container.cpuWorkDoneByTransferThread, std::move(newRequest), container.resource,
        waitInfo.has_value() ? star::core::graphics::GPUWorkSyncInfo{.workWaitOn = waitInfo.value()}
                             : star::core::graphics::GPUWorkSyncInfo{});
    command::transfer::SubmitTransferTask cmd{job::tasks::transfer::CreateTransferTask(
        job::tasks::transfer::TransferPayload{isHighPriority ? job::tasks::transfer::TransferPriority::High
                                                             : job::tasks::transfer::TransferPriority::Standard,
                                              std::move(request)})};

    s_cmdBus->submit(cmd);
    const auto result = cmd.getReply().get(); // synchronous: already populated
    container.gpuWorkDoneSignaledInfo = result.semaphore;

    if (isHighPriority)
    {
        highPriorityRequestCompleteFlags.at(deviceID).insert(&container.cpuWorkDoneByTransferThread);
    }
    if (outTransferQueueFamilyIndex != nullptr)
        *outTransferQueueFamilyIndex = result.queueFamilyIndex;
}

bool star::ManagerRenderResource::isReady(const Handle &deviceID, const Handle &handle)
{
    if (handle.getType() ==
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::BufferTypeName))
    {
        return bufferStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread.load();
    }
    else if (handle.getType() ==
             common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::TextureTypeName))
    {
        return textureStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread.load();
    }
    else
    {
        throw std::runtime_error("Invalid type provided");
    }

    return false;
}

void star::ManagerRenderResource::waitForReady(const Handle &deviceID, const Handle &handle)
{
    assert(deviceID.getType() ==
           common::HandleTypeRegistry::instance().getType(common::special_types::DeviceTypeName).value());

    boost::atomic<bool> *fence = nullptr;

    if (handle.getType() ==
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::BufferTypeName))
    {
        fence = &bufferStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread;
    }
    else if (handle.getType() ==
             common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::TextureTypeName))
    {
        fence = &textureStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread;
    }
    else
    {
        throw std::runtime_error("Invalid handle type");
    }

    assert(fence != nullptr && "Fence not valid or found");
    if (!fence->load())
    {
        fence->wait(false);
    }
}

star::StarBuffers::Buffer &star::ManagerRenderResource::getBuffer(const Handle &deviceID, const star::Handle &handle)
{
    assert(handle.getType() ==
               common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::BufferTypeName) &&
           "Handle provided is not a buffer handle");

    auto &container = bufferStorage.at(deviceID)->get(handle);
    if (!container.resource)
    {
        if (!container.cpuWorkDoneByTransferThread.load())
        {
            container.cpuWorkDoneByTransferThread.wait(false);
        }
        else
        {
            STAR_THROW("Unknown error has occurred during getBuffer. This can happen if the transfer "
                       "flag is not properly reset");
        }
    }

    assert(container.resource && "Resource should always exist before returning");

    return *container.resource;
}

star::StarTextures::Texture &star::ManagerRenderResource::getTexture(const Handle &deviceID, const star::Handle &handle)
{
    assert(handle.getType() ==
               common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::TextureTypeName) &&
           "Handle provided is not a texture handle");

    const auto &container = textureStorage.at(deviceID)->get(handle);
    if (!container.resource)
    {
        if (!container.cpuWorkDoneByTransferThread.load())
        {
            container.cpuWorkDoneByTransferThread.wait(false);
        }
        else
        {
            throw std::runtime_error("Unknown error has occurred during getBuffer. This can happen if the transfer "
                                     "flag is not properly reset");
        }
    }

    assert(container.resource && "Resource should always exist before returning");

    return *container.resource;
}

void star::ManagerRenderResource::cleanup(const Handle &deviceID, core::device::StarDevice &device)
{
    bufferStorage.at(deviceID)->cleanupAll(&device);
    bufferStorage.at(deviceID).reset();
    textureStorage.at(deviceID)->cleanupAll(&device);
    textureStorage.at(deviceID).reset();

    s_cmdBus = nullptr;
}
