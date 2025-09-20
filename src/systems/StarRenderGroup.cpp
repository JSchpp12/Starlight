#include "StarRenderGroup.hpp"

namespace star
{
StarRenderGroup::StarRenderGroup(core::device::DeviceContext &device, std::shared_ptr<StarObject> baseObject)
    : device(device)
{
    auto objInfo = RenderObjectInfo(baseObject);
    this->groups.push_back(Group(objInfo));

    auto layoutBuilder = StarDescriptorSetLayout::Builder();
    this->largestDescriptorSet = this->groups.front().baseObject.object->getDescriptorSetLayouts(device);

    this->numObjects++;
}

StarRenderGroup::~StarRenderGroup()
{
    // cleanup objects
    for (auto &group : this->groups)
    {
        for (auto &obj : group.objects)
        {
            obj.object->cleanupRender(device);
        }
        // cleanup base object last since it owns the pipeline
        group.baseObject.object->cleanupRender(device);
    }

    this->device.getDevice().getVulkanDevice().destroyPipelineLayout(m_pipelineLayout);
}

void StarRenderGroup::frameUpdate(core::device::DeviceContext &context)
{

    for (auto &group : groups)
    {
        group.baseObject.object->frameUpdate(context);
        for (auto &obj : group.objects)
        {
            obj.object->frameUpdate(context);
        }
    }
}

void StarRenderGroup::prepRender(core::device::DeviceContext &context, const vk::Extent2D &renderingResolution,
                                 const uint8_t &numFramesInFlight, StarShaderInfo::Builder initEngineBuilder,
                                 core::renderer::RenderingTargetInfo renderingInfo)
{
    auto fullSetLayout = initEngineBuilder.getCurrentSetLayouts();
    for (auto &set : this->largestDescriptorSet)
    {
        fullSetLayout.emplace_back(set);

        initEngineBuilder.addSetLayout(set);
    }

    m_pipelineLayout = createPipelineLayout(context, fullSetLayout);
    prepareObjects(initEngineBuilder, renderingInfo, context.getRenderingSurface().getResolution(), numFramesInFlight);
}

void StarRenderGroup::addObject(std::shared_ptr<StarObject> newObject)
{
    // check if any other object can share the same Pipeline
    Group *targetGroup = nullptr;

    // for now only check if they share the same shader
    for (auto &group : this->groups)
    {
        // check the first object in each group just to see if they have the same shaders

        auto objectShaders = group.baseObject.object->getShaders();
        auto newObjectShaders = newObject->getShaders();

        bool isMatch = true;
        for (auto &it : objectShaders)
        {
            auto doesExistInOther = newObjectShaders.find(it.first);

            // check if new object even has shader at that stage
            if (doesExistInOther == newObjectShaders.end())
            {
                isMatch = false;
                break;
            }

            // both objects have a shader at the same stage, see if they are the same
            if (it.second.getPath() != newObjectShaders.at(it.first).getPath())
            {
                isMatch = false;
                break;
            }
        }
        if (isMatch)
            targetGroup = &group;
    }

    if (targetGroup == nullptr)
    {
        // requires new pipeline -- and group
        auto objInfo = RenderObjectInfo{newObject};
        this->groups.push_back(Group(objInfo));
    }
    else
    {
        auto objInfo = RenderObjectInfo(newObject);
        targetGroup->objects.push_back(objInfo);
    }

    // check if this new object has a larger descriptor set layout than the current one
    auto newLayouts = newObject->getDescriptorSetLayouts(device);

    std::vector<std::shared_ptr<StarDescriptorSetLayout>> combinedSet =
        std::vector<std::shared_ptr<StarDescriptorSetLayout>>();
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> *largerSet =
        newLayouts.size() > this->largestDescriptorSet.size() ? &newLayouts : &this->largestDescriptorSet;
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> *smallerSet =
        newLayouts.size() > this->largestDescriptorSet.size() ? &this->largestDescriptorSet : &newLayouts;

    // already assuming these are already compatible
    for (size_t i = 0; i < largerSet->size(); i++)
    {
        if (i < smallerSet->size())
        {
            if (largerSet->at(i)->getBindings().size() > smallerSet->at(i)->getBindings().size())
            {
                // the new set is larger than the current one, replace
                combinedSet.push_back(std::move(largerSet->at(i)));
            }
            else
            {
                combinedSet.push_back(std::move(smallerSet->at(i)));
            }
        }
        else
        {
            combinedSet.push_back(std::move(largerSet->at(i)));
        }
    }
    this->largestDescriptorSet = combinedSet;

    this->numObjects++;
}

void StarRenderGroup::recordRenderPassCommands(vk::CommandBuffer &mainDrawBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->groups)
    {
        group.baseObject.object->recordRenderPassCommands(mainDrawBuffer, m_pipelineLayout, frameInFlightIndex);
        for (auto &obj : group.objects)
        {
            // record commands for each object
            obj.object->recordRenderPassCommands(mainDrawBuffer, m_pipelineLayout, frameInFlightIndex);
        }
    }
}

