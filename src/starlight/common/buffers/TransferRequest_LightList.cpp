#include "TransferRequest_LightList.hpp"

#include <star_common/helper/CastHelpers.hpp>

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightList::createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const
{
    uint32_t numLights = 0; 
    common::helper::SafeCast<size_t, uint32_t>(myLights.size(), numLights); 

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(LightBufferObject) * numLights)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "LightList_Stage")
        .setInstanceCount(numLights)
        .setInstanceSize(sizeof(LightBufferObject))
        .buildUnique();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightList::createFinal(vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = std::vector<uint32_t>{this->graphicsQueueFamilyIndex};
	for (const auto &index : transferQueueFamilyIndex)
		indices.push_back(index);

    uint32_t numIndices, numLights; 
    common::helper::SafeCast<size_t, uint32_t>(indices.size(), numIndices); 
    common::helper::SafeCast<size_t, uint32_t>(myLights.size(), numLights);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setPQueueFamilyIndices(indices.data())
                .setQueueFamilyIndexCount(numIndices)
                .setSize(sizeof(LightBufferObject) * numLights)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer),
            "LightInfo")
        .setInstanceCount(numLights)
        .setInstanceSize(sizeof(LightBufferObject))
        .buildUnique();
}

void star::TransferRequest::LightList::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    std::vector<LightBufferObject> lightInformation(this->myLights.size());
    LightBufferObject newBufferObject{};

    for (size_t i = 0; i < this->myLights.size(); i++)
    {
        const Light &currLight = this->myLights.at(i);
        newBufferObject.position = glm::vec4{currLight.getPosition(), 1.0f};
        newBufferObject.direction = currLight.direction;
        newBufferObject.ambient = currLight.getAmbient();
        newBufferObject.diffuse = currLight.getDiffuse();
        newBufferObject.specular = currLight.getSpecular();
        newBufferObject.settings.x = currLight.getEnabled() ? 1 : 0;
        newBufferObject.settings.y = currLight.getType();
        newBufferObject.controls.x = glm::cos(
            glm::radians(currLight.getInnerDiameter())); // represent the diameter of light as the cos of the light
                                                         // (increase shader performance when doing comparison)
        newBufferObject.controls.y = glm::cos(glm::radians(currLight.getOuterDiameter()));
        lightInformation[i] = newBufferObject;
    }

    buffer.writeToBuffer(lightInformation.data(), mapped, sizeof(LightBufferObject) * lightInformation.size());

    buffer.unmap();
}
