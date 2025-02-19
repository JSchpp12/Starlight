#pragma once

#include "BufferManagerRequest.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {
	class InstanceModelInfo : public BufferManagerRequest {
	public:
		InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
			: star::BufferManagerRequest(
				BufferManagerRequest::BufferCreationArgs{
					sizeof(glm::mat4),
					static_cast<uint32_t>(objectInstances.size()),
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					VMA_MEMORY_USAGE_AUTO,
					vk::BufferUsageFlagBits::eUniformBuffer,
					vk::SharingMode::eConcurrent},
					static_cast<uint8_t>(frameInFlightToUpdateOn)), 
				displayMatrixInfo(std::vector<glm::mat4>(objectInstances.size())){
					for (int i = 0; i < objectInstances.size(); i++){
						displayMatrixInfo[i] = objectInstances[i]->getDisplayMatrix();
					}
				}

		void write(StarBuffer& buffer) override;

	private:
		std::vector<glm::mat4> displayMatrixInfo;

	};
}