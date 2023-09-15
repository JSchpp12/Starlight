#pragma once
#include "Enums.hpp"
#include "Handle.hpp"
#include "Triangle.hpp"

#include <glm/glm.hpp>

#include <vector> 
#include <memory> 

namespace star {
class Mesh {
public:
    class Builder {
    public:
        Builder() = default;
        Builder& setTriangles(std::unique_ptr<std::vector<Triangle>> triangles) {
            this->triangles = std::move(triangles);
            return *this;
        }
        Builder& setMaterial(Handle material) {
            this->material = material;
            return *this;
        }
        std::unique_ptr<Mesh> build() {
            assert(this->triangles && "Triangles must be provided to a mesh!");

            return std::make_unique<Mesh>(std::move(this->triangles), this->material);
        }

    private:
        std::unique_ptr<std::vector<Triangle>> triangles;
        Handle material = Handle::getDefault();
    };

    Mesh(std::unique_ptr<std::vector<Triangle>> triangles, Handle material) :
        triangles(std::move(triangles)), material(material) { };

    const std::unique_ptr<std::vector<Triangle>>& getTriangles() { return this->triangles; }
    Handle getMaterial() { return this->material; }

private:
    std::unique_ptr<std::vector<Triangle>> triangles;
    //TODO: add this back -- along with moving an image to the material
    Handle material;
};
}