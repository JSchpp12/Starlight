#pragma once

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "StarCommandBuffer.hpp"
#include "Handle.hpp"

#include <vulkan/vulkan.hpp>
#include <stack>
#include <functional>
#include <unordered_map>

namespace star {
	enum class CommandBufferOrder {
		BEOFRE_RENDER_PASS,
		MAIN_RENDER_PASS,  //special only usable by main rendder pass
		AFTER_RENDER_PASS, 
		END_OF_FRAME
	};

	class ManagerCommandBuffer {
	public: 
		struct CommandBufferRequest {
			//when
			//type
			//callback function to record the buffer
			//is it dependent on another buffer to finish first?
			std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback;
			std::function<void(star::Handle)> promiseBufferHandleCallback;
			CommandBufferOrder order;
			star::Command_Buffer_Type type;
			bool willBeSubmittedEachFrame; 
			bool recordOnce; 
			std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback;
			std::optional<std::function<void(const int&)>> afterBufferSubmissionCallback;
			std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback;

			CommandBufferRequest(std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback, 
				std::function<void(star::Handle)> promiseBufferHandleCallback, const CommandBufferOrder& order, 
				const star::Command_Buffer_Type& type, const bool& willBeSubmittedEachFrame, 
				const bool& recordOnce, std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback, 
				std::optional<std::function<void(const int&)>> afterBufferSubmissionCallback, 
				std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback)
				: recordBufferCallback(recordBufferCallback), order(order), type(type), 
				willBeSubmittedEachFrame(willBeSubmittedEachFrame), recordOnce(recordOnce),
				beforeBufferSubmissionCallback(beforeBufferSubmissionCallback), 
				afterBufferSubmissionCallback(afterBufferSubmissionCallback),
				overrideBufferSubmissionCallback(overrideBufferSubmissionCallback){};
		};

		ManagerCommandBuffer(StarDevice& device, const int& numFramesInFlight)
			: device(device), numFramesInFlight(numFramesInFlight){};

		static void request(std::function<CommandBufferRequest(void)> request);

		static void submitDynamicBuffer(Handle bufferHandle);

		vk::Semaphore* getMainGraphicsDoneSemaphore(const int& frameIndexToBeDrawn);

		void update(const int& frameIndexToBeDrawn);

	private:
		struct CompleteRequest {
			std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback;
			std::unique_ptr<StarCommandBuffer> commandBuffer;
			Command_Buffer_Type type; 
			bool recordOnce; 

			std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback; 
			std::optional<std::function<void(const int&)>> afterBufferSubmissionCallback;
			std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback;

			CompleteRequest(std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback, std::unique_ptr<StarCommandBuffer> commandBuffer, 
				const Command_Buffer_Type& type, bool recordOnce,
				std::optional<std::function<void(const int&)>> beforeSubmissionCallback = std::optional<std::function<void(const int&)>>(),
				std::optional<std::function<void(const int&)>> afterSubmissionCallback = std::optional<std::function<void(const int&)>>(),
				std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback = std::optional<std::function<void(StarCommandBuffer&, const int&)>>())
				: recordBufferCallback(recordBufferCallback), commandBuffer(std::move(commandBuffer)),
				beforeBufferSubmissionCallback(beforeSubmissionCallback), 
				afterBufferSubmissionCallback(afterSubmissionCallback), 
				overrideBufferSubmissionCallback(overrideBufferSubmissionCallback),
				type(type), recordOnce(recordOnce) {};
		};

		static std::stack<std::function<CommandBufferRequest(void)>> newCommandBufferRequests;

		static std::stack<Handle> dynamicBuffersToSubmit; 

		StarDevice& device; 
		const int numFramesInFlight = 0; 
		std::vector<CompleteRequest> allBuffers; 
		std::vector<Handle> standardBuffers; 
		std::vector<Handle> dynamicBuffers; 
		CompleteRequest* mainGraphicsBuffer = nullptr;
		//std::unordered_map<star::Command_Buffer_Type, std::vector<CompleteRequest>> standardBuffers = std::unordered_map<star::Command_Buffer_Type, std::vector<CompleteRequest>>();	//these buffers will be submitted each frame
		//std::unordered_map<star::Command_Buffer_Type, std::vector<CompleteRequest>> dynamicBuffers = std::unordered_map<star::Command_Buffer_Type, std::vector<CompleteRequest>>();		//these buffers will be submitted when requested
		uint32_t numBuffers = 0; 

		void handleNewRequests();

		void submitCommandBuffers(const int& swapChainIndex);

		void recordBuffers(const int& swapChainIndex);
	};
}