#pragma once

#include "BufferManagerRequest.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {
	class InstanceNormalInfo : public BufferManagerRequest {
	public:
		InstanceNormalInfo(const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
			: BufferManagerRequest(
				BufferManagerRequest::BufferCreationArgs{
					sizeof(glm::mat4),
					static_cast<uint32_t>(objectInstances.size()),
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					VMA_MEMORY_USAGE_AUTO,
					vk::BufferUsageFlagBits::eUniformBuffer,
					vk::SharingMode::eConcurrent},
				static_cast<uint8_t>(frameInFlightToUpdateOn)),
			normalMatrixInfo(std::vector<glm::mat4>(objectInstances.size())) {
				for (int i = 0; i < objectInstances.size(); i++){
					normalMatrixInfo[i] = objectInstances[i]->getDisplayMatrix();
				}
			}

	protected:
		void write(StarBuffer& buffer) override;

	private:
		std::vector<glm::mat4> normalMatrixInfo; 

	};
}