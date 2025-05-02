#include "StarShaderInfo.hpp"

#include "ManagerRenderResource.hpp"
#include "ManagerDescriptorPool.hpp"

vk::DescriptorSet star::StarShaderInfo::ShaderInfoSet::getDescriptorSet(){
    if (this->setNeedsRebuild){
        rebuildSet();
    }

    return *this->descriptorSet;
}

void star::StarShaderInfo::ShaderInfoSet::add(const star::StarShaderInfo::ShaderInfo& shaderInfo){
    this->shaderInfos.push_back(shaderInfo);
}

void star::StarShaderInfo::ShaderInfoSet::buildIndex(const int& index){
    assert(this->descriptorWriter && "Dependencies must have been built first");
    this->setNeedsRebuild = true; 

    if (shaderInfos[index].bufferInfo.has_value()) {
        const auto& info = shaderInfos[index].bufferInfo.value();
        if (!ManagerRenderResource::isReady(info.handle)){
            ManagerRenderResource::waitForReady(info.handle);
        }
        const auto& buffer = star::ManagerRenderResource::getBuffer(info.handle);

        auto bufferInfo = vk::DescriptorBufferInfo{
            buffer.getVulkanBuffer(),
            0,
            buffer.getBufferSize()
        };
        this->descriptorWriter->writeBuffer(index, bufferInfo);
    }
    else if (shaderInfos[index].textureInfo.has_value()) {
        const StarTexture* texture = nullptr; 
        if (shaderInfos[index].textureInfo.value().texture.has_value())
            texture = shaderInfos[index].textureInfo.value().texture.value();
        else if (shaderInfos[index].textureInfo.value().handle.has_value())
            texture = &star::ManagerRenderResource::getTexture(shaderInfos[index].textureInfo.value().handle.value());
        else if (shaderInfos[index].textureInfo.value().texture.has_value()){
            texture = shaderInfos[index].textureInfo.value().texture.value();
        }else{
            throw std::runtime_error("Unknown error regarding texture binding");
        }


        vk::DescriptorImageInfo textureInfo; 
        if (shaderInfos[index].textureInfo.value().requestedImageViewFormat.has_value()){
            textureInfo = vk::DescriptorImageInfo{
                texture->getSampler(),
                texture->getImageView(&shaderInfos[index].textureInfo.value().requestedImageViewFormat.value()),
                shaderInfos[index].textureInfo.value().expectedLayout
            };
        }else{
            textureInfo = vk::DescriptorImageInfo{
                texture->getSampler(),
                texture->getImageView(),
                shaderInfos[index].textureInfo.value().expectedLayout
            };
        }

        this->descriptorWriter->writeImage(index, textureInfo);
    }
}

void star::StarShaderInfo::ShaderInfoSet::build(){
    this->isBuilt = true; 
    
    this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->layout, ManagerDescriptorPool::getPool());
    for (int i = 0; i < this->shaderInfos.size(); i++) {
        buildIndex(i); 
    }
}

void star::StarShaderInfo::ShaderInfoSet::rebuildSet()
{
    if (!this->descriptorWriter){
        this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->layout, ManagerDescriptorPool::getPool());
    }

    this->descriptorSet = std::make_shared<vk::DescriptorSet>(this->descriptorWriter->build());
    this->setNeedsRebuild = false; 
}

bool star::StarShaderInfo::isReady(const uint8_t &frameInFlight)
{
    for (const auto& set : this->shaderInfoSets[frameInFlight]){
        for (int i = 0; i < set->shaderInfos.size(); i++){
            if (set->shaderInfos.at(i).willCheckForIfReady){
                if (set->shaderInfos.at(i).bufferInfo.has_value()){
                    if (!ManagerRenderResource::isReady(set->shaderInfos.at(i).bufferInfo.value().handle))
                    return false;
                }else if (set->shaderInfos.at(i).textureInfo.has_value()){
                    if (set->shaderInfos.at(i).textureInfo.value().handle.has_value() && !ManagerRenderResource::isReady(set->shaderInfos.at(i).textureInfo.value().handle.value())){
                        return false;
                    }
                }
            }
        }
    }

    return true; 
}

std::vector<vk::DescriptorSetLayout> star::StarShaderInfo::getDescriptorSetLayouts()
{
    std::vector<vk::DescriptorSetLayout> fLayouts = std::vector<vk::DescriptorSetLayout>();
    for (auto& set : this->layouts) {
        fLayouts.push_back(set->getDescriptorSetLayout());
    }
    return fLayouts;
}

std::vector<vk::DescriptorSet> star::StarShaderInfo::getDescriptors(const int & frameInFlight)
{
    for (auto& set : this->shaderInfoSets[frameInFlight]) {
        if (!set->getIsBuilt())
            set->build();
        else{
            for (int i = 0; i < set->shaderInfos.size(); i++) {
                if (set->shaderInfos.at(i).bufferInfo.has_value()) {
                    //check if buffer has changed
                    auto& info = set->shaderInfos.at(i).bufferInfo.value();
                    ManagerRenderResource::waitForReady(info.handle);
                    if (!ManagerRenderResource::isReady(info.handle))
                        ManagerRenderResource::waitForReady(info.handle);
                        
                    const auto& buffer = ManagerRenderResource::getBuffer(info.handle).getVulkanBuffer();
                    if (!info.currentBuffer || info.currentBuffer != buffer){
                        info.currentBuffer = buffer;
                        set->buildIndex(i);
                    }
                }else if (set->shaderInfos.at(i).textureInfo.value().handle.has_value()){
                    auto& info = set->shaderInfos.at(i).textureInfo.value();

                    const auto& texture = ManagerRenderResource::getTexture(info.handle.value()).getImage();
                    if (!info.currentImage.has_value() || info.currentImage.value() != texture){
                        info.currentImage = texture;
                        set->buildIndex(i);
                    }
                }
            }
        }
    }

    auto allSets = std::vector<vk::DescriptorSet>(); 
    for (int i = 0; i < this->shaderInfoSets[frameInFlight].size(); i++) {
        allSets.push_back(this->shaderInfoSets[frameInFlight].at(i)->getDescriptorSet()); 
    }

    return allSets; 
}

star::StarShaderInfo::Builder &star::StarShaderInfo::Builder::add(const Handle &textureHandle, const vk::ImageLayout &desiredLayout, vk::Format &requestedImageViewFormat, const bool &willCheckForIfReady)
{
    this->activeSet->back()->add(ShaderInfo(ShaderInfo::TextureInfo{textureHandle, desiredLayout, requestedImageViewFormat}, willCheckForIfReady)); 
    return *this;
}

star::StarShaderInfo::Builder &star::StarShaderInfo::Builder::startSet()
{
    auto size = this->activeSet->size();
    assert(size < this->layouts.size() && "Pushed beyond size");

    this->activeSet->push_back(std::make_shared<ShaderInfoSet>(this->device, *this->layouts[size]));
    return *this;
}

star::StarShaderInfo::Builder& star::StarShaderInfo::Builder::add(const Handle &textureHandle, const vk::ImageLayout &desiredLayout, const bool& willCheckForIfReady)
{
    this->activeSet->back()->add(ShaderInfo(ShaderInfo::TextureInfo{textureHandle, desiredLayout}, willCheckForIfReady)); 
    return *this;
}
