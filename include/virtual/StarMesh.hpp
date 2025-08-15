#pragma once

#include "CastHelpers.hpp"
#include "GeometryHelpers.hpp"
#include "Handle.hpp"
#include "ManagerRenderResource.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "DeviceContext.hpp"
#include "StarMaterial.hpp"
#include "Vertex.hpp"

#include <vulkan/vulkan.hpp>

#include <array>

namespace star
{
class StarMesh
{
  public:
    StarMesh(const Handle &vertBuffer, const Handle &indBuffer, std::vector<Vertex> &vertices,
             std::vector<uint32_t> &indices, std::shared_ptr<StarMaterial> material, bool hasAdjacenciesPacked)
        : material(std::move(material)), hasAdjacenciesPacked(hasAdjacenciesPacked),
          triangular(indices.size() % 3 == 0), numVerts(CastHelpers::size_t_to_unsigned_int(vertices.size())),
          numInds(CastHelpers::size_t_to_unsigned_int(indices.size())), vertBuffer(vertBuffer), indBuffer(indBuffer)
    {

        CalcBoundingBox(vertices, this->aaboundingBoxBounds[1], this->aaboundingBoxBounds[0]);
    }

    StarMesh(const Handle &vertBuffer, const Handle &indBuffer, std::vector<Vertex> &vertices,
             std::vector<uint32_t> &indices, std::shared_ptr<StarMaterial> material, const glm::vec3 &boundBoxMinCoord,
             const glm::vec3 &boundBoxMaxCoord, bool packAdjacencies = false)
        : material(std::move(material)), hasAdjacenciesPacked(packAdjacencies), triangular(indices.size() % 3 == 0),
          aaboundingBoxBounds{boundBoxMinCoord, boundBoxMaxCoord},
          numVerts(CastHelpers::size_t_to_unsigned_int(vertices.size())),
          numInds(CastHelpers::size_t_to_unsigned_int(indices.size())), vertBuffer(vertBuffer), indBuffer(indBuffer)
    {
        CalcBoundingBox(vertices, this->aaboundingBoxBounds[1], this->aaboundingBoxBounds[0]);
    }

    virtual ~StarMesh() = default;

    virtual void prepRender(core::DeviceContext &device);

    virtual void recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
                                          int &frameInFlightIndex, const uint32_t &instanceCount);

    bool isKnownToBeReady(const uint8_t &frameInFlightIndex);

    StarMaterial &getMaterial()
    {
        return *this->material;
    }
    bool hasAdjacentVertsPacked() const
    {
        return this->hasAdjacenciesPacked;
    }
    bool isTriangular() const
    {
        return this->triangular;
    }
    std::array<glm::vec3, 2> getBoundingBoxCoords() const
    {
        return std::array<glm::vec3, 2>{this->aaboundingBoxBounds[0], this->aaboundingBoxBounds[1]};
    }

    uint32_t getNumVerts() const
    {
        return this->numVerts;
    }
    uint32_t getNumIndices() const
    {
        return this->numInds;
    }

  protected:
    Handle vertBuffer, indBuffer;
    bool hasAdjacenciesPacked = false;
    bool triangular = false;
    glm::vec3 aaboundingBoxBounds[2];
    std::shared_ptr<StarMaterial> material;
    uint32_t numVerts = 0, numInds = 0;
    bool isReady = false;

    static void CalcBoundingBox(const std::vector<Vertex> &verts, glm::vec3 &upperBoundingBoxCoord,
                                glm::vec3 &lowerBoundingBoxCoord);
};
} // namespace star