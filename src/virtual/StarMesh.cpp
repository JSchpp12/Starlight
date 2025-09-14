#include "StarMesh.hpp"

void star::StarMesh::prepRender(star::core::device::DeviceContext &context)
{
    m_deviceID = context.getDeviceID(); 

    this->material->prepRender(context);
}

bool star::StarMesh::isKnownToBeReady(const uint8_t &frameInFlightIndex)
{
    if (this->isReady)
        return true;

    // need to also check if vert + ind buffers are ready
    if (ManagerRenderResource::isReady(m_deviceID, this->vertBuffer) && ManagerRenderResource::isReady(m_deviceID, this->indBuffer) &&
        this->material->isKnownToBeReady(frameInFlightIndex))
    {
        this->isReady = true;
        return true;
    }

    return false;
}

void star::StarMesh::recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
                                              uint8_t &frameInFlightIndex, const uint32_t &instanceCount)
{

    this->material->bind(commandBuffer, pipelineLayout, frameInFlightIndex);

    vk::DeviceSize offset{0};
    auto &vBuff = ManagerRenderResource::getBuffer(m_deviceID, this->vertBuffer);
    auto &iBuff = ManagerRenderResource::getBuffer(m_deviceID, this->indBuffer);
    commandBuffer.bindVertexBuffers(0, vBuff.getVulkanBuffer(), offset);
    commandBuffer.bindIndexBuffer(iBuff.getVulkanBuffer(), offset, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(this->numInds, instanceCount, 0, 0, 0);
}

void star::StarMesh::CalcBoundingBox(const std::vector<star::Vertex> &verts, glm::vec3 &upperBoundingBoxCoord, glm::vec3 &lowerBoundingBoxCoord){
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