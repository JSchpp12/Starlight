#include "TransferRequest_InstanceNormalInfo.hpp"

std::unique_ptr<star::StarBuffer> star::TransferRequest::InstanceNormalInfo::createStagingBuffer(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
	auto create = StarBuffer::BufferCreationArgs{
		sizeof(glm::mat4),
		static_cast<uint32_t>(this->normalMatrixInfo.size()),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eConcurrent,
		"InstanceNormalInfoBuffer"
	};

	return std::make_unique<StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> star::TransferRequest::InstanceNormalInfo::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
		auto create = StarBuffer::BufferCreationArgs{
		sizeof(glm::mat4),
		static_cast<uint32_t>(this->normalMatrixInfo.size()),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eUniformBuffer |vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eConcurrent,
		"InstanceNormalInfoBuffer"
	};

	return std::make_unique<StarBuffer>(allocator, create); 
}

void star::TransferRequest::InstanceNormalInfo::writeDataToStageBuffer(star::StarBuffer& buffer) const{
    buffer.map(); 

	std::vector<glm::mat4> inverseTranspose = std::vector<glm::mat4>(this->normalMatrixInfo.size());

	for (int i = 0; i < this->normalMatrixInfo.size(); i++){
		inverseTranspose[i] = glm::inverse(glm::transpose(this->normalMatrixInfo[i]));
	}
    
	buffer.writeToBuffer(inverseTranspose.data());
	buffer.unmap(); 
}