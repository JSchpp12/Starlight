#pragma once

#include "BufferModifier.hpp"
#include "StarCamera.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class GlobalInfo : public BufferModifier {
	public:
		GlobalInfo(const uint16_t& frameInFlightIndexToUpdateOn, StarCamera& camera, const int& numLights) 
			: BufferModifier(
				(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 
				VMA_MEMORY_USAGE_AUTO, vk::BufferUsageFlagBits::eUniformBuffer, 
				sizeof(GlobalUniformBufferObject), 1, 
				vk::SharingMode::eConcurrent, 1,frameInFlightIndexToUpdateOn),
			camera(camera), numLights(numLights) {};

	protected:
		struct GlobalUniformBufferObject {
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
			uint32_t numLights;                             //number of lights in render
		};

		star::StarCamera& camera; 
		const int& numLights; 

		// Inherited via BufferModifier
		void writeBufferData(StarBuffer& buffer) override;
	};
}