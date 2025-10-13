#include "managers/ManagerRenderResource.hpp"
#include "ManagerRenderResource.hpp"

std::unordered_map<star::core::device::DeviceID, std::shared_ptr<star::core::device::StarDevice>>
    star::ManagerRenderResource::devices =
        std::unordered_map<core::device::DeviceID, std::shared_ptr<star::core::device::StarDevice>>();
std::unordered_map<star::core::device::DeviceID, std::set<boost::atomic<bool> *>>
    star::ManagerRenderResource::highPriorityRequestCompleteFlags =
        std::unordered_map<star::core::device::DeviceID, std::set<boost::atomic<bool> *>>();
std::unordered_map<star::core::device::DeviceID,
                   std::unique_ptr<star::ManagerStorageContainer<star::ManagerRenderResource::FinalizedRenderRequest>>>
    star::ManagerRenderResource::bufferStorage = std::unordered_map<
        star::core::device::DeviceID,
        std::unique_ptr<star::ManagerStorageContainer<star::ManagerRenderResource::FinalizedRenderRequest>>>();

void star::ManagerRenderResource::init(core::device::DeviceID deviceID,
                                       std::shared_ptr<star::core::device::StarDevice> device,
                                       std::shared_ptr<star::job::TransferWorker> worker, const int &numFramesInFlight)
{
    {
        core::device::DeviceID tmpDevice = deviceID;
        devices.insert(std::make_pair<core::device::DeviceID, std::shared_ptr<core::device::StarDevice>>(
            std::move(tmpDevice), std::move(device)));
    }
    {
        core::device::DeviceID tmpDevice = deviceID;
        bufferStorage.insert(std::make_pair<core::device::DeviceID,
                                            std::unique_ptr<star::ManagerStorageContainer<FinalizedRenderRequest>>>(
            std::move(tmpDevice), std::make_unique<ManagerStorageContainer<FinalizedRenderRequest>>()));
    }
    {
        core::device::DeviceID tmpDevice = deviceID;

        highPriorityRequestCompleteFlags.insert(std::make_pair<core::device::DeviceID, std::set<boost::atomic<bool> *>>(
            std::move(tmpDevice), std::set<boost::atomic<bool> *>()));
    }

    managerWorker = worker;
}

star::Handle star::ManagerRenderResource::addRequest(
    const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore,
    std::unique_ptr<star::ManagerController::RenderResource::Buffer> newRequest, const bool &isHighPriority)
{
    assert(devices.contains(deviceID) && "Device has not been properly initialized");

    Handle newBufferHandle = Handle(Handle_Type::buffer);

    bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();

    auto newFull = std::make_unique<FinalizedRenderRequest>(std::move(resourceSemaphore), std::move(newRequest));
    if (isStatic)
    {
        newFull->cpuWorkDoneByTransferThread.store(false);
        managerWorker->add(newFull->cpuWorkDoneByTransferThread, newFull->resourceSemaphore,
                           newFull->bufferRequest->createTransferRequest(*devices.at(deviceID)), newFull->buffer,
                           isHighPriority);

        newFull->bufferRequest.release();
    }

    if (isHighPriority)
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull->cpuWorkDoneByTransferThread);

    bufferStorage.at(deviceID)->add(std::move(newFull), isStatic, newBufferHandle);

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(
    const core::device::DeviceID &deviceID, vk::Semaphore resourceSemaphore,
    std::unique_ptr<star::ManagerController::RenderResource::Texture> newRequest, const bool &isHighPriority)
{
    Handle newHandle = Handle(Handle_Type::texture);

    bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();

    auto newFull = std::make_unique<FinalizedRenderRequest>(std::move(resourceSemaphore), std::move(newRequest));

    if (isStatic)
    {
        newFull->cpuWorkDoneByTransferThread.store(false);
        managerWorker->add(newFull->cpuWorkDoneByTransferThread, newFull->resourceSemaphore,
                           newFull->textureRequest->createTransferRequest(*devices.at(deviceID)), newFull->texture,
                           isHighPriority);

        newFull->textureRequest.release();
    }

    if (isHighPriority)
        highPriorityRequestCompleteFlags.at(deviceID).insert(&newFull->cpuWorkDoneByTransferThread);

    bufferStorage.at(deviceID)->add(std::move(newFull), isStatic, newHandle);

    return newHandle;
}

