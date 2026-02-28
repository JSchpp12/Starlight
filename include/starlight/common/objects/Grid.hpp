#pragma once

#include "Color.hpp"
#include "ConfigFile.hpp"
#include "HeightDisplacementMaterial.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarMesh.hpp"
#include "StarObject.hpp"
#include "VertColorMaterial.hpp"
#include "Vertex.hpp"

#include <memory>

namespace star
{
/// <summary>
/// Grid object which will programmatically generate vertices as needed.
/// </summary>
class Grid : public StarObject
{
  public:
    virtual ~Grid();

    Grid(int vertX, int vertY);

    Grid(int vertX, int vertY, std::shared_ptr<StarMaterial> material);

    std::optional<glm::vec2> getXYCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head);

    int getSizeX()
    {
        return this->vertX;
    }
    int getSizeY()
    {
        return this->vertY;
    }

  protected:
    int vertX = 0, vertY = 0;
    int width = 0;

    virtual void loadGeometry(std::unique_ptr<std::vector<Vertex>> &verts,
                              std::unique_ptr<std::vector<uint32_t>> &inds);

    virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;
};
} // namespace star