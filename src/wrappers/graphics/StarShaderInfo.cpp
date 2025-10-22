#include "StarShaderInfo.hpp"

#include "ManagerDescriptorPool.hpp"

vk::DescriptorSet star::StarShaderInfo::ShaderInfoSet::getDescriptorSet()
{
    if (this->setNeedsRebuild)
    {
        rebuildSet();
    }

    return *this->descriptorSet;
}

void star::StarShaderInfo::ShaderInfoSet::add(const star::StarShaderInfo::ShaderInfo &shaderInfo)
{
    this->shaderInfos.push_back(shaderInfo);
}

void star::StarShaderInfo::ShaderInfoSet::buildIndex(const star::Handle &deviceID, const int &index)
{
    assert(this->descriptorWriter && "Dependencies must have been built first");
    this->setNeedsRebuild = true;

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
        if (shaderInfos[index].textureInfo.value().texture.has_value())
            texture = shaderInfos[index].textureInfo.value().texture.value();
        else if (shaderInfos[index].textureInfo.value().handle.has_value())
            texture = &star::ManagerRenderResource::getTexture(deviceID,
                                                               shaderInfos[index].textureInfo.value().handle.value());
        else if (shaderInfos[index].textureInfo.value().texture.has_value())
        {
            texture = shaderInfos[index].textureInfo.value().texture.value();
        }
        else
        {
            throw std::runtime_error("Unknown error regarding texture binding");
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

    this->descriptorWriter = std::make_unique<StarDescriptorWriter>(
        this->device, this->setLayout, core::device::managers::ManagerDescriptorPool::getPool());
    for (size_t i = 0; i < this->shaderInfos.size(); i++)
    {
        buildIndex(deviceID, i);
    }
}

void star::StarShaderInfo::ShaderInfoSet::rebuildSet()
{
    if (!this->descriptorWriter)
    {
        this->descriptorWriter = std::make_unique<StarDescriptorWriter>(
            this->device, this->setLayout, core::device::managers::ManagerDescriptorPool::getPool());
    }

    this->descriptorSet = std::make_shared<vk::DescriptorSet>(this->descriptorWriter->build());
    this->setNeedsRebuild = false;
}

bool star::StarShaderInfo::isReady(const uint8_t &frameInFlight)
{
    assert(frameInFlight <= shaderInfoSets.size());

    for (const auto &set : this->shaderInfoSets[frameInFlight])
    {
        for (size_t i = 0; i < set->shaderInfos.size(); i++)
        {
            if (set->shaderInfos.at(i).m_willCheckForIfReady)
            {
                if (set->shaderInfos.at(i).bufferInfo.has_value() &&
                    set->shaderInfos.at(i).bufferInfo.value().handle.has_value())
                {
                    if (!ManagerRenderResource::isReady(m_deviceID,
                                                        set->shaderInfos.at(i).bufferInfo.value().handle.value()))
                        return false;
                }
                else if (set->shaderInfos.at(i).textureInfo.has_value())
                {
                    if (set->shaderInfos.at(i).textureInfo.value().handle.has_value() &&
                        !ManagerRenderResource::isReady(m_deviceID,
                                                        set->shaderInfos.at(i).textureInfo.value().handle.value()))
                    {
                        return false;
                    }
                }
                else
                {
                    throw std::runtime_error("Unknown shader info encountered while checking for ready status");
                }
            }
        }
    }

    return true;
}

// std::set<vk::Semaphore> star::StarShaderInfo::getDependentSemaphores(const uint8_t &frameInFlight) const
// {
//     assert(frameInFlight <= shaderInfoSets.size());

//     std::set<vk::Semaphore> semaphores;

//     for (const auto &set : shaderInfoSets[frameInFlight])
//     {
//         for (size_t i = 0; i < set->shaderInfos.size(); i++)
//         {
//             if (!set->shaderInfos.at(i).m_willCheckForIfReady)
//             {
//                 if (set->shaderInfos.at(i).bufferInfo.has_value() || set->shaderInfos.at(i).textureInfo.has_value())
//                 {
//                     semaphores.insert(set->shaderInfos.at(i).m_resourceSemaphore);
//                 }
//                 else
//                 {
//                     throw std::runtime_error("Unknown shader info encountered while gathering semaphores");
//                 }
//             }
//         }
//     }

//     return semaphores;
// }

std::vector<vk::DescriptorSetLayout> star::StarShaderInfo::getDescriptorSetLayouts()
{
    std::vector<vk::DescriptorSetLayout> fLayouts = std::vector<vk::DescriptorSetLayout>();
    for (auto &set : this->layouts)
    {
        fLayouts.push_back(set->getDescriptorSetLayout());
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

std::vector<vk::DescriptorSet> star::StarShaderInfo::getDescriptors(const int &frameInFlight)
{
    for (auto &set : this->shaderInfoSets[frameInFlight])
    {
        if (!set->getIsBuilt())
            set->build(m_deviceID);
        else
        {
            for (size_t i = 0; i < set->shaderInfos.size(); i++)
            {
                if (set->shaderInfos.at(i).bufferInfo.has_value() &&
                    set->shaderInfos.at(i).bufferInfo.value().handle.has_value())
                {
                    // check if buffer has changed
                    auto &info = set->shaderInfos.at(i).bufferInfo.value();
                    auto &handle = set->shaderInfos.at(i).bufferInfo.value().handle.value();

                    const auto &buffer = ManagerRenderResource::getBuffer(m_deviceID, handle).getVulkanBuffer();
                    if (!info.currentBuffer.has_value() || info.currentBuffer.value() != buffer)
                    {
                        info.currentBuffer = buffer;
                        set->buildIndex(m_deviceID, i);
                    }
                }
                else if (set->shaderInfos.at(i).textureInfo.has_value() &&
                         set->shaderInfos.at(i).textureInfo.value().handle.has_value())
                {
                    auto &info = set->shaderInfos.at(i).textureInfo.value();

                    const auto &texture =
                        ManagerRenderResource::getTexture(m_deviceID, info.handle.value()).getVulkanImage();
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
    for (size_t i = 0; i < this->shaderInfoSets[frameInFlight].size(); i++)
    {
        allSets.push_back(this->shaderInfoSets[frameInFlight].at(i)->getDescriptorSet());
    }

    return allSets;
}

star::StarShaderInfo::Builder &star::StarShaderInfo::Builder::startSet()
{
    auto size = this->activeSet->size();
    assert(size < this->layouts.size() && "Pushed beyond size defined in set layout");

    this->activeSet->push_back(std::make_shared<ShaderInfoSet>(this->device, *this->layouts[size]));
    return *this;
}