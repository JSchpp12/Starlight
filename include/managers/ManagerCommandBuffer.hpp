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
			star::Command_Buffer_Type type;
			vk::PipelineStageFlags waitStage; 
			bool willBeSubmittedEachFrame; 
			bool recordOnce; 
			std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback;
			std::optional<std::function<void(const int&)>> afterBufferSubmissionCallback;
			std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback;

			CommandBufferRequest(std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback, 
				std::function<void(star::Handle)> promiseBufferHandleCallback, 
				const Command_Buffer_Order& order,
				const star::Command_Buffer_Type& type, const vk::PipelineStageFlags& waitStage,
				const bool& willBeSubmittedEachFrame, 
				const bool& recordOnce, std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback, 
				std::optional<std::function<void(const int&)>> afterBufferSubmissionCallback, 
				std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback)
				: recordBufferCallback(recordBufferCallback), promiseBufferHandleCallback(promiseBufferHandleCallback),
				order(order), 
				type(type), waitStage(waitStage),
				willBeSubmittedEachFrame(willBeSubmittedEachFrame), recordOnce(recordOnce),
				beforeBufferSubmissionCallback(beforeBufferSubmissionCallback), 
				afterBufferSubmissionCallback(afterBufferSubmissionCallback),
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
		struct CompleteRequest {
			std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback;
			std::unique_ptr<StarCommandBuffer> commandBuffer;
			Command_Buffer_Type type; 
			Command_Buffer_Order order;
			vk::PipelineStageFlags waitStage; 
			bool recordOnce; 

			std::optional<std::function<void(const int&)>> beforeBufferSubmissionCallback; 
			std::optional<std::function<void(const int&)>> afterBufferSubmissionCallback;
			std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback;

			CompleteRequest(std::function<void(vk::CommandBuffer&, const int&)> recordBufferCallback, std::unique_ptr<StarCommandBuffer> commandBuffer, 
				const Command_Buffer_Type& type, const bool& recordOnce, const vk::PipelineStageFlags& waitStage, 
				const Command_Buffer_Order& order,
				std::optional<std::function<void(const int&)>> beforeSubmissionCallback = std::optional<std::function<void(const int&)>>(),
				std::optional<std::function<void(const int&)>> afterSubmissionCallback = std::optional<std::function<void(const int&)>>(),
				std::optional<std::function<void(StarCommandBuffer&, const int&)>> overrideBufferSubmissionCallback = std::optional<std::function<void(StarCommandBuffer&, const int&)>>())
				: recordBufferCallback(recordBufferCallback), commandBuffer(std::move(commandBuffer)),
				order(order), waitStage(waitStage), 
				beforeBufferSubmissionCallback(beforeSubmissionCallback), 
				afterBufferSubmissionCallback(afterSubmissionCallback), 
				overrideBufferSubmissionCallback(overrideBufferSubmissionCallback),
				type(type), recordOnce(recordOnce) {};
		};

		class BufferContainer {
		public:
			BufferContainer(StarDevice& device, const int& numImagesInFlight)
				: device(device)
			{
				for (int i = Command_Buffer_Order::before_render_pass; i <= Command_Buffer_Order::end_of_frame; i++) {
					for (int j = Command_Buffer_Type::Tgraphics; j <= Command_Buffer_Order::end_of_frame; j++) {
						this->bufferGroups[static_cast<star::Command_Buffer_Order>(i)] = std::make_unique<GenericBufferGroupInfo>(device, numImagesInFlight);
					}
				}
			};

			~BufferContainer() = default; 

			std::vector<vk::Semaphore> submitGroupWhenReady(const star::Command_Buffer_Order& order, const int& swapChainIndex, std::vector<vk::Semaphore>* waitSemaphores = nullptr, std::vector<vk::PipelineStageFlags>* waitPoints = nullptr) {
				std::vector<vk::Semaphore> semaphores;
				std::vector<std::pair<CompleteRequest&, vk::Fence>> buffersToSubmitWithFences = std::vector<std::pair<CompleteRequest&, vk::Fence>>();

				for (int type = star::Command_Buffer_Type::Tgraphics; type != star::Command_Buffer_Type::Tcompute; type++) {
					std::vector<std::reference_wrapper<CompleteRequest>> buffersToSubmit = this->getAllBuffersOfTypeAndOrderReadyToSubmit(Command_Buffer_Order::end_of_frame, static_cast<star::Command_Buffer_Type>(type), true);

					if (!buffersToSubmit.empty()) {
						waitUntilOrderGroupReady(swapChainIndex, order, static_cast<Command_Buffer_Type>(type));

						semaphores.push_back(this->bufferGroups[order]->semaphores[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex));

						//before submission
						for (CompleteRequest& buffer : buffersToSubmit) {
							if (buffer.beforeBufferSubmissionCallback.has_value())
								buffer.beforeBufferSubmissionCallback.value()(swapChainIndex);

							if (!buffer.recordOnce) {
								buffer.commandBuffer->begin(swapChainIndex);
								buffer.recordBufferCallback(buffer.commandBuffer->buffer(swapChainIndex), swapChainIndex);
								buffer.commandBuffer->buffer(swapChainIndex).end();
							}
						}

						//submit
						{
							std::vector<vk::CommandBuffer> buffers = std::vector<vk::CommandBuffer>();
							for (CompleteRequest& buffer : buffersToSubmit) {
								buffers.push_back(buffer.commandBuffer->buffer(swapChainIndex));
							}

							vk::SubmitInfo submitInfo{};

							if (waitSemaphores != nullptr && waitPoints != nullptr) {
								submitInfo.waitSemaphoreCount = waitSemaphores->size();
								submitInfo.pWaitSemaphores = waitSemaphores->data();
								submitInfo.pWaitDstStageMask = waitPoints->data();
							}

							submitInfo.signalSemaphoreCount = 1;
							submitInfo.pSignalSemaphores = &this->bufferGroups[order]->semaphores[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex);

							//record for later
							semaphores.push_back(this->bufferGroups[order]->semaphores[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex));

							submitInfo.pCommandBuffers = buffers.data(); 
							submitInfo.commandBufferCount = buffers.size();

							std::unique_ptr<vk::Result> commandResult = std::unique_ptr<vk::Result>(); 
							vk::Fence workingFence = this->bufferGroups[order]->fences[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex);
							switch (type) {
								case(Command_Buffer_Type::Tgraphics): 
									commandResult = std::make_unique<vk::Result>(this->device.getGraphicsQueue().submit(1, &submitInfo, workingFence));
									break;
								case(Command_Buffer_Type::Tcompute): 
									commandResult = std::make_unique<vk::Result>(this->device.getComputeQueue().submit(1, &submitInfo, workingFence));
									break;
								case(Command_Buffer_Type::Ttransfer):
									commandResult = std::make_unique<vk::Result>(this->device.getTransferQueue().submit(1, &submitInfo, workingFence));
									break;					   
								default:
									break; 
							}

							assert(commandResult != nullptr && "Invalid command buffer type");

							if (*commandResult.get() != vk::Result::eSuccess) {
								throw std::runtime_error("Failed to submit command buffer");
							}

							for (auto& buffer : buffersToSubmit) {
								buffersToSubmitWithFences.push_back(std::make_pair(buffer, workingFence));
							}
						}
					}
				}

				//after submit
				for (auto& bufferAndFence : buffersToSubmitWithFences) {
					if (bufferAndFence.first.afterBufferSubmissionCallback.has_value()) {
						this->device.getDevice().waitForFences(bufferAndFence.second, VK_TRUE, UINT64_MAX);
						bufferAndFence.first.afterBufferSubmissionCallback.value()(swapChainIndex);
					}
				}

				return semaphores; 
			}

			star::Handle add(std::unique_ptr<CompleteRequest> newRequest, const bool& willBeSubmittedEachFrame, const star::Command_Buffer_Type& type, const star::Command_Buffer_Order& order) {
				star::Handle newHandle = star::Handle(this->allBuffers.size(), star::Handle_Type::buffer);
				const int bufferIndex = this->allBuffers.size();

				this->allBuffers.push_back(std::move(newRequest));
				this->bufferSubmissionStatus.push_back(willBeSubmittedEachFrame ? 2 : 0);
				
				this->bufferGroups[order]->bufferOrderGroupsIndices[type].push_back(bufferIndex);

				return newHandle; 
			}

			bool shouldSubmitThisBuffer(const size_t& bufferIndex) {
				assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

				return this->bufferSubmissionStatus[bufferIndex] & 1 || this->bufferSubmissionStatus[bufferIndex] & 2;
			}

			void resetThisBufferStatus(const size_t& bufferIndex) {
				assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

				if (this->bufferSubmissionStatus[bufferIndex] & 1)
					this->bufferSubmissionStatus[bufferIndex] = 0; 
			}

			void setToSubmitThisBuffer(const size_t& bufferIndex) {
				assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

				this->bufferSubmissionStatus[bufferIndex] = 1; 
			}

			CompleteRequest& getBuffer(const star::Handle& bufferHandle) {
				assert(bufferHandle.id < this->allBuffers.size() && "Requested index does not exist"); 
				 
				return *this->allBuffers[bufferHandle.id];
			}

			size_t size() {
				return this->allBuffers.size(); 
			}

		private:
			struct GenericBufferGroupInfo {
				std::unordered_map<star::Command_Buffer_Type, std::vector<size_t>> bufferOrderGroupsIndices;
				std::unordered_map<star::Command_Buffer_Type, std::vector<vk::Semaphore>> semaphores;
				std::unordered_map<star::Command_Buffer_Type, std::vector<vk::Fence>> fences; 

				GenericBufferGroupInfo(StarDevice& device, const int& numFramesInFlight)
					: device(device) {
					vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
					semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

					vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo();
					fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
					fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
					for (int j = 0; j < numFramesInFlight; j++) {
						for (int i = star::Command_Buffer_Type::Tgraphics; i != star::Command_Buffer_Type::Tcompute; i++) {
							vk::Fence newFence = device.getDevice().createFence(fenceInfo);
							this->fences[static_cast<star::Command_Buffer_Type>(i)].push_back(newFence);

							this->semaphores[static_cast<star::Command_Buffer_Type>(i)].push_back(device.getDevice().createSemaphore(semaphoreInfo));
						}
					}
				};

				~GenericBufferGroupInfo() {
					for (auto& typeSemaphore : this->semaphores)
						for (auto& semaphore: typeSemaphore.second)
							this->device.getDevice().destroySemaphore(semaphore);

					for (auto& typeFence : this->fences)
						for (auto& fence : typeFence.second)
							this->device.getDevice().destroyFence(fence);
				};

			private:
				StarDevice& device; 
			};

			StarDevice& device; 
			std::vector<std::unique_ptr<CompleteRequest>> allBuffers = std::vector<std::unique_ptr<CompleteRequest>>();
			std::unordered_map<star::Command_Buffer_Order, std::unique_ptr<GenericBufferGroupInfo>> bufferGroups = std::unordered_map<star::Command_Buffer_Order, std::unique_ptr<GenericBufferGroupInfo>>();

			//0 - no
			//1 - dynamic submit
			//2 - standard always submit
			std::vector<unsigned char> bufferSubmissionStatus;

			void waitUntilOrderGroupReady(const int& frameIndex, const star::Command_Buffer_Order& order, const star::Command_Buffer_Type& type) {
				this->device.getDevice().waitForFences(this->bufferGroups[order]->fences[type].at(frameIndex), VK_TRUE, UINT64_MAX);
				this->device.getDevice().resetFences(this->bufferGroups[order]->fences[type].at(frameIndex));
			}

			std::vector<std::reference_wrapper<CompleteRequest>> getAllBuffersOfTypeAndOrderReadyToSubmit(const star::Command_Buffer_Order& order, const star::Command_Buffer_Type& type, bool triggerReset = false) {
				std::vector<std::reference_wrapper<CompleteRequest>> buffers;

				for (const auto& index : this->bufferGroups[order]->bufferOrderGroupsIndices[type]) {
					if (this->allBuffers[index]->type == type && (this->bufferSubmissionStatus[index] == 1 || this->bufferSubmissionStatus[index] == 2)) {
						if (triggerReset)
							this->resetThisBufferStatus(index);

						buffers.push_back(*this->allBuffers[index]);
					}
				}

				return buffers;
			}
		};

		static std::stack<std::function<CommandBufferRequest(void)>> newCommandBufferRequests;

		static std::stack<Handle> dynamicBuffersToSubmit; 

		StarDevice& device; 
		const int numFramesInFlight = 0; 
		BufferContainer buffers;
		std::unique_ptr<star::Handle> mainGraphicsBufferHandle = std::unique_ptr<star::Handle>(); 

		uint32_t numBuffers = 0; 

		void handleNewRequests();

		vk::Semaphore submitCommandBuffers(const int& swapChainIndex);

		void handleDynamicBufferRequests(); 
	};
}