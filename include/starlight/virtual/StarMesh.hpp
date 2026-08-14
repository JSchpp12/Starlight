#pragma once

#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarMaterial.hpp"
#include "Vertex.hpp"
#include "core/device/DeviceContext.hpp"
#include <star_common/Handle.hpp>
#include <star_common/helper/CastHelpers.hpp>

#include <vulkan/vulkan.hpp>

#include <array>

namespace star
{
class StarMesh
{
  public:
    StarMesh(const Handle &vertBuffer, const Handle &indBuffer, std::vector<Vertex> &vertices,
             std::vector<uint32_t> &indices, std::shared_ptr<StarMaterial> material, bool hasAdjacenciesPacked);

    StarMesh(const Handle &vertBuffer, const Handle &indBuffer, std::vector<Vertex> &vertices,
             std::vector<uint32_t> &indices, std::shared_ptr<StarMaterial> material, const glm::vec3 &boundBoxMinCoord,
             const glm::vec3 &boundBoxMaxCoord, bool packAdjacencies = false);

    /// Construct with a precomputed bounding box and explicit vertex/index counts,
    /// without needing the host vertex data. Use when vertices are uploaded as a
    /// compact, non-Vertex host type (e.g. a terrain-specific vertex format) and the
    /// bounding box has already been computed by the caller.
    StarMesh(const Handle &vertBuffer, const Handle &indBuffer, uint32_t vertexCount, uint32_t indexCount,
             std::shared_ptr<StarMaterial> material, const glm::vec3 &boundBoxMinCoord,
             const glm::vec3 &boundBoxMaxCoord, bool hasAdjacenciesPacked = false);

    virtual ~StarMesh() = default;

    virtual void prepRender(core::device::DeviceContext &device);

    virtual void recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
                                          const uint8_t &frameInFlightIndex, const uint32_t &instanceCount,
                                          uint32_t descriptorSetStartIndex = 0);

    bool isKnownToBeReady(const uint8_t &frameInFlightIndex);

    void registerTransferWaits(core::device::DeviceContext &context, Handle commandBuffer);

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
    Handle m_deviceID;
    Handle vertBuffer, indBuffer;
    glm::vec3 aaboundingBoxBounds[2];
    std::shared_ptr<StarMaterial> material;
    uint32_t numVerts = 0, numInds = 0;
    bool hasAdjacenciesPacked = false;
    bool triangular = false;
    bool isReady = false;
};
} // namespace star