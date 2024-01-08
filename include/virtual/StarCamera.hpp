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
class StarCamera
{
public:
	StarCamera() = default; 
	virtual ~StarCamera() = default;

	virtual void setPosition(const glm::vec3& newPosition) {
		this->position = newPosition;

		//compute right vector
		//use vec {0, 1, 0} as arbitrary vec for calculation 
		glm::vec3 tempArbVector{ 0.0f, 1.0f, 0.0f };
		auto rightVector = glm::normalize(glm::cross(tempArbVector, this->forwardVector));

		//compute up vector 
		this->upVector = glm::normalize(glm::cross(this->forwardVector, rightVector));
	}

	glm::vec3 getLookDirection() { return this->forwardVector; };

	glm::vec3 getPosition() { return this->position; };

	glm::mat4 getDisplayMatrix() {
		return glm::lookAt(
			this->position,
			this->position + (this->forwardVector),
			this->upVector
		);
	}

	glm::mat4 getInverseViewMatrix() {
		return glm::inverse(
			glm::lookAt(
				this->position,
				this->position + (this->forwardVector),
				this->upVector
			)
		);
	}

	void setLookDirection(glm::vec3 newLookDirection) {
		this->forwardVector = glm::normalize(newLookDirection);

	}

	void moveRelative(glm::vec3 direction, float amt) {
		direction = glm::normalize(direction);

		//update camera position
		auto position = this->position + (direction * amt);
		this->setPosition(position);
	}

protected:
	glm::vec3 upVector=glm::vec3{0.0f, 1.0f, 0.0f}, forwardVector = glm::vec3{0.f, 0.f, -1.0f}, position = glm::vec3{0.0f, 0.0f, -5.0f};
	float fieldOfView = 45, nearClippingPlaneDistance=0.0f, farClippingPlaneDistance=10.0f;
	float pitch = 0.0f, yaw = 0.0f; 
private:

};
}