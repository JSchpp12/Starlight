#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Memory.hpp"
#include "StarCamera.hpp"
#include "ObjVertInfo.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	namespace TransferRequest{
		class GlobalInfo : public Memory<StarBuffer::BufferCreationArgs>{
			public:
			struct GlobalUniformBufferObject {
				alignas(16) glm::mat4 proj;
				alignas(16) glm::mat4 view;
				alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
				uint32_t numLights;                             //number of lights in render
			};
	
			GlobalInfo(const StarCamera& camera, const int& numLights) : camera(camera), numLights(numLights)
			{
			}
	
			StarBuffer::BufferCreationArgs getCreateArgs() const override{
				return StarBuffer::BufferCreationArgs{
					sizeof(GlobalUniformBufferObject),
					1,
					(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 
					VMA_MEMORY_USAGE_AUTO, 
					vk::BufferUsageFlagBits::eUniformBuffer, 
					vk::SharingMode::eConcurrent
				};
			}
	
			protected:
			int numLights = 0; 
			const StarCamera camera;
			
			void writeData(StarBuffer& buffer) const override; 
		};
	}

	class GlobalInfo : public ManagerController::RenderResource::Buffer {
	public:
		GlobalInfo(const uint16_t& frameInFlightIndexToUpdateOn, StarCamera& camera, const int& numLights) 
			: ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightIndexToUpdateOn)),
			camera(camera), numLights(numLights), lastNumLights(numLights)
		{
		}

		std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest() const override;

	protected:
		const star::StarCamera& camera; 
		const int& numLights; 
		int lastNumLights = 0;
	};
}