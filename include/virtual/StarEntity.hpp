#pragma once 

#include "Enums.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <memory>
#include <stdexcept>
#include <iostream>

namespace star {
class StarEntity {
public:
	glm::vec3 positionCoords = glm::vec3();

	StarEntity() = default;

	StarEntity(const glm::vec3& position) : positionCoords(position) {
		this->setPosition(position);
	}

	StarEntity(const glm::vec3& position, const glm::vec3& scale) {
		this->setPosition(position);
		this->setScale(scale);
	}

	glm::vec3 getPosition() const{
		glm::mat4 matrixCopy = getDisplayMatrix();
		return glm::vec3{ matrixCopy[3][0], matrixCopy[3][1], matrixCopy[3][2] };
	}

	glm::vec3 getScale() {
		glm::mat4 matrixCopy = translationMat * rotationMat * scaleMat;
		return glm::vec3{
			matrixCopy[0][0],
			matrixCopy[1][1],
			matrixCopy[2][2]
		};
	}

	virtual void setScale(glm::vec3 scale) {
		if (scale.x != scale.y && scale.y != scale.z) {
			std::cout << "WARNING: Non-uniform scaling applied to object. This WILL break axis aligned bounding box calculations." << std::endl;
		}
		scaleMat = glm::scale(scaleMat, scale);
	}

	virtual void setPosition(glm::vec3 newPosition) {
		positionCoords = newPosition;
		translationMat = glm::translate(newPosition);
		this->updateCoordsTranslation(translationMat);
	}

	/// <summary>
	/// Apply translation to object's current position vector and update accordingly
	/// </summary>
	/// <param name="movement"></param>
	virtual void moveRelative(const glm::vec3& movement) {
		//need to update model matrix before applying further translations
		translationMat = glm::translate(translationMat, movement);

		positionCoords = translationMat * glm::vec4(positionCoords, 1.0);
		this->updateCoordsTranslation(translationMat);
	}
	/// <summary>
	/// Move entity in a direction defined by a normalized direction vector. 
	/// </summary>
	/// <param name="movementDirection">Movement direction vector. Will be normalized.</param>
	/// <param name="movementAmt">Ammount to move the entity by</param>
	virtual void moveRelative(const glm::vec3& movementDirection, const float& movementAmt) {
		auto normMove = glm::normalize(movementDirection);
		glm::vec3 movement = glm::vec3{
			normMove.x * movementAmt,
			normMove.y * movementAmt,
			normMove.z * movementAmt
		};

		translationMat = glm::translate(translationMat, movement);
		this->updateCoordsTranslation(translationMat);
	}

	/// <summary>
	/// Rotate object about a relative axis
	/// </summary>
	/// <param name="amt">Ammount of rotation to apply</param>
	/// <param name="rotationVector">Vector around which to apply rotation</param>
	/// <param name="inDegrees">Is the amount provided in degrees</param>
	void rotateRelative(const Type::Axis& axis, float amt, bool inDegrees = true) {
		float radians = 0.0f;
		glm::vec3 rotationVector = glm::vec3();
		assert((axis == Type::Axis::x || axis == Type::Axis::y || axis == Type::Axis::z) && "Invalid axis type provided");

		if (inDegrees) {
			radians = glm::radians(amt);
		}
		else {
			radians = amt;
		}

		if (axis == Type::Axis::x) {
			rotationVector = xAxis;
		}
		else if (axis == Type::Axis::y) {
			rotationVector = yAxis;
		}
		else if (axis == Type::Axis::z) {
			rotationVector = zAxis;
		}
		//might want to normalize vector
		rotationVector = glm::normalize(rotationVector);

		rotationMat = glm::rotate(rotationMat, radians, rotationVector);

		this->updateCoordsRot(rotationMat);
	}

	void rotateGlobal(const Type::Axis& axis, const float& amt, bool inDegrees = true) {
		float radians = 0.0f;
		glm::vec3 rotationVector = glm::vec3();
		assert((axis == Type::Axis::x || axis == Type::Axis::y || axis == Type::Axis::z) && "Invalid axis type provided");

		if (inDegrees) {
			radians = glm::radians(amt);
		}
		else {
			radians = amt;
		}

		if (axis == Type::Axis::x) {
			rotationVector = glm::vec3{ 1.0f, 0.0f, 0.0f };
		}
		else if (axis == Type::Axis::y) {
			rotationVector = glm::vec3{ 0.0f, 1.0f, 0.0f };
		}
		else if (axis == Type::Axis::z) {
			rotationVector = glm::vec3{ 0.0f, 0.0f, 1.0f };
		}

		rotationVector = glm::normalize(rotationVector);
		rotationMat = glm::rotate(rotationMat, radians, rotationVector);

		this->updateCoordsRot(rotationMat);

	}

	virtual glm::mat4 getDisplayMatrix() const {
		return translationMat * rotationMat * scaleMat;
	}

	void setForwardVector(const glm::vec3& newForward) {
		//TODO: setting this vector should probably update the 
		this->forwardVector = glm::vec4(glm::normalize(newForward), 1.0f);

		glm::vec3 forward = glm::vec3(this->forwardVector);
		glm::vec3 rightVec = glm::cross(forward, glm::vec3{ 0.0f, 1.0f, 0.0f });

		this->upVector = glm::vec4(glm::cross(rightVec, forward), 1.0);
	}

	glm::vec4 getForwardVector() const { return this->forwardVector; }
	glm::vec4 getUpVector() const { return this->upVector; }

protected:
	glm::mat4 rotationMat = glm::mat4(glm::identity<glm::mat4>());
	glm::mat4 translationMat = glm::mat4(glm::identity<glm::mat4>());
	glm::mat4 scaleMat = glm::mat4(glm::identity<glm::mat4>());

private:
	glm::vec4 xAxis = glm::vec4{ 1.0f, 0.0, 0.0f, 0.0f };
	glm::vec4 yAxis = glm::vec4{ 0.0f, 1.0f, 0.0f, 0.0f };
	glm::vec4 zAxis = glm::vec4{ 0.0f, 0.0f, 1.0f, 0.0f };

	glm::vec4 upVector = glm::vec4{ 0.0f, 1.0f, 0.0f, 0.0f };		//original up vector of object 
	glm::vec4 forwardVector = glm::vec4{ 1.0f, 0.0, 0.0f, 0.0f };	//original forward vector of object

		void updateCoordsRot(const glm::mat4& rotMat) {
		xAxis = glm::normalize(rotMat * xAxis);
		yAxis = glm::normalize(rotMat * yAxis);
		zAxis = glm::normalize(rotMat * zAxis);

		this->upVector = glm::normalize(rotMat * this->upVector);
		this->forwardVector = glm::normalize(rotMat * this->forwardVector);
	}

	void updateCoordsTranslation(const glm::mat4& transMat) {
		xAxis = glm::normalize(transMat * xAxis);
		yAxis = glm::normalize(transMat * yAxis);
		zAxis = glm::normalize(transMat * zAxis);
	}
};
}