#include "StarShaderInfo.hpp"

#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/Image.hpp"
#include "core/logging/LoggingFactory.hpp"

/// @brief Whether this handle refers to a slot in the device's Buffer manager (FrameData::BufferHandle)
/// rather than a transfer-uploaded resource tracked by ManagerRenderResource.
/// @details The two systems mint different handle types; the device manager resolves its own handles through the
/// manager at use time so it never depends on where the manager's record storage lives.
static bool IsDeviceBufferManagerHandle(const star::Handle &handle)
{
    return handle.isInitialized() &&
           handle.getType() == star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                   star::core::device::manager::GetBufferHandleTypeName);
}

/// @brief Whether this handle refers to a slot in the device's Image manager (FrameData::TextureHandle)
/// rather than a transfer-uploaded texture tracked by ManagerRenderResource.
/// @details Same split as the buffer manager: image records live in manager-owned storage that may move, so the
/// image manager resolves its own handles at use time.
static bool IsDeviceImageManagerHandle(const star::Handle &handle)
{
    return handle.isInitialized() &&
           handle.getType() == star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                   star::common::special_types::GetImageTypeName);
}

vk::DescriptorSet star::StarShaderInfo::ShaderInfoSet::getDescriptorSet(const Handle &deviceID)
{
    while (!m_pendingBuildIndices.empty())
    {
        auto &ele = m_pendingBuildIndices.back();
        buildIndex(deviceID, ele);
        m_pendingBuildIndices.pop_back();
        setNeedsRebuild = true;
    }

    if (this->setNeedsRebuild)
    {
        rebuildSet();
    }

    return *this->descriptorSet;
}

void star::StarShaderInfo::ShaderInfoSet::setNewResource(size_t index, ShaderInfo newInfo)
{
    m_pendingBuildIndices.emplace_back(static_cast<uint32_t>(index));

    this->shaderInfos[index] = std::move(newInfo);
}

void star::StarShaderInfo::ShaderInfoSet::add(const star::StarShaderInfo::ShaderInfo &shaderInfo)
{
    this->shaderInfos.push_back(shaderInfo);
}

void star::StarShaderInfo::ShaderInfoSet::buildIndex(const star::Handle &deviceID, size_t index)
{
    assert(this->descriptorWriter && "Dependencies must have been built first");

    if (shaderInfos[index].bufferInfo.has_value())
    {
        const auto &info = shaderInfos[index].bufferInfo.value();
        if (info.handle.has_value())
        {
            if (IsDeviceBufferManagerHandle(info.handle.value()))
            {
                assert(this->m_bufferManager != nullptr &&
                       "Device-managed buffer handle requires the buffer manager to be provided by the builder");
                const auto *record = this->m_bufferManager->get(info.handle.value());
                const auto *buffer = record != nullptr ? record->getBuffer() : nullptr;
                if (buffer == nullptr || buffer->getVulkanBuffer() == VK_NULL_HANDLE)
                {
                    // The producer failed and the manager handed back a blank buffer. Skip the write; consumers are
                    // expected to tolerate the unbound resource.
                    core::logging::warning(
                        "Buffer manager slot is blank; skipping descriptor write for a failed transfer");
                    return;
                }

                auto bufferInfo = vk::DescriptorBufferInfo{buffer->getVulkanBuffer(), 0, buffer->getBufferSize()};
                this->descriptorWriter->writeBuffer(index, bufferInfo);
            }
            else
            {
                if (!ManagerRenderResource::isReady(deviceID, info.handle.value()))
                {
                    ManagerRenderResource::waitForReady(deviceID, info.handle.value());
                }
                const auto &buffer = ManagerRenderResource::getBuffer(deviceID, info.handle.value());

                auto bufferInfo = vk::DescriptorBufferInfo{buffer.getVulkanBuffer(), 0, buffer.getBufferSize()};
                this->descriptorWriter->writeBuffer(index, bufferInfo);
            }
        }
        else if (info.buffer.has_value())
        {
            auto bufferInfo = vk::DescriptorBufferInfo{info.buffer.value()->getVulkanBuffer(), 0,
                                                       info.buffer.value()->getBufferSize()};
            this->descriptorWriter->writeBuffer(index, bufferInfo);
        }
        else
        {
            std::cerr << "Invalid buffer info provided to shader builder" << std::endl;
            throw std::runtime_error("Invalid buffer info provided to shader builder");
        }
    }
    else if (shaderInfos[index].textureInfo.has_value())
    {
        const auto &texInfo = shaderInfos[index].textureInfo.value();
        const StarTextures::Texture *texture = nullptr;
        if (texInfo.texture != nullptr)
        {
            texture = texInfo.texture;
        }
        else if (texInfo.handle.isInitialized())
        {
            if (IsDeviceImageManagerHandle(texInfo.handle))
            {
                assert(this->m_imageManager != nullptr &&
                       "Image-manager handle requires the image manager to be provided by the builder");
                const auto *record = this->m_imageManager->get(texInfo.handle);
                assert(record != nullptr && "Image must be resolved before the descriptor set is written");
                texture = &record->texture;
            }
            else
            {
                texture = &star::ManagerRenderResource::getTexture(deviceID, texInfo.handle);
            }
        }
        else
        {
            STAR_THROW("Unknown error regarding texture binding");
        }
        assert(texture != nullptr && "Texture cannot be found");

        vk::DescriptorImageInfo textureInfo;
        textureInfo.sampler = texture->getSampler().has_value() ? texture->getSampler().value() : VK_NULL_HANDLE;
        textureInfo.imageLayout = shaderInfos[index].textureInfo.value().expectedLayout;
        const auto &views = texture->getImageView();
        const auto &requestedFormat = shaderInfos[index].textureInfo.value().requestedImageViewFormat;
        textureInfo.imageView = views.get(requestedFormat.has_value() ? *requestedFormat : texture->getBaseFormat());

        if (!textureInfo.imageView)
        {
            // Failed/blank texture, or a missing view for the requested format. Skip the write; consumers are expected
            // to tolerate the unbound resource.
            core::logging::warning("Texture has no view for the requested format; skipping descriptor write");
            return;
        }

        this->descriptorWriter->writeImage(index, textureInfo);
    }
}

