#include "StarShaderInfo.hpp"

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
            if (!ManagerRenderResource::isReady(deviceID, info.handle.value()))
            {
                ManagerRenderResource::waitForReady(deviceID, info.handle.value());
            }
            const auto &buffer = ManagerRenderResource::getBuffer(deviceID, info.handle.value());

            auto bufferInfo = vk::DescriptorBufferInfo{buffer.getVulkanBuffer(), 0, buffer.getBufferSize()};
            this->descriptorWriter->writeBuffer(index, bufferInfo);
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
        const StarTextures::Texture *texture = nullptr;
        if (shaderInfos[index].textureInfo.value().texture != nullptr)
            texture = shaderInfos[index].textureInfo.value().texture;
        else if (shaderInfos[index].textureInfo.value().handle.isInitialized())
            texture = &star::ManagerRenderResource::getTexture(deviceID, shaderInfos[index].textureInfo.value().handle);
        else if (shaderInfos[index].textureInfo.value().texture != nullptr)
        {
            texture = shaderInfos[index].textureInfo.value().texture;
        }
        else
        {
            STAR_THROW("Unknown error regarding texture binding");
        }
        assert(texture != nullptr && "Texture cannot be found");

        vk::DescriptorImageInfo textureInfo;
        textureInfo.sampler = texture->getSampler().has_value() ? texture->getSampler().value() : VK_NULL_HANDLE;
        textureInfo.imageLayout = shaderInfos[index].textureInfo.value().expectedLayout;
        textureInfo.imageView =
            shaderInfos[index].textureInfo.value().requestedImageViewFormat.has_value()
                ? texture->getImageView(&shaderInfos[index].textureInfo.value().requestedImageViewFormat.value())
                : texture->getImageView();

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

            if (set->shaderInfos[0].bufferInfo.has_value() &&
                set->shaderInfos[0].bufferInfo.value().handle.has_value())
            {
                if (!ManagerRenderResource::isReady(m_deviceID,
                                                    set->shaderInfos[0].bufferInfo.value().handle.value()))
                    return false;
            }
            else if (set->shaderInfos[0].textureInfo.has_value())
            {
                if (set->shaderInfos[0].textureInfo.value().handle.isInitialized() &&
                    !ManagerRenderResource::isReady(m_deviceID, set->shaderInfos[0].textureInfo.value().handle))
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

void star::StarShaderInfo::cleanupRender(core::device::StarDevice &device)
{
    for (auto &set : this->layouts)
    {
        set->cleanupRender(device);
    }
}

std::vector<vk::DescriptorSet> star::StarShaderInfo::getDescriptors(size_t frameInFlight)
{
    assert(frameInFlight < shaderInfoSets.size() && "Requested frameInFlight is beyond sizwe of createdSets"); 

    for (auto &set : this->shaderInfoSets[frameInFlight])
    {
        if (!set->getIsBuilt())
            set->build(m_deviceID);
        else
        {
            for (size_t i{0}; i < set->shaderInfos.size(); i++)
            {
                if (set->shaderInfos[0].bufferInfo.has_value() &&
                    set->shaderInfos[0].bufferInfo.value().handle.has_value())
                {
                    // check if buffer has changed
                    auto &info = set->shaderInfos[0].bufferInfo.value();
                    auto &handle = set->shaderInfos[0].bufferInfo.value().handle.value();

                    const auto &buffer = ManagerRenderResource::getBuffer(m_deviceID, handle).getVulkanBuffer();
                    if (info.currentBuffer != VK_NULL_HANDLE || info.currentBuffer != buffer)
                    {
                        info.currentBuffer = buffer;
                        set->buildIndex(m_deviceID, i);
                    }
                }
                else if (set->shaderInfos[i].textureInfo.has_value() &&
                         set->shaderInfos[i].textureInfo.value().handle.isInitialized())
                {
                    auto &info = set->shaderInfos[i].textureInfo.value();

                    const auto &texture = ManagerRenderResource::getTexture(m_deviceID, info.handle).getVulkanImage();
                    if (!info.currentImage.has_value() || info.currentImage.value() != texture)
                    {
                        info.currentImage = texture;
                        set->buildIndex(m_deviceID, i);
                    }
                }
            }
        }
    }

    auto allSets = std::vector<vk::DescriptorSet>();
    for (size_t i{0}; i < this->shaderInfoSets[frameInFlight].size(); i++)
    {
        allSets.push_back(this->shaderInfoSets[frameInFlight][i]->getDescriptorSet(m_deviceID));
    }

    return allSets;
}

void star::StarShaderInfo::setNewResource(size_t frameInFlightIndex, size_t setIndex, size_t bindingIndex,
                                          BufferInfo buffer)
{
    assert(frameInFlightIndex < shaderInfoSets.size() && "FrameInFlight index is beyond size"); 
    assert(setIndex < shaderInfoSets[frameInFlightIndex].size() && "Set index is beyond size of sets"); 

    shaderInfoSets[frameInFlightIndex][setIndex]->setNewResource(bindingIndex, std::move(buffer));
}

void star::StarShaderInfo::setNewResource(size_t frameInFlightIndex, size_t setIndex, size_t bindingIndex,
                                          TextureInfo texture)
{
    assert(frameInFlightIndex < shaderInfoSets.size() && "FrameInFlight index is beyond size");
    assert(setIndex < shaderInfoSets[frameInFlightIndex].size() && "Set index is beyond size of sets"); 

    shaderInfoSets[frameInFlightIndex][setIndex]->setNewResource(bindingIndex, std::move(texture));
}

star::StarShaderInfo::Builder &star::StarShaderInfo::Builder::startSet()
{
    this->activeSet->push_back(std::make_shared<ShaderInfoSet>(this->device, m_pool, *this->layouts[this->activeSet->size()]));
    return *this;
}