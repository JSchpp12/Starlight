#pragma once

#include "devices/StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <memory>
#include <unordered_map>

namespace star
{
class StarDescriptorSetLayout
{
  public:
    class Builder
    {
      public:
        Builder(core::devices::StarDevice &device) : m_device(device)
        {
        }

        Builder &addBinding(uint32_t binding, vk::DescriptorType descriptorType, vk::ShaderStageFlags stageFlags,
                            uint32_t count = 1);
        std::unique_ptr<StarDescriptorSetLayout> build() const;

      protected:
      private:
        core::devices::StarDevice &m_device;
        std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings{};
    };

    StarDescriptorSetLayout(core::devices::StarDevice &device, std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings);

    ~StarDescriptorSetLayout();

    bool isCompatibleWith(const StarDescriptorSetLayout &compare);

    vk::DescriptorSetLayout getDescriptorSetLayout();

    const std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> &getBindings()
    {
        return this->bindings;
    }

  protected:
    void build();

  private:
    core::devices::StarDevice &m_device;
    vk::DescriptorSetLayout descriptorSetLayout;
    std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings;

    // allow access to the descriptor writer
    friend class StarDescriptorWriter;
};

class StarDescriptorPool
{
  public:
    class Builder
    {
      public:
        Builder(core::devices::StarDevice &device) : m_device(device) {};

        Builder &addPoolSize(vk::DescriptorType descriptorType, uint32_t count);
        Builder &setPoolFlags(vk::DescriptorPoolCreateFlags flags);
        Builder &setMaxSets(uint32_t count);
        std::unique_ptr<StarDescriptorPool> build() const;

      protected:
      private:
        core::devices::StarDevice &m_device;
        std::vector<vk::DescriptorPoolSize> poolSizes;
        uint32_t maxSets = 50;
        vk::DescriptorPoolCreateFlags poolFlags{};
    };

    StarDescriptorPool(core::devices::StarDevice &device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags,
                       const std::vector<vk::DescriptorPoolSize> &poolSizes);
    ~StarDescriptorPool();

    vk::DescriptorPool getDescriptorPool();

    bool allocateDescriptorSet(const vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet &descriptorSets);

    void freeDescriptors(std::vector<vk::DescriptorSet> &descriptors) const;

    void resetPool();

  protected:
  private:
    core::devices::StarDevice &m_device;
    vk::DescriptorPool descriptorPool;

    // allow this class to read the private info of StarDescriptorSetLayout for construction
    friend class StarDescriptorWriter;
};

class StarDescriptorWriter
{
  public:
    StarDescriptorWriter(core::devices::StarDevice &device, StarDescriptorSetLayout &setLayout, StarDescriptorPool &pool);

    StarDescriptorWriter &writeBuffer(uint32_t binding, vk::DescriptorBufferInfo &bufferInfos);

    StarDescriptorWriter &writeImage(uint32_t binding, vk::DescriptorImageInfo &imageInfo);

    vk::DescriptorSet build();

    void overwrite(vk::DescriptorSet &set);

  private:
    struct FullDescriptorInfo
    {
        std::optional<vk::DescriptorBufferInfo> bufferInfo = std::optional<vk::DescriptorBufferInfo>();
        std::optional<vk::DescriptorImageInfo> imageInfo = std::optional<vk::DescriptorImageInfo>();
        vk::DescriptorType type;
        uint32_t binding;

        FullDescriptorInfo(vk::DescriptorBufferInfo &bufferInfo, vk::DescriptorType type, uint32_t binding)
            : bufferInfo(bufferInfo), type(type), binding(binding) {};

        FullDescriptorInfo(vk::DescriptorImageInfo &imageInfo, vk::DescriptorType type, uint32_t binding)
            : imageInfo(imageInfo), type(type), binding(binding) {};

        vk::WriteDescriptorSet getSetInfo()
        {
            auto fullSet = vk::WriteDescriptorSet{};
            fullSet.sType = vk::StructureType::eWriteDescriptorSet;
            fullSet.descriptorType = this->type;
            fullSet.dstBinding = this->binding;
            if (this->bufferInfo.has_value())
                fullSet.pBufferInfo = &this->bufferInfo.value();
            else if (this->imageInfo.has_value())
                fullSet.pImageInfo = &this->imageInfo.value();
            fullSet.descriptorCount = 1;

            return fullSet;
        }
    };

    core::devices::StarDevice &m_device;
    StarDescriptorSetLayout &setLayout;
    StarDescriptorPool &pool;
    std::vector<FullDescriptorInfo> writeSets;
};
} // namespace star