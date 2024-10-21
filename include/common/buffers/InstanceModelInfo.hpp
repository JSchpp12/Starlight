#pragma once 

#include "BufferModifier.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace star {
	class InstanceModelInfo : public BufferModifier {
	public:
		InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances, const vk::DeviceSize& minPropertyFromDevice, const int& frameInFlightToUpdateOn)
			: BufferModifier(
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				VMA_MEMORY_USAGE_AUTO,
				vk::BufferUsageFlagBits::eUniformBuffer,
				sizeof(glm::mat4),
				objectInstances.size(), 
				vk::SharingMode::eConcurrent, 
				minPropertyFromDevice, 
				frameInFlightToUpdateOn), 
			objectInstances(objectInstances) {};

		// Inherited via BufferModifier
		void writeBufferData(StarBuffer& buffer) override;

	private:
		const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances;
	};
}