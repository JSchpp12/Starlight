#include "managers/ManagerRenderResource.hpp"

#include "job/TransferWorker.hpp"
#include "job/tasks/TransferTask.hpp"

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
star::job::TaskManager *star::ManagerRenderResource::managerTaskSystem = nullptr;
size_t star::ManagerRenderResource::s_numStandardTransferWorkers = 0;
std::atomic<size_t> star::ManagerRenderResource::s_nextStandardWorker{0};
std::vector<uint32_t> star::ManagerRenderResource::s_workerQueueFamilyIndices;

void star::ManagerRenderResource::setWorkerQueueFamilyIndices(std::vector<uint32_t> indices)
{
    s_workerQueueFamilyIndices = std::move(indices);
}

uint32_t star::ManagerRenderResource::getPrimaryTransferQueueFamilyIndex()
{
    assert(!s_workerQueueFamilyIndices.empty() && "Transfer worker family indices not published yet");
    return s_workerQueueFamilyIndices[0];
}

void star::ManagerRenderResource::init(const Handle &deviceID, star::core::device::StarDevice *device,
                                       job::TaskManager &taskManager, const int &numFramesInFlight)
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

    managerTaskSystem = &taskManager;

    const size_t totalTransferWorkers = taskManager.getNumOfWorkersForType(TransferWorkerHandle(0));
    s_numStandardTransferWorkers = totalTransferWorkers > 1 ? totalTransferWorkers - 1 : 1;
    s_nextStandardWorker.store(0, std::memory_order_relaxed);
}

