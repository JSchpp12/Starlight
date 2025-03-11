#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Memory.hpp"
#include "Light.hpp"

#include <glm/glm.hpp>

#include <vector>
namespace star {
	namespace TransferRequest{
		class LightInfo : public Memory<StarBuffer::BufferCreationArgs>{
			public:
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
		
			LightInfo(const std::vector<std::unique_ptr<Light>>& lights)
			{
				for (int i = 0; i < lights.size(); ++i)
				{
					myLights.push_back(Light(*lights[i].get()));
				} 
			}
		
			StarBuffer::BufferCreationArgs getCreateArgs() const override{
				return StarBuffer::BufferCreationArgs{
					sizeof(LightBufferObject),
					static_cast<uint32_t>(this->myLights.size()),
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					VMA_MEMORY_USAGE_AUTO,
					vk::BufferUsageFlagBits::eStorageBuffer,
					vk::SharingMode::eConcurrent
				};
			}
		
			protected:
			std::vector<Light> myLights; 
		
			void writeData(StarBuffer& buffer) const override; 
		
		};
	}
	

	class LightInfo : public ManagerController::RenderResource::Buffer {
	public:
	LightInfo(const uint16_t& frameInFlightIndexToUpdateOn, const std::vector<std::unique_ptr<Light>>& lights) 
	: lights(lights), 
	ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightIndexToUpdateOn)) 
	{ 
	}

	protected:
	const std::vector<std::unique_ptr<Light>>& lights;
	uint16_t lastWriteNumLights = 0; 

	std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest() const override;

	bool isValid(const uint8_t& currentFrameInFlightIndex) const override;
	};
}
