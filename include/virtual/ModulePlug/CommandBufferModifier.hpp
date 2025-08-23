// #pragma once

// #include "Handle.hpp"
// #include "ManagerCommandBuffer.hpp"


// #include <functional>
// #include <memory>
// #include <optional>
// #include <stack>
// #include <vector>
// #include <vulkan/vulkan.hpp>


// namespace star
// {
// class CommandBufferModifier
// {
//   public:
//     CommandBufferModifier();

//     ~CommandBufferModifier() = default;

//     virtual void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) = 0;

//     void setBufferHandle(Handle bufferHandle);

//   protected:
//     void submitMyBuffer();

//     virtual Command_Buffer_Order getCommandBufferOrder() = 0;

//     virtual star::Command_Buffer_Order_Index getCommandBufferOrderIndex();

//     virtual Queue_Type getCommandBufferType() = 0;

//     virtual vk::PipelineStageFlags getWaitStages() = 0;

//     virtual bool getWillBeSubmittedEachFrame() = 0;

//     virtual bool getWillBeRecordedOnce() = 0;

//     virtual std::optional<std::function<void(const int &)>> getBeforeBufferSubmissionCallback();

//     virtual std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
//     getOverrideBufferSubmissionCallback();

//     ManagerCommandBuffer::Request getCommandBufferRequest();

//   private:
//     std::optional<Handle> bufferHandle;
// };
// } // namespace star