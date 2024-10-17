#pragma once

#include "BufferModifier.hpp"
#include "Light.hpp"

#include <glm/glm.hpp>

#include <vector>
namespace star {
	class LightInfo : public BufferModifier {
	public:
		LightInfo(const uint16_t& frameInFlightIndexToUpdateOn, const std::vector<std::unique_ptr<Light>>& lights) : lights(lights), BufferModifier(
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_AUTO,
			vk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(LightBufferObject),
			lights.size(),
			vk::SharingMode::eConcurrent, 1, frameInFlightIndexToUpdateOn) { };

	private:
		struct LightBufferObject {
			glm::vec4 position = glm::vec4(1.0f);
			glm::vec4 direction = glm::vec4(1.0f);     //direction in which the light is pointing
			glm::vec4 ambient = glm::vec4(1.0f);
			glm::vec4 diffuse = glm::vec4(1.0f);
			glm::vec4 specular = glm::vec4(1.0f);
			//controls.x = inner cutoff diameter 
			//controls.y = outer cutoff diameter
			glm::vec4 controls = glm::vec4(0.0f);       //container for single float values
			//settings.x = enabled
			//settings.y = type
			glm::uvec4 settings = glm::uvec4(0);    //container for single uint values
		};

		const std::vector<std::unique_ptr<Light>>& lights;

		void writeBufferData(StarBuffer& buffer) override;

	};
}