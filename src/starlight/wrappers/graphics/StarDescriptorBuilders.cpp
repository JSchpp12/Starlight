#include "StarDescriptorBuilders.hpp"

#include "starlight/core/Exceptions.hpp"

namespace star
{
StarDescriptorSetLayout::Builder &StarDescriptorSetLayout::Builder::addBinding(uint32_t binding,
                                                                               vk::DescriptorType descriptorType,
                                                                               vk::ShaderStageFlags stageFlags,
                                                                               uint32_t count)
{
    assert(bindings.count(binding) == 0 && "Binding already in use");
    vk::DescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;

    this->bindings[binding] = layoutBinding;
    return *this;
}

bool StarDescriptorSetLayout::isCompatibleWith(const StarDescriptorSetLayout &compare) const
{
    if (compare.bindings.size() != this->bindings.size())
        return false;

    for (const auto &binding : this->bindings)
    {
        // check if the other layout has a binding of the same type
        if (compare.bindings.find(binding.first) == compare.bindings.end() || binding.second.descriptorType != compare.bindings.at(binding.first).descriptorType)
        {
            return false;
        }
    }
    return true;
}

void StarDescriptorSetLayout::prepRender(core::device::StarDevice &device)
{
    std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;

    for (auto &binding : bindings)
    {
        setLayoutBindings.push_back(binding.second);
    }

    vk::DescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    createInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    createInfo.pBindings = setLayoutBindings.data();

    this->descriptorSetLayout = device.getVulkanDevice().createDescriptorSetLayout(createInfo);
    if (!this->descriptorSetLayout)
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void StarDescriptorSetLayout::cleanupRender(core::device::StarDevice &device){
    device.getVulkanDevice().destroyDescriptorSetLayout(this->descriptorSetLayout);
    this->descriptorSetLayout = VK_NULL_HANDLE;
}

/* Descriptor Pool */
StarDescriptorPool::Builder &StarDescriptorPool::Builder::addPoolSize(vk::DescriptorType descriptorType, uint32_t count)
{
    // check if pool already contains descriptorType
    for (int i = 0; i < this->poolSizes.size(); i++)
    {
        if (this->poolSizes.at(i).type == descriptorType)
        {
            this->poolSizes.at(i).descriptorCount = this->poolSizes.at(i).descriptorCount + count;
            return *this;
        }
    }

    // new type add to pool
    this->poolSizes.push_back({descriptorType, count});
    return *this;
}

StarDescriptorPool::Builder &StarDescriptorPool::Builder::setPoolFlags(vk::DescriptorPoolCreateFlags flags)
{
    this->poolFlags = flags;
    return *this;
}

StarDescriptorPool::Builder &StarDescriptorPool::Builder::setMaxSets(uint32_t count)
{
    this->maxSets = count;
    return *this;
}

std::unique_ptr<StarDescriptorPool> StarDescriptorPool::Builder::build() const
{
    return std::make_unique<StarDescriptorPool>(m_device, this->maxSets, this->poolFlags, this->poolSizes);
}

StarDescriptorPool::StarDescriptorPool(core::device::StarDevice &device, uint32_t maxSets,
                                       vk::DescriptorPoolCreateFlags poolFlags,
                                       const std::vector<vk::DescriptorPoolSize> &poolSizes)
    : m_device(device)
{
    vk::DescriptorPoolCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = maxSets;
    createInfo.flags = poolFlags;

    this->descriptorPool = m_device.getVulkanDevice().createDescriptorPool(createInfo);
    if (!this->descriptorPool)
    {
        throw std::runtime_error("Unable to create descriptor pool");
    }
}

StarDescriptorPool::~StarDescriptorPool()
{
    m_device.getVulkanDevice().destroyDescriptorPool(this->descriptorPool);
}

vk::DescriptorPool StarDescriptorPool::getDescriptorPool()
{
    return this->descriptorPool;
}

bool StarDescriptorPool::allocateDescriptorSet(const vk::DescriptorSetLayout descriptorSetLayout,
                                               vk::DescriptorSet &descriptorSet)
{
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    allocInfo.descriptorPool = this->descriptorPool;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;

    auto result = m_device.getVulkanDevice().allocateDescriptorSets(&allocInfo, &descriptorSet);

    if (result != vk::Result::eSuccess)
    {
        if (result == vk::Result::eErrorOutOfPoolMemory)
        {
            std::cout << "Out of pool memory" << std::endl;
        }
        else
        {
            std::cout << "Failed to allocate descriptor set " << result << std::endl;
        }

        return false;
    }

    return true;
}

void StarDescriptorPool::freeDescriptors(std::vector<vk::DescriptorSet> &descriptors) const
{
    m_device.getVulkanDevice().freeDescriptorSets(this->descriptorPool, descriptors);
}

void StarDescriptorPool::resetPool()
{
    m_device.getVulkanDevice().resetDescriptorPool(this->descriptorPool);
}

/* Descriptor Writer */

StarDescriptorWriter::StarDescriptorWriter(core::device::StarDevice &device, StarDescriptorSetLayout &setLayout,
                                           StarDescriptorPool &pool)
    : m_device(device), setLayout{setLayout}, pool{pool}
{
}

StarDescriptorWriter &StarDescriptorWriter::writeBuffer(uint32_t binding, vk::DescriptorBufferInfo &bufferInfos)
{
    assert(this->setLayout.bindings.count(binding) == 1 && "Layout does not contain binding specified");

    auto &bindingDescription = setLayout.bindings[binding];
    assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

    for (auto &set : this->writeSets)
    {
        if (set.binding == binding)
        {
            set = FullDescriptorInfo(bufferInfos, bindingDescription.descriptorType, binding);
            return *this;
        }
    }

    this->writeSets.push_back(FullDescriptorInfo(bufferInfos, bindingDescription.descriptorType, binding));
    return *this;
}

StarDescriptorWriter &StarDescriptorWriter::writeImage(uint32_t binding, vk::DescriptorImageInfo &imageInfo)
{
    assert(this->setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];
    assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

    this->writeSets.push_back(FullDescriptorInfo(imageInfo, bindingDescription.descriptorType, binding));

    return *this;
}

vk::DescriptorSet StarDescriptorWriter::build()
{
    vk::DescriptorSet set;

    bool success = this->pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
    if (!success)
    {
        STAR_THROW("Failed to allocate descriptor set");
    }
    overwrite(set);
    return set;
}

void StarDescriptorWriter::overwrite(vk::DescriptorSet &set)
{

    std::vector<vk::WriteDescriptorSet> nSet;

    for (auto &write : this->writeSets)
    {
        auto newSet = write.getSetInfo();
        newSet.dstSet = set;
        nSet.push_back(newSet);
    }

    m_device.getVulkanDevice().updateDescriptorSets(nSet, nullptr);
}
} // namespace star