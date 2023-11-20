#pragma once 

#include "StarDevice.hpp"
#include "StarTexture.hpp"

#include "vulkan/vulkan.hpp"

#include <vector>

namespace star {
	/// <summary>
	/// Create reusable command buffer object
	/// </summary>
	class StarCommandBuffer {
	public:
		StarCommandBuffer(StarDevice& device, int numBuffersToCreate, star::Command_Buffer_Type type);
		~StarCommandBuffer(); 

		/// <summary>
		/// Signal for begin of command recording.
		/// </summary>
		/// <param name="buffIndex">Buffer index to prepare.(Should equal swap chain image number in main renderer)</param>
		/// <returns>The command buffer which is ready for command recording.</returns>
		void begin(int buffIndex); 

		/// <summary>
		/// Signal for begin of command recording. This function will allow callee to manually define begin information.
		/// </summary>
		/// <param name="buffIndex">Buffer index to prepare.(Should equal swap chain image number in main renderer)</param>
		/// <param name="beginInfo">Vulkan begin info</param>
		/// <returns>The command buffer which is ready for command recording.</returns>
		void begin(int buffIndex, vk::CommandBufferBeginInfo beginInfo);

		void submit(int bufferIndex); 

		/// <summary>
		/// Submit the command buffer for execution
		/// </summary>
		void submit(int bufferIndex, vk::Fence& fence); 

		/// <summary>
		/// Submit the command buffer for execution, adding any one time wait information. 
		/// </summary>
		void submit(int bufferIndex, vk::Fence& fence, std::pair<vk::Semaphore, vk::PipelineStageFlags> overrideWait);

		/// <summary>
		/// This command buffer should wait for the provided semaphores to complete before proceeding.
		/// This wait information will be provided to vulkan during submit(). 
		/// </summary>
		/// <param name="semaphores"></param>
		/// <param name="flags"></param>
		void waitFor(std::vector<vk::Semaphore> semaphores, vk::PipelineStageFlags whereWait); 

		void reset(); 

		/// <summary>
		/// Returns the semaphores that will be signaled once this buffer is done executing. 
		/// </summary>
		const std::vector<vk::Semaphore>& getCompleteSemaphores(); 

		void transitionImageLayout(int bufferIndex, star::StarTexture& texture, vk::ImageLayout newLayout, 
			vk::AccessFlags srcFlags, vk::AccessFlags dstFlags,
			vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags dstStage); 

		vk::CommandBuffer& buffer(int buffIndex) { return this->commandBuffers.at(buffIndex); }
	protected:
		StarDevice& device; 
		vk::Queue& targetQueue; 
		std::vector<vk::CommandBuffer> commandBuffers; 
		std::vector<vk::Semaphore> completeSemaphores; 
		std::vector<vk::Fence> readyFence; 
		std::vector<std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags>>> waitSemaphores; 
		std::vector<std::unique_ptr<std::unordered_map<StarTexture*, std::pair<vk::ImageLayout, vk::ImageLayout>>>> recordedImageTransitions; 
		bool recorded = false; 

		void wait(int bufferIndex);

		void checkForImageTransitions(int bufferIndex); 
	};
}