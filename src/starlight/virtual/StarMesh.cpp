#include "StarMesh.hpp"

namespace star
{
static void CalcBoundingBox(const std::vector<star::Vertex> &verts, glm::vec3 &upperBoundingBoxCoord,
                            glm::vec3 &lowerBoundingBoxCoord)
{
    glm::vec3 max{}, min{};

    // calcualte bounding box info
    for (size_t i = 1; i < verts.size(); i++)
    {
        if (verts.at(i).pos.x < min.x)
            min.x = verts.at(i).pos.x;
        if (verts.at(i).pos.y < min.y)
            min.y = verts.at(i).pos.y;
        if (verts.at(i).pos.z < min.z)
            min.z = verts.at(i).pos.z;

        if (verts.at(i).pos.x > max.x)
            max.x = verts.at(i).pos.x;
        if (verts.at(i).pos.y > max.y)
            max.y = verts.at(i).pos.y;
        if (verts.at(i).pos.z > max.z)
            max.z = verts.at(i).pos.z;
    }

    lowerBoundingBoxCoord = min;
    upperBoundingBoxCoord = max;
}

StarMesh::StarMesh(const Handle &vertBuffer, const Handle &indBuffer, std::vector<Vertex> &vertices,
                   std::vector<uint32_t> &indices, std::shared_ptr<StarMaterial> material, bool hasAdjacenciesPacked)
    : material(std::move(material)), hasAdjacenciesPacked(hasAdjacenciesPacked), triangular(indices.size() % 3 == 0),
      numVerts(star::common::casts::size_t_to_unsigned_int(vertices.size())),
      numInds(star::common::casts::size_t_to_unsigned_int(indices.size())), vertBuffer(vertBuffer), indBuffer(indBuffer)
{
    CalcBoundingBox(vertices, this->aaboundingBoxBounds[1], this->aaboundingBoxBounds[0]);
}

StarMesh::StarMesh(const Handle &vertBuffer, const Handle &indBuffer, std::vector<Vertex> &vertices,
                   std::vector<uint32_t> &indices, std::shared_ptr<StarMaterial> material,
                   const glm::vec3 &boundBoxMinCoord, const glm::vec3 &boundBoxMaxCoord, bool packAdjacencies)
    : material(std::move(material)), hasAdjacenciesPacked(packAdjacencies), triangular(indices.size() % 3 == 0),
      aaboundingBoxBounds{boundBoxMinCoord, boundBoxMaxCoord},
      numVerts(star::common::casts::size_t_to_unsigned_int(vertices.size())),
      numInds(star::common::casts::size_t_to_unsigned_int(indices.size())), vertBuffer(vertBuffer), indBuffer(indBuffer)
{
    CalcBoundingBox(vertices, this->aaboundingBoxBounds[1], this->aaboundingBoxBounds[0]);
}

void star::StarMesh::prepRender(star::core::device::DeviceContext &context)
{
    m_deviceID = context.getDeviceID();
}

bool star::StarMesh::isKnownToBeReady(const uint8_t &frameInFlightIndex)
{
    if (this->isReady)
        return true;

    // need to also check if vert + ind buffers are ready
    if (ManagerRenderResource::isReady(m_deviceID, this->vertBuffer) &&
        ManagerRenderResource::isReady(m_deviceID, this->indBuffer) &&
        this->material->isKnownToBeReady(frameInFlightIndex))
    {
        this->isReady = true;
        return true;
    }

    return false;
}

void star::StarMesh::recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
                                              const uint8_t &frameInFlightIndex, const uint32_t &instanceCount)
{

    this->material->bind(commandBuffer, pipelineLayout, frameInFlightIndex);

    vk::DeviceSize offset{0};
    auto &vBuff = ManagerRenderResource::getBuffer(m_deviceID, this->vertBuffer);
    auto &iBuff = ManagerRenderResource::getBuffer(m_deviceID, this->indBuffer);
    commandBuffer.bindVertexBuffers(0, vBuff.getVulkanBuffer(), offset);
    commandBuffer.bindIndexBuffer(iBuff.getVulkanBuffer(), offset, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(this->numInds, instanceCount, 0, 0, 0);
}
} // namespace star