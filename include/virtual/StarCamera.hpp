#pragma once 
//right hand coordinate system 
//row-major notation 

#include "StarEntity.hpp"

#include <glm/glm.hpp>

//Note: this camera system will fail if the user looks up the +y-axis
namespace star {
//Camera object which will be used during rendering
class StarCamera : public StarEntity
{
public:
	StarCamera(const float& width, const float& height);

	virtual ~StarCamera() = default;

	glm::mat4 getViewMatrix() const; 

	glm::mat4 getProjectionMatrix() const;

	glm::vec2 getResolution() const{ return this->resolution; }

	float getHorizontalFieldOfView(const bool& inRadians = false) const;
	float getVerticalFieldOfView(const bool& inRadians = false) const; 
	float getNearClippingDistance() const {return this->nearClippingPlaneDistance;}
	float getFarClippingDistance() const {return this->farClippingPlaneDistance;}
protected:
	const glm::vec2 resolution;

	float horizontalFieldOfView = 90.0f, nearClippingPlaneDistance=0.1f, farClippingPlaneDistance=10000000.0f;
	float pitch = 0.0f, yaw = 0.0f; 
};
}