#include "LightInfo.hpp"

void star::LightInfo::writeBufferData(StarBuffer& buffer)
{
	buffer.map(); 

	std::vector<LightBufferObject> lightInformation(this->lights.size());
	LightBufferObject newBufferObject{};

	for (size_t i = 0; i < this->lights.size(); i++) {
		const Light& currLight = *this->lights.at(i);
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
