#include "device/managers/ManagerCommandBuffer.hpp"

std::stack<star::Handle> star::core::device::manager::ManagerCommandBuffer::dynamicBuffersToSubmit =
    std::stack<star::Handle>();

star::core::device::manager::ManagerCommandBuffer::ManagerCommandBuffer(StarDevice &device,
                                                                        const uint8_t &numFramesInFlight)
    : numFramesInFlight(numFramesInFlight), buffers(numFramesInFlight, device)
{
}

void star::core::device::manager::ManagerCommandBuffer::cleanup(StarDevice &device)
{
    buffers.cleanup(device);
}

star::Handle star::core::device::manager::ManagerCommandBuffer::submit(StarDevice &device,
                                                                       const uint64_t &currentFrameIndex,
                                                                       Request request)
{

    star::Handle newHandle = this->buffers.add(
        std::make_unique<CommandBufferContainer::CompleteRequest>(
            request.recordBufferCallback,
            std::make_unique<StarCommandBuffer>(device.getVulkanDevice(), this->numFramesInFlight,
                                                device.getCommandPool(request.type), request.type,
                                                !request.overrideBufferSubmissionCallback.has_value(), true),
            request.type, request.recordOnce, request.waitStage, request.order, request.beforeBufferSubmissionCallback,
            request.overrideBufferSubmissionCallback),
        request.willBeSubmittedEachFrame, request.type, request.order,
        static_cast<Command_Buffer_Order_Index>(request.orderIndex));

    if (request.type == Queue_Type::Tgraphics && request.order == Command_Buffer_Order::main_render_pass)
        this->mainGraphicsBufferHandle = std::make_unique<Handle>(newHandle);

    //todo: REMOVE THIS! OR move it to the update function
    if (request.recordOnce)
    {
        for (int i = 0; i < this->numFramesInFlight; i++)
        {
            this->buffers.get(newHandle).commandBuffer->begin(i);
            request.recordBufferCallback(this->buffers.get(newHandle).commandBuffer->buffer(i), i, currentFrameIndex);
            this->buffers.get(newHandle).commandBuffer->buffer(i).end();
        }
    }

    return newHandle;
}

void star::core::device::manager::ManagerCommandBuffer::submitDynamicBuffer(Handle bufferHandle)
{
    ManagerCommandBuffer::dynamicBuffersToSubmit.push(bufferHandle);
}

star::CommandBufferContainer::CompleteRequest &star::core::device::manager::ManagerCommandBuffer::get(
    const Handle &handle)
{
    return buffers.get(handle);
}

vk::Semaphore star::core::device::manager::ManagerCommandBuffer::update(StarDevice &device,
                                                                        const uint8_t &frameInFlight,
                                                                        const uint64_t &currentFrameIndex)
{
    handleDynamicBufferRequests();

    return submitCommandBuffers(device, frameInFlight, currentFrameIndex);
}

vk::Semaphore star::core::device::manager::ManagerCommandBuffer::submitCommandBuffers(StarDevice &device,
                                                                                      const uint8_t &swapChainIndex,
                                                                                      const uint64_t &currentFrameIndex)
{
    // determine the order of buffers to execute
    assert(this->mainGraphicsBufferHandle && "No main graphics buffer set -- cannot happen");

    // submit before
    std::vector<vk::Semaphore> beforeSemaphores = this->buffers.submitGroupWhenReady(
        device, Command_Buffer_Order::before_render_pass, swapChainIndex, currentFrameIndex);

    // need to submit each group of buffers depending on the queue family they are in
    CommandBufferContainer::CompleteRequest &mainGraphicsBuffer = this->buffers.get(*this->mainGraphicsBufferHandle);

    if (mainGraphicsBuffer.beforeBufferSubmissionCallback.has_value())
        mainGraphicsBuffer.beforeBufferSubmissionCallback.value()(swapChainIndex);

    if (!mainGraphicsBuffer.recordOnce)
    {
        mainGraphicsBuffer.commandBuffer->begin(swapChainIndex);
        mainGraphicsBuffer.recordBufferCallback(mainGraphicsBuffer.commandBuffer->buffer(swapChainIndex),
                                                swapChainIndex, currentFrameIndex);
        mainGraphicsBuffer.commandBuffer->buffer(swapChainIndex).end();
    }

    auto mainGraphicsSemaphore = mainGraphicsBuffer.submitCommandBuffer(device, swapChainIndex, &beforeSemaphores);

    assert(mainGraphicsSemaphore && "The main graphics complete semaphore is not valid. This might happen if the "
                                    "override function does not return a valid semaphore");

    std::vector<vk::Semaphore> waitSemaphores = {mainGraphicsSemaphore};

    std::vector<vk::Semaphore> finalSubmissionSemaphores = this->buffers.submitGroupWhenReady(
        device, Command_Buffer_Order::end_of_frame, swapChainIndex, currentFrameIndex, &waitSemaphores);

    if (finalSubmissionSemaphores.empty())
        return mainGraphicsSemaphore;
    else
        return finalSubmissionSemaphores[0];
}

void star::core::device::manager::ManagerCommandBuffer::submitPostPresentationCommands(
    StarDevice &device, const uint8_t &frameInFlightIndex, const uint64_t &currentFrameIndex,
    vk::Semaphore presentationImageReadySemaphore)
{
    auto ready = std::vector<vk::Semaphore>{presentationImageReadySemaphore};
    this->buffers.submitGroupWhenReady(device, Command_Buffer_Order::after_presentation, frameInFlightIndex,
                                       currentFrameIndex, &ready);
}

void star::core::device::manager::ManagerCommandBuffer::handleDynamicBufferRequests()
{
    while (!ManagerCommandBuffer::dynamicBuffersToSubmit.empty())
    {
        Handle dynamicBufferRequest = ManagerCommandBuffer::dynamicBuffersToSubmit.top();

        this->buffers.setToSubmitThisBuffer(dynamicBufferRequest.getID());

        ManagerCommandBuffer::dynamicBuffersToSubmit.pop();
    }
}
