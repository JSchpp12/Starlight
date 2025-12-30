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
}

star::Handle star::core::device::manager::ManagerCommandBuffer::submit(StarDevice &device,
                                                                       const uint64_t &currentFrameIndex,
                                                                       Request request)
{

    star::Handle newHandle = this->buffers.add(
        std::make_shared<CommandBufferContainer::CompleteRequest>(
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

    // todo: REMOVE THIS! OR move it to the update function
    if (request.recordOnce)
    {
        throw std::runtime_error("Single time recording is not supported at present");
    }
    // if (request.recordOnce)
    // {
    //     for (int i = 0; i < this->numFramesInFlight; i++)
    //     {
    //         this->buffers.get(newHandle).commandBuffer->begin(i);
    //         request.recordBufferCallback(this->buffers.get(newHandle).commandBuffer->buffer(i), frameTracker,
    //         currentFrameIndex); this->buffers.get(newHandle).commandBuffer->buffer(i).end();
    //     }
    // }

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
                                                                        const common::FrameTracker &frameTracker)
{
    handleDynamicBufferRequests();

    return submitCommandBuffers(device, frameTracker, frameTracker.getCurrent().getGlobalFrameCounter());
}

vk::Semaphore star::core::device::manager::ManagerCommandBuffer::submitCommandBuffers(
    StarDevice &device, const common::FrameTracker &frameTracker, const uint64_t &currentFrameIndex)
{
    // determine the order of buffers to execute
    assert(this->mainGraphicsBufferHandle && "No main graphics buffer set -- not a valid rendering setup");

    // submit before
    std::vector<vk::Semaphore> beforeSemaphores = this->buffers.submitGroupWhenReady(
        device, Command_Buffer_Order::before_render_pass, frameTracker, currentFrameIndex);

    // need to submit each group of buffers depending on the queue family they are in
    CommandBufferContainer::CompleteRequest &mainGraphicsBuffer = this->buffers.get(*this->mainGraphicsBufferHandle);

    if (mainGraphicsBuffer.beforeBufferSubmissionCallback.has_value())
        mainGraphicsBuffer.beforeBufferSubmissionCallback.value()(frameTracker.getCurrent().getFrameInFlightIndex());

    if (!mainGraphicsBuffer.recordOnce)
    {
        mainGraphicsBuffer.commandBuffer->begin(frameTracker.getCurrent().getFrameInFlightIndex());
        mainGraphicsBuffer.recordBufferCallback(
            mainGraphicsBuffer.commandBuffer->buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker,
            currentFrameIndex);
        mainGraphicsBuffer.commandBuffer->buffer(frameTracker.getCurrent().getFrameInFlightIndex()).end();
    }

    auto mainGraphicsSemaphore = mainGraphicsBuffer.submitCommandBuffer(device, frameTracker, &beforeSemaphores);

    assert(mainGraphicsSemaphore && "The main graphics complete semaphore is not valid. This might happen if the "
                                    "override function does not return a valid semaphore");

    std::vector<vk::Semaphore> waitSemaphores = {mainGraphicsSemaphore};

    std::vector<vk::Semaphore> finalSubmissionSemaphores = this->buffers.submitGroupWhenReady(
        device, Command_Buffer_Order::end_of_frame, frameTracker, currentFrameIndex, &waitSemaphores);

    if (!finalSubmissionSemaphores.empty())
    {
        return finalSubmissionSemaphores[0];
    }
    return mainGraphicsSemaphore;
}

// void star::core::device::manager::ManagerCommandBuffer::submitPostPresentationCommands(
//     StarDevice &device, const uint8_t &frameInFlightIndex, const uint64_t &currentFrameIndex,
//     vk::Semaphore presentationImageReadySemaphore)
// {
//     auto ready = std::vector<vk::Semaphore>{presentationImageReadySemaphore};
//     this->buffers.submitGroupWhenReady(device, Command_Buffer_Order::after_presentation, frameInFlightIndex,
//                                        currentFrameIndex, &ready);
// }

void star::core::device::manager::ManagerCommandBuffer::handleDynamicBufferRequests()
{
    while (!ManagerCommandBuffer::dynamicBuffersToSubmit.empty())
    {
        Handle dynamicBufferRequest = ManagerCommandBuffer::dynamicBuffersToSubmit.top();

        this->buffers.setToSubmitThisBuffer(dynamicBufferRequest);

        ManagerCommandBuffer::dynamicBuffersToSubmit.pop();
    }
}
