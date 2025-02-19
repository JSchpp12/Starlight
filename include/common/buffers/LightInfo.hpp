#ifndef STAR_LIGHT_INFO_HPP
#define STAR_LIGHT_INFO_HPP


#include "BufferManagerRequest.hpp"
#include "Light.hpp"

#include <glm/glm.hpp>

#include <vector>
namespace star {
	class LightInfo : public BufferManagerRequest {
	public:
		LightInfo(const uint16_t& frameInFlightIndexToUpdateOn, const std::vector<std::unique_ptr<Light>>& lights) 
		: lights(lights), 
		BufferManagerRequest(
			BufferManagerRequest::BufferCreationArgs{
				sizeof(LightBufferObject),
				static_cast<uint32_t>(lights.size()),
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				VMA_MEMORY_USAGE_AUTO,
				vk::BufferUsageFlagBits::eStorageBuffer,
				vk::SharingMode::eConcurrent},
			static_cast<uint8_t>(frameInFlightIndexToUpdateOn)) { };

	protected:
	
		void write(StarBuffer& buffer) override;

		bool isValid(const uint8_t& currentFrameInFlightIndex) const override;

	private:
		uint16_t lastWriteNumLights = 0; 

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
	};
}

#endif