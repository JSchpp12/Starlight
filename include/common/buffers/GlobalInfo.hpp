#pragma once

#include "BufferManagerRequest.hpp"
#include "StarCamera.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class GlobalInfo : public BufferManagerRequest {
	public:
		GlobalInfo(const uint16_t& frameInFlightIndexToUpdateOn, StarCamera& camera, const int& numLights) 
			: BufferManagerRequest(
				BufferManagerRequest::BufferCreationArgs{
					sizeof(GlobalUniformBufferObject),
					1,
					(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 
					VMA_MEMORY_USAGE_AUTO, 
					vk::BufferUsageFlagBits::eUniformBuffer, 
					vk::SharingMode::eConcurrent},
				static_cast<uint8_t>(frameInFlightIndexToUpdateOn)),
			camera(camera), numLights(numLights) {};

	protected:
		struct GlobalUniformBufferObject {
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
			uint32_t numLights;                             //number of lights in render
		};

		const star::StarCamera camera; 
		const int numLights; 

		void write(StarBuffer& buffer) override;
	};
}