void star::ManagerRenderResource::update(const core::device::DeviceID &deviceID, const int &frameInFlightIndex)
{
    // must wait for all high priority requests to complete

    // need to make sure any previous transfers have completed before submitting
    auto requestsToUpdate = std::vector<FinalizedRenderRequest *>();
    {
        // check if the request is still in processing by GPU -- wait if it is
        for (auto &request : bufferStorage.at(deviceID)->getDynamicMap())
        {
            std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage.at(deviceID)->at(request.second);

            // check the requests which need updating this frame
            if ((container->bufferRequest && !container->bufferRequest->isValid(frameInFlightIndex)) ||
                (container->textureRequest && !container->textureRequest->isValid(frameInFlightIndex)))
            {
                requestsToUpdate.push_back(container.get());
            }
        }
    }

    for (auto &request : requestsToUpdate)
    {
        if (!request->cpuWorkDoneByTransferThread.load())
        {
            request->cpuWorkDoneByTransferThread.wait(false);
        }
    }

    for (int i = 0; i < requestsToUpdate.size(); i++)
    {
        requestsToUpdate[i]->cpuWorkDoneByTransferThread.store(false);
        if (requestsToUpdate[i]->bufferRequest)
        {
            managerWorker->add(requestsToUpdate[i]->cpuWorkDoneByTransferThread, requestsToUpdate[i]->resourceSemaphore,
                               requestsToUpdate[i]->bufferRequest->createTransferRequest(*devices.at(deviceID)),
                               requestsToUpdate[i]->buffer, true);
        }
        else if (requestsToUpdate[i]->textureRequest)
        {
            managerWorker->add(requestsToUpdate[i]->cpuWorkDoneByTransferThread, requestsToUpdate[i]->resourceSemaphore,
                               requestsToUpdate[i]->textureRequest->createTransferRequest(*devices.at(deviceID)),
                               requestsToUpdate[i]->texture, true);
        }
    }

    //updated works MUST be submitted before command buffers using those semaphores can be used. 
    for (auto &request : requestsToUpdate)
    {
        if (!request->cpuWorkDoneByTransferThread.load())
        {
            request->cpuWorkDoneByTransferThread.wait(false);
        }
    }
}

void star::ManagerRenderResource::updateRequest(const core::device::DeviceID &deviceID,
                                                std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest,
                                                const star::Handle &handle, const bool &isHighPriority)
{
    // possible race condition....need to make sure the request on the secondary thread has been finished first before
    // replacing
    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage.at(deviceID)->get(handle);

    while (!container->cpuWorkDoneByTransferThread.load())
    {
        container->cpuWorkDoneByTransferThread.wait(false);
    }

    container->bufferRequest = std::move(newRequest);

    managerWorker->add(container->cpuWorkDoneByTransferThread, container->resourceSemaphore,
                       container->bufferRequest->createTransferRequest(*devices.at(deviceID)), container->buffer,
                       isHighPriority);

    highPriorityRequestCompleteFlags.at(deviceID).insert(&container->cpuWorkDoneByTransferThread);
}

bool star::ManagerRenderResource::isReady(const core::device::DeviceID &deviceID, const star::Handle &handle)
{
    // check the fence for the buffer request
    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage.at(deviceID)->get(handle);

    return container->cpuWorkDoneByTransferThread.load();
}

void star::ManagerRenderResource::waitForReady(const core::device::DeviceID &deviceID, const Handle &handle)
{
    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage.at(deviceID)->get(handle);

    bool value = container->cpuWorkDoneByTransferThread.load();
    if (!value)
    {
        container->cpuWorkDoneByTransferThread.wait(false);
    }
}

star::StarBuffers::Buffer &star::ManagerRenderResource::getBuffer(const core::device::DeviceID &deviceID,
                                                                  const star::Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage.at(deviceID)->get(handle);

    return *container->buffer;
}

star::StarTextures::Texture &star::ManagerRenderResource::getTexture(const core::device::DeviceID &deviceID,
                                                                     const star::Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::texture && "Handle provided is not a texture handle");

    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage.at(deviceID)->get(handle);

    return *container->texture;
}

void star::ManagerRenderResource::destroy(const core::device::DeviceID &deviceID, const star::Handle &handle)
{
    bufferStorage.at(deviceID)->destroy(handle);
}

void star::ManagerRenderResource::cleanup(const core::device::DeviceID &deviceID, core::device::StarDevice &device)
{
    bufferStorage.at(deviceID).reset();
    devices.at(deviceID).reset();

    managerWorker.reset();
}
star::ManagerRenderResource::FinalizedRenderRequest &star::ManagerRenderResource::get(
    const core::device::DeviceID &deviceID, const Handle &handle)
{
    return *bufferStorage.at(deviceID)->get(handle);
}