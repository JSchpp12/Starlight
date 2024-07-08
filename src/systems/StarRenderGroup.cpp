#include "StarRenderGroup.hpp"

namespace star {
StarRenderGroup::StarRenderGroup(StarDevice& device, size_t numSwapChainImages, 
	vk::Extent2D swapChainExtent, StarObject& baseObject, 
	uint32_t baseObj_VBStart, uint32_t baseObj_IBStart)
	: device(device),numSwapChainImages(numSwapChainImages), swapChainExtent(swapChainExtent)
{
	auto objInfo = RenderObjectInfo(baseObject, baseObj_VBStart, baseObj_IBStart);
	this->groups.push_back(Group(objInfo));

	auto layoutBuilder = StarDescriptorSetLayout::Builder(device); 
	this->largestDescriptorSet = this->groups.front().baseObject.object.getDescriptorSetLayouts(device);

	this->numObjects++; 
	this->numMeshes = baseObject.getMeshes().size(); 
}

StarRenderGroup::~StarRenderGroup() {
	//cleanup objects
	for (auto& group : this->groups) {
		for (auto& obj : group.objects) {
			obj.object.cleanupRender(device); 
		}
		//cleanup base object last since it owns the pipeline
		group.baseObject.object.cleanupRender(device); 
	}

	this->device.getDevice().destroyPipelineLayout(this->pipelineLayout);
}

void StarRenderGroup::addObject(StarObject& newObject, uint32_t indexStartOffset, uint32_t vertexStartOffset) {
	//check if any other object can share the same Pipeline 
	Group* targetGroup = nullptr; 

	//for now only check if they share the same shader
	for (auto& group : this->groups) {
		//check the first object in each group just to see if they have the same shaders

		auto objectShaders = group.baseObject.object.getShaders();
		auto newObjectShaders = newObject.getShaders();

		bool isMatch = true; 
		for (auto& it : objectShaders) {
			auto doesExistInOther = newObjectShaders.find(it.first); 

			//check if new object even has shader at that stage
			if (doesExistInOther == newObjectShaders.end()) {
				isMatch = false; 
				break; 
			}

			//both objects have a shader at the same stage, see if they are the same
			if (it.second.getPath() != newObjectShaders.at(it.first).getPath()) {
				isMatch = false;
				break; 
			}
		}
		if (isMatch)
			targetGroup = &group; 
	}

	if (targetGroup == nullptr) {
		//requires new pipeline -- and group
		auto objInfo = RenderObjectInfo(newObject, vertexStartOffset, indexStartOffset);
		this->groups.push_back(Group(objInfo));
	}
	else {
		auto objInfo = RenderObjectInfo(newObject, vertexStartOffset, indexStartOffset);
		targetGroup->objects.push_back(objInfo);
	}

	//check if this new object has a larger descriptor set layout than the current one
	auto newLayouts = newObject.getDescriptorSetLayouts(device); 

	std::vector<std::unique_ptr<StarDescriptorSetLayout>> combinedSet = std::vector<std::unique_ptr<StarDescriptorSetLayout>>(); 
	std::vector<std::unique_ptr<StarDescriptorSetLayout>>* largerSet = newLayouts.size() > this->largestDescriptorSet.size() ? &newLayouts : &this->largestDescriptorSet;
	std::vector<std::unique_ptr<StarDescriptorSetLayout>>* smallerSet = newLayouts.size() > this->largestDescriptorSet.size() ? &this->largestDescriptorSet : &newLayouts; 

	//already assuming these are already compatible
	for (int i = 0; i < largerSet->size(); i++) {
		if (i < smallerSet->size()) {
			if (largerSet->at(i)->getBindings().size() > smallerSet->at(i)->getBindings().size()) {
				//this->largestDescriptorSet.at(i) = std::move(newLayouts.at(i));
				//the new set is larger than the current one, replace
				combinedSet.push_back(std::move(largerSet->at(i))); 
			}
			else {
				combinedSet.push_back(std::move(smallerSet->at(i))); 
			}
		}
		else {
			combinedSet.push_back(std::move(largerSet->at(i))); 
		}
	}
	this->largestDescriptorSet = std::move(combinedSet); 

	this->numObjects++;
	this->numMeshes += newObject.getMeshes().size();
}

void StarRenderGroup::recordRenderPassCommands(StarCommandBuffer& mainDrawBuffer, int swapChainImageIndex) {
	for (auto& group : this->groups) {
		group.baseObject.object.recordRenderPassCommands(mainDrawBuffer, pipelineLayout, swapChainImageIndex, group.baseObject.startVBIndex, group.baseObject.startIBIndex);
		for (auto& obj : group.objects) {
			//record commands for each object
			obj.object.recordRenderPassCommands(mainDrawBuffer, this->pipelineLayout, swapChainImageIndex, obj.startVBIndex, obj.startIBIndex);
		}
	}
}

void StarRenderGroup::recordPreRenderPassCommands(StarCommandBuffer& mainDrawBuffer, int swapChainImageIndex)
{
	for (auto& group : this->groups) {
		group.baseObject.object.recordPreRenderPassCommands(mainDrawBuffer, swapChainImageIndex); 
		for (auto& obj : group.objects) {
			obj.object.recordPreRenderPassCommands(mainDrawBuffer, swapChainImageIndex);
		}
	}
}

void StarRenderGroup::init(StarDescriptorSetLayout& engineSetLayout, vk::RenderPass engineRenderPass, 
	std::vector<vk::DescriptorSet> enginePerImageDescriptors)
{
	createPipelineLayout(engineSetLayout);
	prepareObjects(engineSetLayout, engineRenderPass, enginePerImageDescriptors);
}

bool StarRenderGroup::isObjectCompatible(StarObject& object)
{
	bool descriptorSetsCompatible = false; 
	//check if descriptor layouts are compatible
	auto layoutBuilder = StarDescriptorSetLayout::Builder(device); 
	auto compLayouts = object.getDescriptorSetLayouts(device);

	std::vector<std::unique_ptr<StarDescriptorSetLayout>>* largerSet = &this->largestDescriptorSet; 
	std::vector<std::unique_ptr<StarDescriptorSetLayout>>* smallerSet = &compLayouts; 

	if (this->largestDescriptorSet.size() < compLayouts.size()) {
		largerSet = &compLayouts;
		smallerSet = &this->largestDescriptorSet;
	}
		
	for (int i = 0; i < smallerSet->size(); i++) {
		if (!largerSet->at(i)->isCompatibleWith(*smallerSet->at(i)))
			return false; 
	}

	return true;
}

void StarRenderGroup::prepareObjects(StarDescriptorSetLayout& engineLayout, vk::RenderPass engineRenderPass, std::vector<vk::DescriptorSet> enginePerImageDescriptors) {
	//get descriptor sets from objects and place into render structs
	int objCounter = 0;

	std::vector<std::reference_wrapper<StarDescriptorSetLayout>> finalizedGroupLayouts{
		engineLayout
	}; 
	for (auto& layout : this->largestDescriptorSet) {
		finalizedGroupLayouts.push_back(*layout); 
	}

	for (auto& group : this->groups) {
		//prepare base object
		auto globalObjDesc = generateObjectExternalDescriptors(objCounter, enginePerImageDescriptors);
		group.baseObject.object.prepRender(device, swapChainExtent, pipelineLayout, engineRenderPass, numSwapChainImages, finalizedGroupLayouts, globalObjDesc);
		objCounter++; 

		for (auto& renderObject : group.objects) {
			globalObjDesc = generateObjectExternalDescriptors(objCounter, enginePerImageDescriptors);
			renderObject.object.prepRender(device, numSwapChainImages, finalizedGroupLayouts, globalObjDesc, group.baseObject.object.getPipline());

			objCounter++;
		}
	}
}

std::vector<std::vector<vk::DescriptorSet>> star::StarRenderGroup::generateObjectExternalDescriptors(int objectOffset,
	std::vector<vk::DescriptorSet> enginePerImageDescriptors)
{
	vk::DescriptorBufferInfo bufferInfo{};
	auto set = std::vector<std::vector<vk::DescriptorSet>>();

	for (int i = 0; i < this->numSwapChainImages; i++) {
		auto descriptors = std::vector<vk::DescriptorSet>();

		//add descriptors from engine
		descriptors.push_back(enginePerImageDescriptors.at(i));

		set.push_back(descriptors);
	}

	return set;
}

void StarRenderGroup::createPipelineLayout(StarDescriptorSetLayout& engineSetLayout) {
	std::vector<vk::DescriptorSetLayout> layouts{
		engineSetLayout.getDescriptorSetLayout()
	};

	for (auto& set : this->largestDescriptorSet) {
		layouts.push_back(set->getDescriptorSetLayout()); 
	}

	/* Pipeline Layout */
	//uniform values in shaders need to be defined here 
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	this->pipelineLayout = this->device.getDevice().createPipelineLayout(pipelineLayoutInfo);
	
	if (!this->pipelineLayout) {
		throw std::runtime_error("failed to create pipeline layout");
	}
}
}
