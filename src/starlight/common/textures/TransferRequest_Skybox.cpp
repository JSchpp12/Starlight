#include "TransferRequest_Skybox.hpp"

#include "starlight/core/Exceptions.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"
#include "starlight/maker/texture/MakeTexture.hpp"
#include "starlight/util/io/ReadExr.hpp"
#include "starlight/wrappers/Allocator.hpp"

#include <cassert>
#include <string>
#include <utility>

namespace star::TransferRequest
{
static constexpr uint32_t CubeFaceCount = 6;

static std::string GetTextureName(const SkyboxInfo &info)
{
    return info.name.empty() ? "skybox" : info.name;
}

static vk::Format SelectFormat(const SkyboxLayout &layout, const SkyboxFormatScaling &formatScaling)
{
    switch (layout)
    {
    case SkyboxLayout::Equirect:
        // Equirectangular skyboxes are sourced from HDR data, so the format is always floating point.
        switch (formatScaling)
        {
        case SkyboxFormatScaling::Full:
            return vk::Format::eR32G32B32A32Sfloat;
        case SkyboxFormatScaling::Half:
            return vk::Format::eR16G16B16A16Sfloat;
        }
        break;
    }

    return vk::Format::eUndefined;
}

Skybox::Skybox(std::shared_ptr<std::future<SkyboxInfo>> skyboxInfo, uint32_t graphicsQueueFamilyIndex)
    : m_skyboxInfo(std::move(skyboxInfo)), m_graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
{
}

void Skybox::prep()
{
    if (!m_info.has_value())
        m_info = m_skyboxInfo->get();

    if (m_info->name.empty())
        STAR_THROW("Skybox info is missing name");
}

std::string Skybox::getDebugDescription() const
{
    return m_info.has_value() ? GetTextureName(*m_info) : "skybox";
}

std::unique_ptr<StarBuffers::Buffer> Skybox::createStagingBuffer(core::device::StarDevice &device) const
{
    assert(m_info.has_value() && "prep() must resolve the skybox info before createStagingBuffer()");

    const auto exrInfo = io::ReadExr(m_info->name).getInfo();
    if (!exrInfo)
        STAR_THROW(exrInfo.error().what());

    // LoadEXR always expands the file to four float channels per pixel, so the staging buffer must match that
    // layout rather than the file's native channel width.
    const vk::DeviceSize size = static_cast<vk::DeviceSize>(exrInfo->resolution.x) *
                                static_cast<vk::DeviceSize>(exrInfo->resolution.y) *
                                static_cast<vk::DeviceSize>(4u * sizeof(float));

    StarBuffers::Buffer builtBuffer;
    const auto makeResult = maker::buffer::MakeBuffer(device)
                                .setCreateInfo(Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(size)
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                                               GetTextureName(*m_info) + "_SkyboxTransferSRCBuffer")
                                .setInstanceCount(1)
                                .setInstanceSize(size)
                                .make(&builtBuffer, 1);
    if (!makeResult)
        STAR_THROW("Failed to create staging buffer for skybox: " + makeResult.error().detail);

    return std::make_unique<StarBuffers::Buffer>(std::move(builtBuffer));
}

std::unique_ptr<StarTextures::Texture> Skybox::createFinal(core::device::StarDevice &device,
                                                           const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    assert(m_info.has_value() && "prep() must resolve the skybox info before createFinal()");

    const SkyboxTextureOptions &options = m_info->textureOptions;
    if (options.faceSize == 0)
        STAR_THROW("Skybox texture options are incomplete: face_size is required");

    const vk::Format format = SelectFormat(m_info->layout, options.formatScaling);
    if (format == vk::Format::eUndefined)
        STAR_THROW("Unsupported skybox layout for texture creation");

    std::vector<uint32_t> indices{this->m_graphicsQueueFamilyIndex};
    indices.reserve(transferQueueFamilyIndex.size() + 1);
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    const vk::ImageViewCreateInfo cubeView =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::eCube)
            .setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(CubeFaceCount)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1));

    const vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
                                                  .setMagFilter(vk::Filter::eLinear)
                                                  .setMinFilter(vk::Filter::eLinear)
                                                  .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                                  .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                                  .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                                  .setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
                                                  .setUnnormalizedCoordinates(VK_FALSE)
                                                  .setCompareEnable(VK_FALSE)
                                                  .setCompareOp(vk::CompareOp::eAlways)
                                                  .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                                  .setMipLodBias(0.0f)
                                                  .setMinLod(0.0f)
                                                  .setMaxLod(0.0f);

    StarTextures::Texture texture;
    const auto makeResult =
        maker::texture::MakeTexture(device)
            .setCreateInfo(
                Allocator::AllocationBuilder()
                    .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                    .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                    .build(),
                vk::ImageCreateInfo()
                    .setExtent(vk::Extent3D().setWidth(options.faceSize).setHeight(options.faceSize).setDepth(1))
                    .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
                    .setImageType(vk::ImageType::e2D)
                    .setMipLevels(1)
                    .setArrayLayers(CubeFaceCount)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setSharingMode(vk::SharingMode::eConcurrent)
                    .setPQueueFamilyIndices(indices.data())
                    .setQueueFamilyIndexCount(static_cast<uint32_t>(indices.size())),
                GetTextureName(*m_info) + "_SkyboxTexture")
            .setBaseFormat(format)
            .addViewInfo(cubeView)
            .setSamplerInfo(samplerInfo)
            .make(&texture, 1);
    if (!makeResult)
        STAR_THROW("Failed to create skybox texture: " + makeResult.error().detail);

    return std::make_unique<StarTextures::Texture>(std::move(texture));
}

void Skybox::copyFromTransferSRCToDST(StarBuffers::Buffer &, StarTextures::Texture &, vk::CommandBuffer &) const
{
}

void Skybox::writeDataToStageBuffer(StarBuffers::Buffer &stagingBuffer) const
{
    assert(m_info.has_value() && "prep() must resolve the skybox info before writeDataToStageBuffer()");

    auto exrData = io::ReadExr(m_info->name).read();
    if (!exrData)
        STAR_THROW(exrData.error().what());

    const auto pixels = exrData->data();

    void *mapped = nullptr;
    stagingBuffer.map(&mapped);
    stagingBuffer.writeToBuffer(pixels.data(), mapped, static_cast<vk::DeviceSize>(pixels.size_bytes()));
    stagingBuffer.unmap();
}
} // namespace star::TransferRequest
