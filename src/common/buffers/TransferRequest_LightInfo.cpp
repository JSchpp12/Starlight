#include "TransferRequest_LightInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(LightBufferObject) * CastHelpers::size_t_to_unsigned_int(this->myLights.size()))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "LightInfo_Stage")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->myLights.size()))
        .setInstanceSize(sizeof(LightBufferObject))
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = std::vector<uint32_t>{this->graphicsQueueFamilyIndex};
	for (const auto &index : transferQueueFamilyIndex)
		indices.push_back(index);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setPQueueFamilyIndices(indices.data())
                .setQueueFamilyIndexCount(indices.size())
                .setSize(sizeof(LightBufferObject) * CastHelpers::size_t_to_unsigned_int(this->myLights.size()))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer),
            "LightInfo_Src")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->myLights.size()))
        .setInstanceSize(sizeof(LightBufferObject))
        .build();
}

void star::TransferRequest::LightInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
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