uint16_t star::ManagerRenderResource::submitStandardTransferTask(job::tasks::transfer::TransferPayload payload)
{
    assert(managerTaskSystem && "ManagerRenderResource not initialized");

    auto task = job::tasks::transfer::CreateTransferTask(std::move(payload));

    // Worker 0 is reserved for high-priority tasks. Standard workers are 1..N.
    // When only one transfer worker exists, both high- and standard-priority work are routed to worker 0.
    const uint16_t initialWorkerId =
        s_numStandardTransferWorkers == 1
            ? uint16_t{0}
            : static_cast<uint16_t>(
                  (s_nextStandardWorker.fetch_add(1, std::memory_order_relaxed) % s_numStandardTransferWorkers) + 1);

    // Try the round-robin-selected worker first (non-blocking). submitTask only moves the
    // task on success, so on a false return the task is still valid for retry.
    Handle tryWorkerHandle;

    tryWorkerHandle = TransferWorkerHandle(initialWorkerId);
    if (!managerTaskSystem->getIsWorkerQueueFull(task, tryWorkerHandle))
    {
        managerTaskSystem->submitTask(std::move(task), tryWorkerHandle);
        return initialWorkerId;
    }

    // Selected standard worker is full. Try the remaining standard workers before falling back
    // to the dedicated high-priority worker (worker 0).
    if (s_numStandardTransferWorkers > 1)
    {
        for (size_t i = 1; i <= s_numStandardTransferWorkers; ++i)
        {
            const uint16_t candidateId = static_cast<uint16_t>(i);
            if (candidateId == initialWorkerId)
                continue;

            tryWorkerHandle = TransferWorkerHandle(candidateId);
            if (!managerTaskSystem->getIsWorkerQueueFull(task, tryWorkerHandle))
            {
                managerTaskSystem->submitTask(std::move(task), tryWorkerHandle);
                return candidateId;
            }
        }
    }

    // All standard workers full. Fall back to the dedicated high-priority worker 0.
    // Worker 0 already routes Standard-priority payloads into its standard queue, which its
    // thread loop drains only after the high-priority queue is empty.
    managerTaskSystem->submitTask(std::move(task), TransferWorkerHandle(uint16_t{0}));
    return uint16_t{0};
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore)
{
    Handle newBufferHandle = bufferStorage.at(deviceID)->insert(
        FinalizedResourceRequest<star::StarBuffers::Buffer>(std::move(resourceSemaphore)));

    bufferStorage.at(deviceID)->get(newBufferHandle).cpuWorkDoneByTransferThread.store(true);

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                                                     std::unique_ptr<star::TransferRequest::Buffer> newRequest,
                                                     vk::Semaphore *consumingQueueCompleteSemaphore,
                                                     const bool &isHighPriority, uint32_t *outTransferQueueFamilyIndex)
{
    assert(devices.contains(deviceID) && "Device has not been properly initialized");

    Handle newBufferHandle = bufferStorage.at(deviceID)->insert(
        FinalizedResourceRequest<star::StarBuffers::Buffer>(std::move(resourceSemaphore)));
    auto &newFull = bufferStorage.at(deviceID)->get(newBufferHandle);

    newFull.cpuWorkDoneByTransferThread.store(false);

    auto request = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &newFull.cpuWorkDoneByTransferThread, newFull.resourceSemaphore, std::move(newRequest), newFull.resource,
        std::nullopt);

    if (isHighPriority)
    {
        managerTaskSystem->submitTask(job::tasks::transfer::CreateTransferTask(job::tasks::transfer::TransferPayload{
                                          job::tasks::transfer::TransferPriority::High, std::move(request)}),
                                      TransferWorkerHandle(0));
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull.cpuWorkDoneByTransferThread);
        if (outTransferQueueFamilyIndex != nullptr && !s_workerQueueFamilyIndices.empty())
            *outTransferQueueFamilyIndex = s_workerQueueFamilyIndices[0];
    }
    else
    {
        const uint16_t workerId = submitStandardTransferTask(job::tasks::transfer::TransferPayload{
            job::tasks::transfer::TransferPriority::Standard, std::move(request)});
        if (outTransferQueueFamilyIndex != nullptr && !s_workerQueueFamilyIndices.empty())
            *outTransferQueueFamilyIndex = s_workerQueueFamilyIndices[workerId];
    }

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                                                     std::unique_ptr<star::TransferRequest::Texture> newRequest,
                                                     vk::Semaphore *consumingQueueCompleteSemaphore,
                                                     const bool &isHighPriority, uint32_t *outTransferQueueFamilyIndex)
{
    Handle newHandle = textureStorage.at(deviceID)->insert(
        FinalizedResourceRequest<star::StarTextures::Texture>(std::move(resourceSemaphore)));
    auto &newFull = textureStorage.at(deviceID)->get(newHandle);

    newFull.cpuWorkDoneByTransferThread.store(false);

    auto request = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &newFull.cpuWorkDoneByTransferThread, newFull.resourceSemaphore, std::move(newRequest), newFull.resource,
        std::nullopt);

    if (isHighPriority)
    {
        managerTaskSystem->submitTask(job::tasks::transfer::CreateTransferTask(job::tasks::transfer::TransferPayload{
                                          job::tasks::transfer::TransferPriority::High, std::move(request)}),
                                      TransferWorkerHandle(0));
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull.cpuWorkDoneByTransferThread);
        if (outTransferQueueFamilyIndex != nullptr && !s_workerQueueFamilyIndices.empty())
            *outTransferQueueFamilyIndex = s_workerQueueFamilyIndices[0];
    }
    else
    {
        const uint16_t workerId = submitStandardTransferTask(job::tasks::transfer::TransferPayload{
            job::tasks::transfer::TransferPriority::Standard, std::move(request)});
        if (outTransferQueueFamilyIndex != nullptr && !s_workerQueueFamilyIndices.empty())
            *outTransferQueueFamilyIndex = s_workerQueueFamilyIndices[workerId];
    }

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
                                                std::optional<core::graphics::GPUWorkSyncInfo> waitInfo,
                                                const bool &isHighPriority, uint32_t *outTransferQueueFamilyIndex)
{
    auto &container = bufferStorage.at(deviceID)->get(handle);

    if (!container.cpuWorkDoneByTransferThread.load())
    {
        container.cpuWorkDoneByTransferThread.wait(false);
    }
    container.cpuWorkDoneByTransferThread.store(false);

    auto request = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &container.cpuWorkDoneByTransferThread, container.resourceSemaphore, std::move(newRequest), container.resource,
        std::move(waitInfo));

    if (isHighPriority)
    {
        managerTaskSystem->submitTask(job::tasks::transfer::CreateTransferTask(job::tasks::transfer::TransferPayload{
                                          job::tasks::transfer::TransferPriority::High, std::move(request)}),
                                      TransferWorkerHandle(0));
        highPriorityRequestCompleteFlags.at(deviceID).insert(&container.cpuWorkDoneByTransferThread);
        if (outTransferQueueFamilyIndex != nullptr && !s_workerQueueFamilyIndices.empty())
            *outTransferQueueFamilyIndex = s_workerQueueFamilyIndices[0];
    }
    else
    {
        const uint16_t workerId = submitStandardTransferTask(job::tasks::transfer::TransferPayload{
            job::tasks::transfer::TransferPriority::Standard, std::move(request)});
        if (outTransferQueueFamilyIndex != nullptr && !s_workerQueueFamilyIndices.empty())
            *outTransferQueueFamilyIndex = s_workerQueueFamilyIndices[workerId];
    }
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

    managerTaskSystem = nullptr;
    s_workerQueueFamilyIndices.clear();
}
