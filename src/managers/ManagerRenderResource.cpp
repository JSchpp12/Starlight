#include "managers/ManagerRenderResource.hpp"

auto star::ManagerRenderResource::devices =
    std::unordered_map<star::Handle, std::shared_ptr<star::core::device::StarDevice>, star::HandleHash>();
auto star::ManagerRenderResource::highPriorityRequestCompleteFlags =
    std::unordered_map<star::Handle, std::set<boost::atomic<bool> *>, star::HandleHash>();
auto star::ManagerRenderResource::bufferStorage = std::unordered_map<
    star::Handle,
    std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>, 1500>>,
    star::HandleHash>();
auto star::ManagerRenderResource::textureStorage = std::unordered_map<
    star::Handle,
    std::unique_ptr<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>, 1000>>,
    star::HandleHash>();

void star::ManagerRenderResource::init(const Handle &deviceID, std::shared_ptr<star::core::device::StarDevice> device,
                                       std::shared_ptr<star::job::TransferWorker> worker, const int &numFramesInFlight)
{
    devices.insert(std::make_pair(deviceID, std::move(device)));
    bufferStorage.insert(std::make_pair(
        deviceID,
        std::make_unique<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarBuffers::Buffer>, 1500>>(
            common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::BufferTypeName))));
    textureStorage.insert(std::make_pair(
        deviceID,
        std::make_unique<core::ManagedHandleContainer<FinalizedResourceRequest<star::StarTextures::Texture>, 1000>>(
            common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::TextureTypeName))));

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

    if (!container.cpuWorkDoneByTransferThread.load())
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
        fence = &bufferStorage.at(deviceID)->get(handle).cpuWorkDoneByTransferThread;
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
            throw std::runtime_error("Unknown error has occurred during getBuffer. This can happen if the transfer "
                                     "flag is not properly reset");
        }
    }

    assert(container.resource && "Resource should always exist before returning");

    return *container.resource;
}

star::StarTextures::Texture &star::ManagerRenderResource::getTexture(const Handle &deviceID, const star::Handle &handle)
{
    assert(handle.getType() == common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                   common::special_types::TextureTypeName) &&
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

    devices.at(deviceID).reset();
    managerWorker.reset();
}