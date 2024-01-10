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

	void setLookDirection(const glm::vec3& newLookDir) {

		this->forwardVector = glm::vec4(newLookDir, 0.0f); 
	}

	glm::mat4 getViewMatrix() const{
		glm::vec3 position = this->getPosition(); 

		return glm::lookAt(
			this->getPosition(),
			this->getPosition() + glm::vec3(this->forwardVector),
			glm::vec3(this->upVector)
		);
	}

	glm::mat4 getProjectionMatrix() const{
		return glm::perspective(glm::radians(45.0f), this->resolution.x / this->resolution.y, 0.1f, 10.0f);
	}

	glm::vec2 getResolution() const{ return this->resolution; }

	glm::vec3 getLookDirection() const { return this->getDisplayMatrix() * this->forwardVector; }

protected:
	glm::vec2 resolution;

	float fieldOfView = 45, nearClippingPlaneDistance=0.0f, farClippingPlaneDistance=10.0f;
	float pitch = 0.0f, yaw = 0.0f; 

};
}