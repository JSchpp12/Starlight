#pragma once

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "StarCommandBuffer.hpp"
#include "Handle.hpp"

#include "internals/CommandBufferContainer.hpp"

#include <vulkan/vulkan.hpp>
#include <stack>
#include <functional>
#include <unordered_map>

namespace star {
	class ManagerCommandBuffer {
	public: 
		struct CommandBufferRequest {
			//when
			//type
			//callback function to record the buffer
			//is it dependent on another buffer to finish first?
			std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback;
			std::function<void(star::Handle)> promiseBufferHandleCallback;
			Command_Buffer_Order order;
			int orderIndex; 
			star::Command_Buffer_Type type;
			vk::PipelineStageFlags waitStage; 
			bool willBeSubmittedEachFrame; 
			bool recordOnce; 
			std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback;
			std::optional<std::function<void(StarCommandBuffer&, const int&, std::vector<vk::Semaphore>)>> overrideBufferSubmissionCallback;

			CommandBufferRequest(std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback, 
				std::function<void(star::Handle)> promiseBufferHandleCallback, 
				const Command_Buffer_Order& order,
				const int orderIndex,
				const star::Command_Buffer_Type& type, const vk::PipelineStageFlags& waitStage,
				const bool& willBeSubmittedEachFrame, 
				const bool& recordOnce, std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback, 
				std::optional<std::function<void(StarCommandBuffer&, const int&, std::vector<vk::Semaphore>)>> overrideBufferSubmissionCallback)
				: recordBufferCallback(recordBufferCallback), promiseBufferHandleCallback(promiseBufferHandleCallback),
				order(order), orderIndex(orderIndex),
				type(type), waitStage(waitStage),
				willBeSubmittedEachFrame(willBeSubmittedEachFrame), recordOnce(recordOnce),
				beforeBufferSubmissionCallback(beforeBufferSubmissionCallback), 
				overrideBufferSubmissionCallback(overrideBufferSubmissionCallback){};
		};

		ManagerCommandBuffer(StarDevice& device, const int& numFramesInFlight);

		~ManagerCommandBuffer(); 

		static void request(std::function<CommandBufferRequest(void)> request);

		static void submitDynamicBuffer(Handle bufferHandle);

		/// @brief Process and submit all command buffers
		/// @param frameIndexToBeDrawn 
		/// @return semaphore signaling completion of submission
		vk::Semaphore update(const int& frameIndexToBeDrawn);

	private:

		static std::stack<std::function<CommandBufferRequest(void)>> newCommandBufferRequests;

		static std::stack<Handle> dynamicBuffersToSubmit; 

		StarDevice& device; 
		const int numFramesInFlight = 0; 
		CommandBufferContainer buffers;
		std::unique_ptr<star::Handle> mainGraphicsBufferHandle = std::unique_ptr<star::Handle>(); 

		uint32_t numBuffers = 0; 

		void handleNewRequests();

		vk::Semaphore submitCommandBuffers(const int& swapChainIndex);

		void handleDynamicBufferRequests(); 
	};
}