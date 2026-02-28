#pragma once 

#include <glm/glm.hpp>

namespace star {
class Vertex {
public:
    glm::vec3 pos{ 0.0f,0.0f,0.0f };
    glm::vec3 normal{ 0.0f,0.0f,0.0f };
    glm::vec3 color{ 0.0f,0.0f,0.0f };
    glm::vec2 texCoord{ 0.0f,0.0f };
    glm::vec3 aTangent{ 0.0f,0.0f,0.0f };
    glm::vec3 aBitangent{ 0.0f, 0.0f, 0.0f };
    //material information
    glm::vec3 matAmbient = glm::vec3{ 1.0f, 1.0f, 1.0f };
    glm::vec3 matDiffuse = glm::vec3{ 1.0f, 1.0f, 1.0f };
    glm::vec3 matSpecular = glm::vec3{ 1.0f, 1.0f, 1.0f };
    float matShininess = 1.0f;

    Vertex() = default; 
    Vertex(const glm::vec3& pos) : pos(pos) {};
    Vertex(const glm::vec3& pos, const glm::vec3& normal) : pos(pos), normal(normal) {};
    Vertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec3& color) : pos(pos), normal(normal), color(color) {};
    Vertex(const glm::vec3& pos, const glm::vec3 normal, const glm::vec3& color, const glm::vec2& texCoord) : pos(pos), normal(normal), color(color), texCoord(texCoord) {};
private:

};
}