void star::StarShaderInfo::ShaderInfoSet::build(const star::Handle &deviceID)
{
    this->isBuilt = true;

    StarDescriptorPool *pool = nullptr;
    this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->setLayout, m_pool);
    for (size_t i = 0; i < this->shaderInfos.size(); i++)
    {
        buildIndex(deviceID, i);
    }
}

void star::StarShaderInfo::ShaderInfoSet::rebuildSet()
{
    if (!this->descriptorWriter)
    {
        this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->setLayout, m_pool);
    }

    this->descriptorSet = std::make_shared<vk::DescriptorSet>(this->descriptorWriter->build());
    this->setNeedsRebuild = false;
}

bool star::StarShaderInfo::isReady(uint8_t frameInFlight)
{
    for (const auto &set : this->shaderInfoSets[static_cast<size_t>(frameInFlight)])
    {
        for (size_t i{0}; i < set->shaderInfos.size(); i++)
        {
            const auto &shaderInfo = set->shaderInfos[i];

            if (shaderInfo.bufferInfo.has_value())
            {
                const auto &info = shaderInfo.bufferInfo.value();
                if (info.handle.has_value())
                {
                    if (IsDeviceBufferManagerHandle(info.handle.value()))
                    {
                        // A failed producer leaves a blank buffer, but readiness must stay true: reporting false here
                        // would spin the engine's scene-ready wait loop forever. Consumers check the resource.
                        if (set->getBufferManager() == nullptr ||
                            !set->getBufferManager()->get(info.handle.value())->isReady())
                            return false;
                    }
                    else if (!ManagerRenderResource::isReady(m_deviceID, info.handle.value()))
                        return false;
                }
                // Buffer infos without a handle point at an already-created buffer and are ready by construction.
            }
            else if (shaderInfo.textureInfo.has_value())
            {
                const auto &info = shaderInfo.textureInfo.value();
                // Image-manager records are ready by construction; only transfer-uploaded textures can still be
                // pending.
                if (info.handle.isInitialized() && !IsDeviceImageManagerHandle(info.handle) &&
                    !ManagerRenderResource::isReady(m_deviceID, info.handle))
                {
                    return false;
                }
            }
            else
            {
                STAR_THROW("Unknown shader info encountered while checking for ready status");
            }
        }
    }

    return true;
}

std::vector<vk::DescriptorSetLayout> star::StarShaderInfo::getDescriptorSetLayouts()
{
    std::vector<vk::DescriptorSetLayout> fLayouts = std::vector<vk::DescriptorSetLayout>(layouts.size());
    for (size_t i{0}; i < fLayouts.size(); i++)
    {
        fLayouts[i] = layouts[i]->getDescriptorSetLayout();
    }

    return fLayouts;
}

bool star::StarShaderInfo::isSetLayoutCompatible(size_t setIndex, const StarShaderInfo &other,
                                                 size_t otherSetIndex) const
{
    assert(setIndex < layouts.size() && "setIndex out of range for this StarShaderInfo");
    assert(otherSetIndex < other.layouts.size() && "otherSetIndex out of range for other StarShaderInfo");
    return layouts[setIndex]->isCompatibleWith(*other.layouts[otherSetIndex]);
}

void star::StarShaderInfo::cleanupRender(core::device::StarDevice &device)
{
    for (auto &set : this->layouts)
    {
        set->cleanupRender(device);
    }
}

size_t star::StarShaderInfo::getNumDescriptorSets(uint8_t frameInFlight) const noexcept
{
    assert(static_cast<size_t>(frameInFlight) < shaderInfoSets.size() &&
           "Requested frameInFlight is beyond size of createdSets");
    return shaderInfoSets[static_cast<size_t>(frameInFlight)].size();
}

