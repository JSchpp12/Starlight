#pragma once

#include "ManagerCommandBuffer.hpp"
#include "Handle.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>
#include <stack>
#include <memory>
#include <functional>

namespace star {
	class CommandBufferModifier {
	public:
		CommandBufferModifier();

		~CommandBufferModifier() = default; 

		virtual void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) = 0; 

		void setBufferHandle(Handle bufferHandle);
	protected: 
		void submitMyBuffer();

		virtual CommandBufferOrder getCommandBufferOrder() = 0; 

		virtual Command_Buffer_Type getCommandBufferType() = 0;

		virtual vk::PipelineStageFlags getWaitStages() = 0; 

		virtual bool getWillBeSubmittedEachFrame() = 0;

		virtual bool getWillBeRecordedOnce() = 0;

		/// @brief A callback can be provided to be called after the buffer has been submitted
		/// @return The function to call
		virtual std::optional<std::function<void(const int&)>> getAfterBufferSubmissionCallback(); 

		virtual std::optional<std::function<void(const int&)>> getBeforeBufferSubmissionCallback();

		virtual std::optional<std::function<void(StarCommandBuffer&, const int&)>> getOverrideBufferSubmissionCallback();

		ManagerCommandBuffer::CommandBufferRequest getCommandBufferRequest(); 
	private:
		std::optional<Handle> bufferHandle; 
	};
}