void StarRenderGroup::recordPreRenderPassCommands(vk::CommandBuffer &mainDrawBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->groups)
    {
        group.baseObject.object->recordPreRenderPassCommands(mainDrawBuffer, frameInFlightIndex);
        for (auto &obj : group.objects)
        {
            obj.object->recordPreRenderPassCommands(mainDrawBuffer, frameInFlightIndex);
        }
    }
}

void StarRenderGroup::recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    for (auto &group : this->groups)
    {
        group.baseObject.object->recordPostRenderPassCommands(commandBuffer, frameInFlightIndex);
        for (auto &obj : group.objects)
        {
            obj.object->recordPostRenderPassCommands(commandBuffer, frameInFlightIndex);
        }
    }
}

bool StarRenderGroup::isObjectCompatible(StarObject &object)
{
    // check if descriptor layouts are compatible
    auto compLayouts = object.getDescriptorSetLayouts(device);

    std::vector<std::shared_ptr<StarDescriptorSetLayout>> *largerSet = &this->largestDescriptorSet;
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> *smallerSet = &compLayouts;

    if (this->largestDescriptorSet.size() < compLayouts.size())
    {
        largerSet = &compLayouts;
        smallerSet = &this->largestDescriptorSet;
    }

    for (size_t i = 0; i < smallerSet->size(); i++)
    {
        if (!largerSet->at(i)->isCompatibleWith(*smallerSet->at(i)))
            return false;
    }

    return true;
}

void StarRenderGroup::prepareObjects(StarShaderInfo::Builder &groupBuilder,
                                     core::renderer::RenderingTargetInfo renderingInfo,
                                     const vk::Extent2D &swapChainExtent, const uint8_t &numFramesInFlight)
{
    // get descriptor sets from objects and place into render structs

    for (auto &group : this->groups)
    {
        // prepare base object
        group.baseObject.object->prepRender(device, swapChainExtent, numFramesInFlight, groupBuilder, m_pipelineLayout,
                                            renderingInfo);

        for (auto &renderObject : group.objects)
        {
            renderObject.object->prepRender(device, swapChainExtent, numFramesInFlight, groupBuilder,
                                            group.baseObject.object->getPipline());
        }
    }
}

vk::PipelineLayout StarRenderGroup::createPipelineLayout(
    core::device::DeviceContext &context, std::vector<std::shared_ptr<StarDescriptorSetLayout>> &fullSetLayout)
{

    auto sets = std::vector<vk::DescriptorSetLayout>();
    for (auto &set : fullSetLayout)
    {
        set->prepRender(context.getDevice());
        sets.emplace_back(set->getDescriptorSetLayout());
    }

    /* Pipeline Layout */
    // uniform values in shaders need to be defined here
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(sets.size());
    pipelineLayoutInfo.pSetLayouts = sets.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    auto result = this->device.getDevice().getVulkanDevice().createPipelineLayout(pipelineLayoutInfo);

    if (!result)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    return result;
}
} // namespace star
