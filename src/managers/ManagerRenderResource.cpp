#include "ManagerRenderResource.hpp"

std::set<boost::atomic<bool> *> star::ManagerRenderResource::highPriorityRequestCompleteFlags =
    std::set<boost::atomic<bool> *>();
std::unique_ptr<star::ManagerStorageContainer<star::ManagerRenderResource::FinalizedRenderRequest>>
    star::ManagerRenderResource::bufferStorage =
        std::make_unique<star::ManagerStorageContainer<star::ManagerRenderResource::FinalizedRenderRequest>>();

void star::ManagerRenderResource::init(star::StarDevice &device, star::TransferWorker &worker,
                                       const int &numFramesInFlight)
{
    assert(managerDevice == nullptr && "Init function should only be called once");

    managerDevice = &device;
    managerWorker = &worker;
}

star::Handle star::ManagerRenderResource::addRequest(
    std::unique_ptr<star::ManagerController::RenderResource::Buffer> newRequest, const bool &isHighPriority)
{
    Handle newBufferHandle = Handle(Handle_Type::buffer);

    bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();

    auto newFull = std::make_unique<FinalizedRenderRequest>(std::move(newRequest));
    if (isStatic)
    {
        newFull->cpuWorkDoneByTransferThread.store(false);
        managerWorker->add(newFull->cpuWorkDoneByTransferThread,
                           newFull->bufferRequest->createTransferRequest(*managerDevice), newFull->buffer,
                           isHighPriority);
        newFull->bufferRequest.release();
    }

    if (isHighPriority)
        highPriorityRequestCompleteFlags.insert(&newFull->cpuWorkDoneByTransferThread);

    bufferStorage->add(std::move(newFull), isStatic, newBufferHandle);

    return newBufferHandle;
}

star::Handle star::ManagerRenderResource::addRequest(
    std::unique_ptr<star::ManagerController::RenderResource::Texture> newRequest, const bool &isHighPriority)
{
    Handle newHandle = Handle(Handle_Type::texture);

    bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();

    auto newFull = std::make_unique<FinalizedRenderRequest>(std::move(newRequest));

    if (isStatic)
    {
        newFull->cpuWorkDoneByTransferThread.store(false);
        managerWorker->add(newFull->cpuWorkDoneByTransferThread,
                           newFull->textureRequest->createTransferRequest(*managerDevice), newFull->texture,
                           isHighPriority);

        newFull->textureRequest.release();
    }

    if (isHighPriority)
        highPriorityRequestCompleteFlags.insert(&newFull->cpuWorkDoneByTransferThread);

    bufferStorage->add(std::move(newFull), isStatic, newHandle);

    return newHandle;
}

void star::ManagerRenderResource::update(const int &frameInFlightIndex)
{
    // must wait for all high priority requests to complete

    // need to make sure any previous transfers have completed before submitting
    std::vector<FinalizedRenderRequest *> requestsToUpdate = std::vector<FinalizedRenderRequest *>();
    {
        // check if the request is still in processing by GPU -- wait if it is
        for (auto &request : bufferStorage->getDynamicMap())
        {
            std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage->at(request.second);

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
            managerWorker->add(requestsToUpdate[i]->cpuWorkDoneByTransferThread,
                               requestsToUpdate[i]->bufferRequest->createTransferRequest(*managerDevice),
                               requestsToUpdate[i]->buffer, true);
        }
        else if (requestsToUpdate[i]->textureRequest)
        {
            managerWorker->add(requestsToUpdate[i]->cpuWorkDoneByTransferThread,
                               requestsToUpdate[i]->textureRequest->createTransferRequest(*managerDevice),
                               requestsToUpdate[i]->texture, true);
        }
    }
}

void star::ManagerRenderResource::updateRequest(std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest,
                                                const star::Handle &handle, const bool &isHighPriority)
{
    // possible race condition....need to make sure the request on the secondary thread has been finished first before
    // replacing
    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage->get(handle);

    while (!container->cpuWorkDoneByTransferThread.load())
    {
        container->cpuWorkDoneByTransferThread.wait(false);
    }

    container->bufferRequest = std::move(newRequest);

    managerWorker->add(container->cpuWorkDoneByTransferThread,
                       std::move(container->bufferRequest->createTransferRequest(*managerDevice)), container->buffer,
                       isHighPriority);

    highPriorityRequestCompleteFlags.insert(&container->cpuWorkDoneByTransferThread);
}

bool star::ManagerRenderResource::isReady(const star::Handle &handle)
{
    // check the fence for the buffer request
    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage->get(handle);

    return container->cpuWorkDoneByTransferThread.load();
}

void star::ManagerRenderResource::waitForReady(const Handle &handle)
{
    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage->get(handle);

    bool value = container->cpuWorkDoneByTransferThread.load();
    if (!value)
    {
        container->cpuWorkDoneByTransferThread.wait(false);
    }
}

star::StarBuffer &star::ManagerRenderResource::getBuffer(const star::Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage->get(handle);

    return *container->buffer;
}

star::StarTexture &star::ManagerRenderResource::getTexture(const star::Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::texture && "Handle provided is not a texture handle");

    std::unique_ptr<FinalizedRenderRequest> &container = bufferStorage->get(handle);

    return *container->texture;
}

void star::ManagerRenderResource::destroy(const star::Handle &handle)
{
    bufferStorage->destroy(handle);
}

void star::ManagerRenderResource::cleanup(StarDevice &device)
{
    bufferStorage.reset();
}