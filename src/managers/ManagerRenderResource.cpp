#include "managers/ManagerRenderResource.hpp"

auto star::ManagerRenderResource::devices =
    std::unordered_map<star::Handle, std::shared_ptr<star::core::device::StarDevice>, star::HandleHash>();
auto star::ManagerRenderResource::highPriorityRequestCompleteFlags =
    std::unordered_map<star::Handle, std::set<boost::atomic<bool> *>, star::HandleHash>();
auto star::ManagerRenderResource::bufferStorage =
    std::unordered_map<star::Handle,
                       std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>,
                                                                    star::Handle_Type::buffer, 100>>,
                       star::HandleHash>();
auto star::ManagerRenderResource::textureStorage =
    std::unordered_map<star::Handle,
                       std::unique_ptr<core::ManagedHandleContainer<
                           FinalizedResourceRequest<star::StarTextures::Texture>, star::Handle_Type::texture, 50>>,
                       star::HandleHash>();

void star::ManagerRenderResource::init(const Handle &deviceID, std::shared_ptr<star::core::device::StarDevice> device,
                                       std::shared_ptr<star::job::TransferWorker> worker, const int &numFramesInFlight)
{
    devices.insert(std::make_pair(deviceID, std::move(device)));
    bufferStorage.insert(std::make_pair(
        deviceID, std::make_unique<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>,
                                                                star::Handle_Type::buffer, 100>>()));
    textureStorage.insert(std::make_pair(
        deviceID, std::make_unique<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>,
                                                                star::Handle_Type::texture, 50>>()));

    highPriorityRequestCompleteFlags.insert(std::make_pair(deviceID, std::set<boost::atomic<bool> *>()));

    managerWorker = worker;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                                                     const bool &isHighPriority)
{
    Handle newBufferHandle = bufferStorage.at(deviceID)->insert(
        FinalizedResourceRequest<star::StarBuffers::Buffer>(std::move(resourceSemaphore)));

    bufferStorage.at(deviceID)->get(newBufferHandle).cpuWorkDoneByTransferThread.store(true);

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                                                     std::unique_ptr<star::TransferRequest::Buffer> newRequest,
                                                     vk::Semaphore *consumingQueueCompleteSemaphore,
                                                     const bool &isHighPriority)
{
    assert(devices.contains(deviceID) && "Device has not been properly initialized");

    Handle newBufferHandle = bufferStorage.at(deviceID)->insert(
        FinalizedResourceRequest<star::StarBuffers::Buffer>(std::move(resourceSemaphore)));
    auto &newFull = bufferStorage.at(deviceID)->get(newBufferHandle);

    newFull.cpuWorkDoneByTransferThread.store(false);
    managerWorker->add(newFull.cpuWorkDoneByTransferThread, newFull.resourceSemaphore, std::move(newRequest),
                       newFull.resource, isHighPriority);

    if (isHighPriority)
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull.cpuWorkDoneByTransferThread);

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(const Handle &deviceID, vk::Semaphore resourceSemaphore,
                                                     std::unique_ptr<star::TransferRequest::Texture> newRequest,
                                                     vk::Semaphore *consumingQueueCompleteSemaphore,
                                                     const bool &isHighPriority)
{
    Handle newHandle = textureStorage.at(deviceID)->insert(
        FinalizedResourceRequest<star::StarTextures::Texture>(std::move(resourceSemaphore)));
    auto &newFull = textureStorage.at(deviceID)->get(newHandle);

    newFull.cpuWorkDoneByTransferThread.store(false);
    managerWorker->add(newFull.cpuWorkDoneByTransferThread, newFull.resourceSemaphore, std::move(newRequest),
                       newFull.resource, isHighPriority);

    if (isHighPriority)
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull.cpuWorkDoneByTransferThread);

    return newHandle;
}

void star::ManagerRenderResource::frameUpdate(const Handle &deviceID, const uint8_t &frameInFlightIndex)
{
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
                                                const star::Handle &handle, const bool &isHighPriority)
{
    auto &container = bufferStorage.at(deviceID)->get(handle);

    // possible race condition....need to make sure the request on the secondary thread has been finished first before
    // replacing
    if (container.cpuWorkDoneByTransferThread.load())
    {
        container.cpuWorkDoneByTransferThread.wait(false);
    }
    container.cpuWorkDoneByTransferThread.store(false);

    managerWorker->add(container.cpuWorkDoneByTransferThread, container.resourceSemaphore, std::move(newRequest),
                       container.resource, isHighPriority);

    if (isHighPriority)
    {
        highPriorityRequestCompleteFlags.at(deviceID).insert(&container.cpuWorkDoneByTransferThread);
    }
}

bool star::ManagerRenderResource::isReady(const Handle &deviceID, const Handle &handle)
{
    switch (handle.getType())
    {
    case (star::Handle_Type::buffer): {
        return bufferStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread.load();
        break;
    }
    case (star::Handle_Type::texture): {
        return textureStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread.load();
        break;
    }
    default:
        throw std::runtime_error("Invalid type in handle");
        break;
    }

    return false;
}

void star::ManagerRenderResource::waitForReady(const Handle &deviceID, const Handle &handle)
{
    assert(deviceID.getType() == Handle_Type::device); 
    
    boost::atomic<bool> *fence = nullptr;

    switch (handle.getType())
    {
    case (star::Handle_Type::buffer):
        fence = &bufferStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread;
        break;
    case (star::Handle_Type::texture):
        fence = &bufferStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread;
        break;
    default:
        throw std::runtime_error("Invalid handle type");
        break;
    }

    assert(fence != nullptr && "Fence not valid or found");
    if (!fence->load())
    {
        fence->wait(false);
    }
}

star::StarBuffers::Buffer &star::ManagerRenderResource::getBuffer(const Handle &deviceID, const star::Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");
    const auto &container = bufferStorage.at(deviceID)->get(handle).resource;
    if (!container)
    {
        throw std::runtime_error("Buffer has not been created yet. It is either still waiting to be processed by "
                                 "transfer queues or has not been submitted yet. The latter is due to a blank request "
                                 "which is never updated by controllers");
    }

    return *container;
}

star::StarTextures::Texture &star::ManagerRenderResource::getTexture(const Handle &deviceID, const star::Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::texture && "Handle provided is not a texture handle");
    const auto &container = textureStorage.at(deviceID)->get(handle).resource;
    if (!container)
    {
        throw std::runtime_error("Texture has not been created yet. It must be waited on");
    }

    return *container;
}

void star::ManagerRenderResource::cleanup(const Handle &deviceID, core::device::StarDevice &device)
{
    bufferStorage.at(deviceID)->cleanupAll(&device);
    bufferStorage.at(deviceID).reset();
    textureStorage.at(deviceID)->cleanupAll(&device);
    textureStorage.at(deviceID).reset();

    devices.at(deviceID).reset();
    managerWorker.reset();
}