void star::StarShaderInfo::getDescriptors(uint8_t frameInFlight, vk::DescriptorSet *data, size_t &numWritten) noexcept
{
    assert(data != nullptr && "Data address to be written needs to be provided");
    assert(static_cast<size_t>(frameInFlight) < shaderInfoSets.size() &&
           "Requested frameInFlight is beyond sizwe of createdSets");

    for (auto &set : this->shaderInfoSets[static_cast<size_t>(frameInFlight)])
    {
        if (!set->getIsBuilt())
            set->build(m_deviceID);
        else
        {
            for (size_t i{0}; i < set->shaderInfos.size(); i++)
            {
                if (set->shaderInfos[i].bufferInfo.has_value() &&
                    set->shaderInfos[i].bufferInfo.value().handle.has_value())
                {
                    auto &info = set->shaderInfos[i].bufferInfo.value();
                    auto &handle = set->shaderInfos[i].bufferInfo.value().handle.value();

                    vk::Buffer buffer{};
                    if (IsDeviceBufferManagerHandle(handle))
                    {
                        assert(
                            set->getBufferManager() != nullptr &&
                            "Device-managed buffer handle requires the buffer manager to be provided by the builder");
                        const auto *record = set->getBufferManager()->get(handle);
                        const auto *managed = record != nullptr ? record->getBuffer() : nullptr;
                        buffer = managed != nullptr ? managed->getVulkanBuffer() : VK_NULL_HANDLE;
                    }
                    else
                    {
                        buffer = ManagerRenderResource::getBuffer(m_deviceID, handle).getVulkanBuffer();
                    }

                    if (buffer == VK_NULL_HANDLE)
                    {
                        // A failed transfer hands back a blank buffer; never write a null buffer into the set.
                        continue;
                    }

                    if (info.currentBuffer == VK_NULL_HANDLE)
                    {
                        info.currentBuffer = buffer;
                    }
                    else if (info.currentBuffer != buffer)
                    {
                        info.currentBuffer = buffer;
                        set->buildIndex(m_deviceID, i);
                    }
                }
                else if (set->shaderInfos[i].textureInfo.has_value() &&
                         set->shaderInfos[i].textureInfo.value().handle.isInitialized())
                {
                    auto &info = set->shaderInfos[i].textureInfo.value();

                    vk::Image image{};
                    if (IsDeviceImageManagerHandle(info.handle))
                    {
                        assert(set->getImageManager() != nullptr &&
                               "Image-manager handle requires the image manager to be provided by the builder");
                        const auto *record = set->getImageManager()->get(info.handle);
                        assert(record != nullptr && "Image must be resolved before the descriptor set is written");
                        image = record->texture.getVulkanImage();
                    }
                    else
                    {
                        image = ManagerRenderResource::getTexture(m_deviceID, info.handle).getVulkanImage();
                    }

                    if (!info.currentImage.has_value())
                    {
                        info.currentImage = image;
                    }
                    else if (info.currentImage.value() != image)
                    {
                        info.currentImage = image;
                        set->buildIndex(m_deviceID, i);
                    }
                }
            }
        }
    }

    for (size_t i{0}; i < this->shaderInfoSets[frameInFlight].size(); i++)
    {
        *data++ = this->shaderInfoSets[frameInFlight][i]->getDescriptorSet(m_deviceID);
        numWritten++;
    }
}

void star::StarShaderInfo::setNewResource(size_t setIndex, size_t bindingIndex, BufferInfo buffer,
                                          std::optional<size_t> frameInFlightIndex)
{
    if (frameInFlightIndex.has_value())
    {
        assert(frameInFlightIndex.value() < shaderInfoSets.size() && "FrameInFlight index is beyond size");
        assert(setIndex < shaderInfoSets[frameInFlightIndex.value()].size() && "Set index is beyond size of sets");
        shaderInfoSets[frameInFlightIndex.value()][setIndex]->setNewResource(bindingIndex, std::move(buffer));
    }
    else
    {
        for (auto &frame : shaderInfoSets)
        {
            assert(setIndex < frame.size() && "Set index is beyond size of sets");
            frame[setIndex]->setNewResource(bindingIndex, std::move(buffer));
        }
    }
}

void star::StarShaderInfo::setNewResource(size_t setIndex, size_t bindingIndex, TextureInfo texture,
                                          std::optional<size_t> frameInFlightIndex)
{
    if (frameInFlightIndex.has_value())
    {
        assert(frameInFlightIndex.value() < shaderInfoSets.size() && "FrameInFlight index is beyond size");
        assert(setIndex < shaderInfoSets[frameInFlightIndex.value()].size() && "Set index is beyond size of sets");
        shaderInfoSets[frameInFlightIndex.value()][setIndex]->setNewResource(bindingIndex, std::move(texture));
    }
    else
    {
        for (auto &frame : shaderInfoSets)
        {
            assert(setIndex < frame.size() && "Set index is beyond size of sets");
            frame[setIndex]->setNewResource(bindingIndex, std::move(texture));
        }
    }
}

star::StarShaderInfo::Builder &star::StarShaderInfo::Builder::startSet()
{
    this->activeSet->push_back(std::make_shared<ShaderInfoSet>(
        this->device, m_pool, *this->layouts[this->activeSet->size()], m_bufferManager, m_imageManager));
    return *this;
}