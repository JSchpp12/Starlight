#include "ManagerCommandBuffer.hpp"

std::stack<std::function<star::ManagerCommandBuffer::CommandBufferRequest(void)>>
    star::ManagerCommandBuffer::newCommandBufferRequests =
        std::stack<std::function<star::ManagerCommandBuffer::CommandBufferRequest(void)>>();

std::stack<star::Handle> star::ManagerCommandBuffer::dynamicBuffersToSubmit = std::stack<star::Handle>();

star::ManagerCommandBuffer::ManagerCommandBuffer(StarDevice &device, const int &numFramesInFlight)
    : device(device), numFramesInFlight(numFramesInFlight), buffers(device, numFramesInFlight)
{
}

star::ManagerCommandBuffer::~ManagerCommandBuffer()
{
}

void star::ManagerCommandBuffer::request(std::function<CommandBufferRequest(void)> request)
{
    ManagerCommandBuffer::newCommandBufferRequests.push(request);
}

void star::ManagerCommandBuffer::submitDynamicBuffer(Handle bufferHandle)
{
    ManagerCommandBuffer::dynamicBuffersToSubmit.push(bufferHandle);
}

vk::Semaphore star::ManagerCommandBuffer::update(const int &frameIndexToBeDrawn)
{
    handleNewRequests();

    handleDynamicBufferRequests();

    return submitCommandBuffers(frameIndexToBeDrawn);
}

void star::ManagerCommandBuffer::handleNewRequests()
{
    while (!ManagerCommandBuffer::newCommandBufferRequests.empty())
    {
        std::function<CommandBufferRequest(void)> requestFunction =
            ManagerCommandBuffer::newCommandBufferRequests.top();
        auto request = requestFunction();

        star::Handle newHandle = this->buffers.add(
            std::make_unique<CommandBufferContainer::CompleteRequest>(
                request.recordBufferCallback,
                std::make_unique<StarCommandBuffer>(this->device.getDevice(), this->numFramesInFlight,
                                                    this->device.getCommandPool(request.type), request.type,
                                                    !request.overrideBufferSubmissionCallback.has_value(), true),
                request.type, request.recordOnce, request.waitStage, request.order,
                request.beforeBufferSubmissionCallback, request.overrideBufferSubmissionCallback),
            request.willBeSubmittedEachFrame, request.type, request.order,
            static_cast<Command_Buffer_Order_Index>(request.orderIndex));

        request.promiseBufferHandleCallback(newHandle);

        if (request.type == Queue_Type::Tgraphics && request.order == Command_Buffer_Order::main_render_pass)
            this->mainGraphicsBufferHandle = std::make_unique<Handle>(newHandle);

        if (request.recordOnce)
        {
            for (int i = 0; i < this->numFramesInFlight; i++)
            {
                this->buffers.getBuffer(newHandle).commandBuffer->begin(i);
                request.recordBufferCallback(this->buffers.getBuffer(newHandle).commandBuffer->buffer(i), i);
                this->buffers.getBuffer(newHandle).commandBuffer->buffer(i).end();
            }
        }

        ManagerCommandBuffer::newCommandBufferRequests.pop();
    }
}

void star::ManagerCommandBuffer::callPreRecordFunctions(const uint8_t &frameInFlightIndex)
{
}

vk::Semaphore star::ManagerCommandBuffer::submitCommandBuffers(const uint32_t &swapChainIndex)
{
    // determine the order of buffers to execute
    assert(this->mainGraphicsBufferHandle && "No main graphics buffer set -- cannot happen");

    // submit before
    std::vector<vk::Semaphore> beforeSemaphores =
        this->buffers.submitGroupWhenReady(Command_Buffer_Order::before_render_pass, swapChainIndex);

    // need to submit each group of buffers depending on the queue family they are in
    CommandBufferContainer::CompleteRequest &mainGraphicsBuffer =
        this->buffers.getBuffer(*this->mainGraphicsBufferHandle);

    if (mainGraphicsBuffer.beforeBufferSubmissionCallback.has_value())
        mainGraphicsBuffer.beforeBufferSubmissionCallback.value()(swapChainIndex);

    if (!mainGraphicsBuffer.recordOnce)
    {
        mainGraphicsBuffer.commandBuffer->begin(swapChainIndex);
        mainGraphicsBuffer.recordBufferCallback(mainGraphicsBuffer.commandBuffer->buffer(swapChainIndex),
                                                swapChainIndex);
        mainGraphicsBuffer.commandBuffer->buffer(swapChainIndex).end();
    }

    vk::Semaphore mainGraphicsSemaphore = vk::Semaphore();
    if (mainGraphicsBuffer.overrideBufferSubmissionCallback.has_value())
    {
        mainGraphicsSemaphore = mainGraphicsBuffer.overrideBufferSubmissionCallback.value()(
            *mainGraphicsBuffer.commandBuffer, swapChainIndex, beforeSemaphores);
    }
    else
    {
        mainGraphicsBuffer.commandBuffer->submit(
            swapChainIndex, this->device.getQueueFamily(mainGraphicsBuffer.commandBuffer->getType())
                                .getQueues()
                                .at(0)
                                .getVulkanQueue());
        mainGraphicsSemaphore = mainGraphicsBuffer.commandBuffer->getCompleteSemaphores().at(swapChainIndex);
    }

    assert(mainGraphicsSemaphore && "The main graphics complete semaphore is not valid. This might happen if the "
                                    "override function does not return a valid semaphore");

    std::vector<vk::Semaphore> waitSemaphores = {mainGraphicsSemaphore};

    std::vector<vk::Semaphore> finalSubmissionSemaphores =
        this->buffers.submitGroupWhenReady(Command_Buffer_Order::end_of_frame, swapChainIndex, &waitSemaphores);

    if (finalSubmissionSemaphores.empty())
        return mainGraphicsSemaphore;
    else
        return finalSubmissionSemaphores[0];
}

void star::ManagerCommandBuffer::handleDynamicBufferRequests()
{
    while (!ManagerCommandBuffer::dynamicBuffersToSubmit.empty())
    {
        Handle dynamicBufferRequest = ManagerCommandBuffer::dynamicBuffersToSubmit.top();

        this->buffers.setToSubmitThisBuffer(dynamicBufferRequest.getID());

        ManagerCommandBuffer::dynamicBuffersToSubmit.pop();
    }
}
