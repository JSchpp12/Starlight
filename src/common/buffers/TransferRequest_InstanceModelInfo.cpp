#include "TransferRequest_InstanceModelInfo.hpp"

std::unique_ptr<star::StarBuffer> star::TransferRequest::InstanceModelInfo::createStagingBuffer(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
	auto create = StarBuffer::BufferCreationArgs{
		sizeof(glm::mat4),
		static_cast<uint32_t>(this->displayMatrixInfo.size()),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eConcurrent,
		"InstanceModelInfoBuffer"
	};

	return std::make_unique<StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> star::TransferRequest::InstanceModelInfo::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
	auto create = StarBuffer::BufferCreationArgs{
		sizeof(glm::mat4),
		static_cast<uint32_t>(this->displayMatrixInfo.size()),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eConcurrent,
		"InstanceModelInfoBuffer"
	};

	return std::make_unique<StarBuffer>(allocator, create); 
}

void star::TransferRequest::InstanceModelInfo::writeDataToStageBuffer(star::StarBuffer& buffer) const{
    buffer.map();
	for (int i = 0; i < this->displayMatrixInfo.size(); ++i){
		glm::mat4 info = glm::mat4(this->displayMatrixInfo[i]);
		buffer.writeToBuffer(&info, sizeof(glm::mat4), i * sizeof(glm::mat4));
	}

	buffer.unmap();
}