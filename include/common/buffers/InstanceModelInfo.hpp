#pragma once

#include "BufferMemoryTransferRequest.hpp"
#include "BufferManagerRequest.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {
	class InstanceModelInfoTransfer : public BufferMemoryTransferRequest{
	public:
	InstanceModelInfoTransfer(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances)
		: displayMatrixInfo(std::vector<glm::mat4>(objectInstances.size()))
		{
			for (int i = 0; i < objectInstances.size(); i++){
				displayMatrixInfo[i] = objectInstances[i]->getDisplayMatrix();
			}
		}

	BufferCreationArgs getCreateArgs() const override{
		return BufferCreationArgs{
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

	class InstanceModelInfo : public BufferManagerRequest {
	public:
	InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
		: star::BufferManagerRequest(static_cast<uint8_t>(frameInFlightToUpdateOn)), objectInstances(objectInstances)
	{
	}

	std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const override;
	private:
	const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances; 

	};
}