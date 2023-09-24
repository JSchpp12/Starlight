#include "CameraController.hpp"

star::CameraController::CameraController()
{
	this->registerInteractions(); 
}

void star::CameraController::onKeyPress(int key, int scancode, int mods) {
}

void star::CameraController::onKeyRelease(int key, int scancode, int mods)
{
}

void star::CameraController::onMouseMovement(double xpos, double ypos) {
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

void star::CameraController::onMouseButtonAction(int button, int action, int mods)
{
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		this->click = true;
	}
	else if (this->click && button == GLFW_MOUSE_BUTTON_LEFT) {
		this->click = false;
		this->init = false;
	}
}

void star::CameraController::onWorldUpdate() {
	//TODO: improve time var allocation 
	bool moveLeft		= KeyStates::state(A); 
	bool moveRight		= KeyStates::state(D); 
	bool moveForward	= KeyStates::state(W); 
	bool moveBack		= KeyStates::state(S);

	if ((double)time.timeElapsedLastFrameSeconds() > 1) {
		time.updateLastFrameTime();
	}

	if (moveLeft || moveRight || moveForward || moveBack) {
		float moveAmt = 0.1f * time.timeElapsedLastFrameSeconds();

		glm::vec3 cameraPos = this->getPosition();
		glm::vec3 cameraLookDir = -this->getLookDirection();

		if (moveLeft) {
			this->moveRelative(glm::cross(cameraLookDir, *this->upVector), moveAmt);
		}
		if (moveRight) {
			this->moveRelative(glm::cross(cameraLookDir, -*this->upVector), moveAmt);
		}
		if (moveForward) {
			this->moveRelative(*this->lookDirection, moveAmt);
		}
		if (moveBack) {
			this->moveRelative(-*this->lookDirection, moveAmt);
		}
		//std::cout << cameraPos.x << "," << cameraPos.y << "," << cameraPos.z << std::endl;
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
		this->lookDirection = std::make_unique<glm::vec3>(glm::normalize(direction));
	}


}