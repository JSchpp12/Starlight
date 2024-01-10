#include "BasicCamera.hpp"

star::BasicCamera::BasicCamera(const float& width, const float& height)
	: star::StarCamera(width, height)
{
	this->registerInteractions();
}

void star::BasicCamera::onKeyPress(int key, int scancode, int mods) {
}

void star::BasicCamera::onKeyRelease(int key, int scancode, int mods)
{
}

void star::BasicCamera::onMouseMovement(double xpos, double ypos) {
	if (this->click) {
		//prime camera
		if (!this->init) {
			this->prevX = xpos;
			this->prevY = ypos;
			this->init = true;
		}

		this->xMovement = xpos - this->prevX;
		this->yMovement = ypos - this->prevY;
		this->prevX = xpos;
		this->prevY = ypos;
	}
}

void star::BasicCamera::onMouseButtonAction(int button, int action, int mods)
{
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		this->click = true;
	}
	else if (this->click && button == GLFW_MOUSE_BUTTON_LEFT) {
		this->click = false;
		this->init = false;
	}
}

void star::BasicCamera::onWorldUpdate() {
	//TODO: improve time var allocation 
	bool moveLeft = KeyStates::state(A);
	bool moveRight = KeyStates::state(D);
	bool moveForward = KeyStates::state(W);
	bool moveBack = KeyStates::state(S);

	if ((double)time.timeElapsedLastFrameSeconds() > 1) {
		time.updateLastFrameTime();
	}

	if (moveLeft || moveRight || moveForward || moveBack) {
		float moveAmt = 0.3f * time.timeElapsedLastFrameSeconds();

		glm::vec3 cameraPos = this->getPosition();
		glm::vec3 cameraLookDir = -this->getLookDirection();

		if (moveLeft) {
			this->moveRelative(glm::cross(cameraLookDir, glm::vec3(this->upVector)), moveAmt);
		}
		if (moveRight) {
			this->moveRelative(glm::cross(cameraLookDir, -glm::vec3(this->upVector)), moveAmt);
		}
		if (moveForward) {
			this->moveRelative(this->forwardVector, moveAmt);
		}
		if (moveBack) {
			this->moveRelative(-this->forwardVector, moveAmt);
		}

		time.updateLastFrameTime();
	}

	if (this->click && this->init) {
		this->xMovement *= this->sensitivity;
		this->yMovement *= this->sensitivity;

		this->yaw += this->xMovement;
		this->pitch += this->yMovement;

		//apply restrictions due to const up vector for the camera 
		if (this->pitch > 89.0f) {
			pitch = 89.0f;
		}
		if (this->pitch < -89.0f) {
			pitch = -89.0f;
		}

		glm::vec3 direction{
			cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch)),
			sin(glm::radians(this->pitch)),
			sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch))
		};

		this->forwardVector = glm::vec4(glm::normalize(direction), 0.0);
	}
}