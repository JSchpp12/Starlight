#pragma once 

#include "StarEntity.hpp"
#include "Time.hpp"
#include "Handle.hpp"
#include "Material.hpp"
#include "Vertex.hpp"
#include "Mesh.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <queue>
#include <memory>
#include <vector>

namespace star {
class GameObject : public StarEntity {
public:
    static std::vector<std::unique_ptr<Mesh>> loadFromFile(const std::string path); 

    GameObject(glm::vec3 position, glm::vec3 scale, Handle& vertShaderHandle,
        Handle& fragShaderHandle, std::vector<std::unique_ptr<Mesh>> meshes) :
        StarEntity(),
        meshes(std::move(meshes)),
        vertShader(std::make_unique<Handle>(vertShaderHandle)),
        fragShader(std::make_unique<Handle>(fragShaderHandle))
    {
        this->setScale(scale);
        this->setPosition(position);
    }

    /// <summary>
    /// Create a game object loading the meshes from a file. 
    /// </summary>
    /// <param name="position"></param>
    /// <param name="scale"></param>
    /// <param name="vertShaderHandle"></param>
    /// <param name="fragShaderHandle"></param>
    /// <param name="path"></param>
    GameObject(glm::vec3 position, glm::vec3 scale, Handle& vertShaderHandle,
        Handle& fragShaderHandle, const std::string path) :
        StarEntity(),
        meshes(std::move(meshes)),
        vertShader(std::make_unique<Handle>(vertShaderHandle)),
        fragShader(std::make_unique<Handle>(fragShaderHandle))
    {
        this->setScale(scale);
        this->setPosition(position);
    }

    virtual ~GameObject() {};

    //get the handle for the vertex shader 
    Handle getVertShader() { return *this->vertShader.get(); }
    //get the handle for the fragment shader
    Handle getFragShader() { return *this->fragShader.get(); }
    const std::vector<std::unique_ptr<Mesh>>& getMeshes() { return this->meshes; }
    glm::mat4 getNormalMatrix() { return glm::inverseTranspose(getDisplayMatrix()); }

protected:
    //is the model matrix updated with most recent changes 
    std::unique_ptr<Handle> vertShader, fragShader;
    std::vector<std::unique_ptr<Mesh>> meshes;

private:

};
}
