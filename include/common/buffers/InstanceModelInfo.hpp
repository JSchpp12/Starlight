#pragma once

#include "TransferRequest_Memory.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {
	namespace TransferRequest{
		class InstanceModel : public Memory<StarBuffer::BufferCreationArgs>{
			public:
			InstanceModel(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances)
				: displayMatrixInfo(std::vector<glm::mat4>(objectInstances.size()))
				{
					for (int i = 0; i < objectInstances.size(); i++){
						displayMatrixInfo[i] = objectInstances[i]->getDisplayMatrix();
					}
				}
		
			StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override{
				return StarBuffer::BufferCreationArgs{
					sizeof(glm::mat4),
					static_cast<uint32_t>(this->displayMatrixInfo.size()),
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					VMA_MEMORY_USAGE_AUTO,
					vk::BufferUsageFlagBits::eUniformBuffer,
					vk::SharingMode::eConcurrent
				};
			}
			
			void writeData(StarBuffer& buffer) const override;
		
			protected:
			std::vector<glm::mat4> displayMatrixInfo;
		};
	}

	class InstanceModelInfo : public ManagerController::RenderResource::Buffer {
	public:
	InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
		: ManagerController::RenderResource::Buffer(static_cast<uint8_t>(frameInFlightToUpdateOn)), objectInstances(objectInstances)
	{
	}

	std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest() const override;
	private:
	const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances; 

	};
}