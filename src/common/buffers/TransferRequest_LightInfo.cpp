#include "TransferRequest_LightInfo.hpp"

std::unique_ptr<star::StarBuffer> star::TransferRequest::LightInfo::createStagingBuffer(vk::Device& device, VmaAllocator &allocator) const{
	auto create = StarBuffer::BufferCreationArgs{
		sizeof(LightBufferObject),
		static_cast<uint32_t>(this->myLights.size()),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eConcurrent,
		"LightInfoBufferSRC"
	};

	return std::make_unique<StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> star::TransferRequest::LightInfo::createFinal(vk::Device &device, VmaAllocator &allocator) const{
	auto create = StarBuffer::BufferCreationArgs{
		sizeof(LightBufferObject),
		static_cast<uint32_t>(this->myLights.size()),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::SharingMode::eConcurrent,
		"LightInfoBuffer"
	};

	return std::make_unique<StarBuffer>(allocator, create); 
}

void star::TransferRequest::LightInfo::writeDataToStageBuffer(star::StarBuffer& buffer) const{
    buffer.map(); 

	std::vector<LightBufferObject> lightInformation(this->myLights.size());
	LightBufferObject newBufferObject{};

	for (size_t i = 0; i < this->myLights.size(); i++) {
		const Light& currLight = this->myLights.at(i);
		newBufferObject.position = glm::vec4{ currLight.getPosition(), 1.0f };
		newBufferObject.direction = currLight.direction;
		newBufferObject.ambient = currLight.getAmbient();
		newBufferObject.diffuse = currLight.getDiffuse();
		newBufferObject.specular = currLight.getSpecular();
		newBufferObject.settings.x = currLight.getEnabled() ? 1 : 0;
		newBufferObject.settings.y = currLight.getType();
		newBufferObject.controls.x = glm::cos(glm::radians(currLight.getInnerDiameter()));		//represent the diameter of light as the cos of the light (increase shader performance when doing comparison)
		newBufferObject.controls.y = glm::cos(glm::radians(currLight.getOuterDiameter()));
		lightInformation[i] = newBufferObject;
	}

	buffer.writeToBuffer(lightInformation.data(), sizeof(LightBufferObject) * lightInformation.size());

	buffer.unmap(); 
}