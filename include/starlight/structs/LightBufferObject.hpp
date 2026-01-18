#pragma once

#include <glm/glm.hpp>

namespace star {
struct LightBufferObject {
    glm::vec4 position = glm::vec4(1.0f);
    glm::vec4 direction = glm::vec4(1.0f);     //direction in which the light is pointing
    glm::vec4 ambient = glm::vec4(1.0f);
    glm::vec4 diffuse = glm::vec4(1.0f);
    glm::vec4 specular = glm::vec4(1.0f);
    //controls.x = inner cutoff diameter 
    //controls.y = outer cutoff diameter
    glm::vec4 controls = glm::vec4(0.0f);       //container for single float values
    //settings.x = enabled
    //settings.y = type
    glm::uvec4 settings = glm::uvec4(0);    //container for single uint values
};
}