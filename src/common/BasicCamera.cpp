#include "BasicCamera.hpp"

star::BasicCamera::BasicCamera(const uint32_t &width, const uint32_t &height) : star::StarCamera(width, height)
{
}

star::BasicCamera::BasicCamera(const uint32_t &width, const uint32_t &height, const float &horizontalFieldOfView,
                               const float &nearClippingPlaneDistance, const float &farClippingPlaneDistance,
                               const float &movementSpeed, const float &sensitivity)
    : star::StarCamera(width, height, horizontalFieldOfView, nearClippingPlaneDistance, farClippingPlaneDistance),
      movementSpeed(movementSpeed), sensitivity(sensitivity)
{
    this->registerInteractions();
}

void star::BasicCamera::onKeyPress(int key, int scancode, int mods)
{
}

void star::BasicCamera::onKeyRelease(int key, int scancode, int mods)
{
}

void star::BasicCamera::onMouseMovement(double xpos, double ypos)
{
    if (this->click)
    {
        // prime camera
        if (!this->init)
        {
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
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        this->click = true;
    }
    else if (this->click && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        this->click = false;
        this->init = false;
    }
}

void star::BasicCamera::onWorldUpdate(const uint32_t &frameInFlightIndex)
{
    // TODO: improve time var allocation
    bool moveLeft = KeyStates::state(A);
    bool moveRight = KeyStates::state(D);
    bool moveForward = KeyStates::state(W);
    bool moveBack = KeyStates::state(S);

    if ((double)time.timeElapsedLastFrameSeconds() > 1)
    {
        time.updateLastFrameTime();
    }

    if (moveLeft || moveRight || moveForward || moveBack)
    {
        float moveAmt = this->movementSpeed * time.timeElapsedLastFrameSeconds();

        glm::vec3 cameraPos = this->getPosition();
        glm::vec3 cameraLookDir = this->getForwardVector();

        if (moveLeft)
        {
            this->moveRelative(glm::cross(glm::vec3(this->getForwardVector()), -glm::vec3(this->getUpVector())),
                               moveAmt);
        }
        if (moveRight)
        {
            this->moveRelative(glm::cross(glm::vec3(this->getForwardVector()), glm::vec3(this->getUpVector())),
                               moveAmt);
        }
        if (moveForward)
        {
            this->moveRelative(this->getForwardVector(), moveAmt);
        }
        if (moveBack)
        {
            this->moveRelative(-this->getForwardVector(), moveAmt);
        }

        time.updateLastFrameTime();
    }

    if (this->click && this->init)
    {
        this->xMovement *= this->sensitivity;
        this->yMovement *= this->sensitivity;

        this->yaw += this->xMovement;
        this->pitch += this->yMovement;

        // apply restrictions due to const up vector for the camera
        if (this->pitch > 89.0f)
        {
            pitch = 89.0f;
        }
        if (this->pitch < -89.0f)
        {
            pitch = -89.0f;
        }

        glm::vec3 direction{cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch)),
                            sin(glm::radians(this->pitch)),
                            sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch))};

        this->setForwardVector(glm::vec4(glm::normalize(direction), 0.0));
    }
}