#pragma once 
//right hand coordinate system 
//row-major notation 

#include "StarEntity.hpp"

#include <glm/glm.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <memory>

//Note: this camera system will fail if the user looks up the +y-axis
namespace star {
//Camera object which will be used during rendering
class StarCamera : public StarEntity
{
public:
	StarCamera(const float& width, const float& height) 
		: resolution{width, height}
	{
	};

	virtual ~StarCamera() = default;

	glm::mat4 getViewMatrix() const{
		glm::vec3 position = this->getPosition(); 

		return glm::lookAt(
			this->getPosition(),
			this->getPosition() + glm::vec3(this->getForwardVector()),
			glm::vec3(this->getUpVector())
		);
	}

	glm::mat4 getProjectionMatrix() const{
		return glm::perspective(glm::radians(this->fieldOfView), this->resolution.x / this->resolution.y, this->nearClippingPlaneDistance, this->farClippingPlaneDistance);
	}

	glm::vec2 getResolution() const{ return this->resolution; }

	float getFieldOfView() const { return this->fieldOfView; }
protected:
	glm::vec2 resolution;

	float fieldOfView = 45.0f, nearClippingPlaneDistance=0.1f, farClippingPlaneDistance=10.0f;
	float pitch = 0.0f, yaw = 0.0f; 
};
}