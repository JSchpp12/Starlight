#pragma once 

#include "StarDevice.hpp"

#include "vulkan/vulkan.hpp"

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

		vk::CommandBuffer& buffer(int buffIndex) { return this->commandBuffers.at(buffIndex); }

		/// <summary>
		/// Submit the command buffer for execution
		/// </summary>
		void submit(int targetIndex, vk::Fence& fence); 

		/// <summary>
		/// Submit the command buffer for execution, adding any one time wait information. 
		/// </summary>
		void submit(int targetIndex, vk::Fence& fence, std::pair<vk::Semaphore, vk::PipelineStageFlags> overrideWait);

		/// <summary>
		/// This command buffer should wait for the provided semaphores to complete before proceeding.
		/// This wait information will be provided to vulkan during submit(). 
		/// </summary>
		/// <param name="semaphores"></param>
		/// <param name="flags"></param>
		void waitFor(std::vector<vk::Semaphore> semaphores, vk::PipelineStageFlags whereWait); 

		/// <summary>
		/// Returns the semaphores that will be signaled once this buffer is done executing. 
		/// </summary>
		const std::vector<vk::Semaphore>& getCompleteSemaphores(); 

	protected:
		StarDevice& device; 
		vk::Queue& targetQueue; 
		std::vector<vk::CommandBuffer> commandBuffers; 
		std::vector<vk::Semaphore> completeSemaphores; 
		std::vector<std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags>>> waitSemaphores; 
		bool recorded = false; 
	};
}