#pragma once
#include "Enums.hpp"
#include "StarMaterial.hpp"
#include "Triangle.hpp"

#include <glm/glm.hpp>

#include <vector> 
#include <memory> 

namespace star {
class Mesh {
public:
    Mesh(std::unique_ptr<std::vector<Triangle>> triangles, std::unique_ptr<StarMaterial> material) :
        triangles(std::move(triangles)), material(std::move(material)) { };

    std::vector<Triangle>& getTriangles() { return *this->triangles; }
    StarMaterial& getMaterial() { return *this->material; }

private:
    std::unique_ptr<std::vector<Triangle>> triangles;
    //TODO: add this back -- along with moving an image to the material
    std::unique_ptr<StarMaterial> material;

};
}