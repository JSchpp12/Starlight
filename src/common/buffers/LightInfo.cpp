#include "LightInfo.hpp"

void star::TransferRequest::LightInfo::writeData(StarBuffer& buffer) const
{
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

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::LightInfo::createTransferRequest() const
{
	return std::make_unique<TransferRequest::LightInfo>(this->lights);
}

bool star::LightInfo::isValid(const uint8_t& currentFrameInFlightIndex) const
{
	if (!star::ManagerController::RenderResource::Buffer::isValid(currentFrameInFlightIndex) && this->lastWriteNumLights != this->lights.size()){
		return false;
	}

	return true;
}
