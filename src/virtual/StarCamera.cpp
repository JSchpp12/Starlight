#include "StarCamera.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

star::StarCamera::StarCamera(const float& width, const float& height) : resolution(glm::vec2{width, height}){

}

glm::mat4 star::StarCamera::getViewMatrix() const{
    glm::vec3 position = this->getPosition(); 

    return glm::lookAt(
        this->getPosition(),
        this->getPosition() + glm::vec3(this->getForwardVector()),
        glm::vec3(this->getUpVector())
    );
}

glm::mat4 star::StarCamera::getProjectionMatrix() const{
    return glm::perspective(getVerticalFieldOfView(true), this->resolution.x / this->resolution.y, this->nearClippingPlaneDistance, this->farClippingPlaneDistance);
}

float star::StarCamera::getHorizontalFieldOfView(const bool& inRadians) const{
    if (inRadians)
        return glm::radians(this->horizontalFieldOfView);
    else
        return this->horizontalFieldOfView; 
}

float star::StarCamera::getVerticalFieldOfView(const bool& inRadians) const{
    const float verticalRadians = glm::radians(this->horizontalFieldOfView);
    const float resultRadians = 2.0f * glm::atan(glm::tan(verticalRadians / 2.0f) / (this->resolution.x / this->resolution.y));

    if (inRadians)
        return resultRadians; 
    else
        return glm::degrees(resultRadians); 
}