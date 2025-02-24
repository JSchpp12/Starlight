#pragma once

#include "BufferManagerRequest.hpp"
#include "BufferMemoryTransferRequest.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {

	class InstanceNormalTransfer : public BufferMemoryTransferRequest{
		public:
			InstanceNormalTransfer(const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances) {
				for (auto& instance : objectInstances) {
					this->normalMatrixInfo.push_back(instance->getDisplayMatrix());
				}
			}

			virtual BufferMemoryTransferRequest::BufferCreationArgs getCreateArgs() const override{
				return BufferCreationArgs{
					sizeof(glm::mat4),
					static_cast<uint32_t>(this->normalMatrixInfo.size()),
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
					VMA_MEMORY_USAGE_AUTO,
					vk::BufferUsageFlagBits::eUniformBuffer,
					vk::SharingMode::eConcurrent
				};
			}

		protected:
			std::vector<glm::mat4> normalMatrixInfo = std::vector<glm::mat4>(); 

			void writeData(StarBuffer& buffer) const override;
	};

	class InstanceNormalInfo : public BufferManagerRequest {
	public:
		InstanceNormalInfo(const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances, const int& frameInFlightToUpdateOn)
			: BufferManagerRequest(static_cast<uint8_t>(frameInFlightToUpdateOn)),
			objectInstances(objectInstances) {}

	protected:
		std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const override;

	private:
		const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances;

	};
}