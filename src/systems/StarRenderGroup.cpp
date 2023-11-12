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
	this->groups.front().baseObject.object.getMeshes().front()->getMaterial().getDescriptorSetLayout(layoutBuilder);
	this->largestObjectDescriptorSetLayout = layoutBuilder.build(); 

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
	auto layoutBuilder = StarDescriptorSetLayout::Builder(device);
	newObject.getMeshes().at(0)->getMaterial().getDescriptorSetLayout(layoutBuilder);
	auto newLayout = layoutBuilder.build(); 

	if (newLayout->getBindings().size() > this->largestObjectDescriptorSetLayout->getBindings().size()) {
		this->largestObjectDescriptorSetLayout = std::move(newLayout); 
	}


	this->numObjects++;
	this->numMeshes += newObject.getMeshes().size();
}

void StarRenderGroup::updateBuffers(uint32_t currentImage) {
	UniformBufferObject newBufferObject{};

	auto minProp = this->device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;
	auto minAlignmentOfUBOElements = StarBuffer::getAlignment(sizeof(UniformBufferObject), minProp);
	size_t objectCountIt = 0; 

	for (auto& group : this->groups) {
		//need to update the base object too
		newBufferObject.modelMatrix = group.baseObject.object.getDisplayMatrix(); 
		newBufferObject.normalMatrix = group.baseObject.object.getNormalMatrix(); 
		this->uniformBuffers[currentImage]->writeToBuffer(&newBufferObject, sizeof(UniformBufferObject), minAlignmentOfUBOElements* objectCountIt); 
		objectCountIt++; 

		for (auto& obj : group.objects) {
			newBufferObject.modelMatrix = obj.object.getDisplayMatrix();
			newBufferObject.normalMatrix = obj.object.getNormalMatrix();

			this->uniformBuffers[currentImage]->writeToBuffer(&newBufferObject, sizeof(UniformBufferObject), minAlignmentOfUBOElements * objectCountIt);
			objectCountIt++; 
		}
	}
}

void StarRenderGroup::recordCommands(StarCommandBuffer& mainDrawBuffer, int swapChainImageIndex) {
	int vertexCount = 0;

	for (auto& group : this->groups) {
		group.baseObject.object.recordCommands(mainDrawBuffer, pipelineLayout, swapChainImageIndex, group.baseObject.startVBIndex, group.baseObject.startIBIndex);
		for (auto& obj : group.objects) {
			//record commands for each object
			obj.object.recordCommands(mainDrawBuffer, this->pipelineLayout, swapChainImageIndex, obj.startVBIndex, obj.startIBIndex);
		}
	}
}

void StarRenderGroup::init(StarDescriptorSetLayout& engineSetLayout, vk::RenderPass engineRenderPass, 
	std::vector<vk::DescriptorSet> enginePerImageDescriptors)
{
	createRenderBuffers(); 
	createDescriptorSetLayout();
	createDescriptorPool();
	createPipelineLayout(engineSetLayout);
	prepareObjects(engineRenderPass, enginePerImageDescriptors);
}

bool StarRenderGroup::isObjectCompatible(StarObject& object)
{
	bool descriptorSetsCompatible = false; 
	//check if descriptor layouts are compatible
	auto layoutBuilder = StarDescriptorSetLayout::Builder(device); 
	object.getMeshes().front()->getMaterial().getDescriptorSetLayout(layoutBuilder);

	auto layout = layoutBuilder.build(); 

	if (largestObjectDescriptorSetLayout->isCompatibleWith(*layout)) {
		descriptorSetsCompatible = true; 
	}

	return descriptorSetsCompatible;
}

void StarRenderGroup::createRenderBuffers() {
	this->uniformBuffers.resize(this->numSwapChainImages);

	auto minUniformSize = this->device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;

	for (size_t i = 0; i < numSwapChainImages; i++) {
		this->uniformBuffers[i] = std::make_unique<StarBuffer>(this->device, numObjects, sizeof(UniformBufferObject),
			vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minUniformSize);
		this->uniformBuffers[i]->map();
	}
}

void StarRenderGroup::createDescriptorPool() {
	//create descriptor pools 

	//assume largest descriptor layout has the number we need
	auto newPoolBuilder = StarDescriptorPool::Builder(this->device); 
	
	const std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding>& types = this->largestObjectDescriptorSetLayout->getBindings();

	//add objects needs
	for (auto& it : types) {
		newPoolBuilder.addPoolSize(it.second.descriptorType, it.second.descriptorCount * this->numSwapChainImages * this->numMeshes);
	}

	const std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding>& globalTypes = this->globalSetLayout->getBindings();
	for (auto& it : globalTypes) {
		newPoolBuilder.addPoolSize(it.second.descriptorType, it.second.descriptorCount * this->numSwapChainImages * this->numMeshes);
	}

	this->descriptorPool = newPoolBuilder.build(); 
}

void StarRenderGroup::createDescriptorSetLayout()
{
	this->globalSetLayout = StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.build();
}

void StarRenderGroup::prepareObjects(vk::RenderPass engineRenderPass, std::vector<vk::DescriptorSet> enginePerImageDescriptors) {
	//get descriptor sets from objects and place into render structs
	int objCounter = 0;

	for (auto& group : this->groups) {
		//prepare base object
		auto globalObjDesc = generateObjectExternalDescriptors(objCounter, enginePerImageDescriptors);
		group.baseObject.object.prepRender(device, swapChainExtent, pipelineLayout, engineRenderPass, numSwapChainImages, *largestObjectDescriptorSetLayout, *descriptorPool, globalObjDesc);
		objCounter++; 

		for (auto& renderObject : group.objects) {
			globalObjDesc = generateObjectExternalDescriptors(objCounter, enginePerImageDescriptors);
			renderObject.object.prepRender(device, numSwapChainImages, *largestObjectDescriptorSetLayout, *descriptorPool, globalObjDesc, group.baseObject.object.getPipline()); 

			objCounter++;
		}
	}
}

std::vector<std::vector<vk::DescriptorSet>> star::StarRenderGroup::generateObjectExternalDescriptors(int objectOffset,
	std::vector<vk::DescriptorSet> enginePerImageDescriptors)
{
	vk::DescriptorBufferInfo bufferInfo{};
	auto set = std::vector<std::vector<vk::DescriptorSet>>();

	auto minProp = this->device.getPhysicalDevice().getProperties().limits.minUniformBufferOffsetAlignment;
	auto minAlignmentOfUBOElements = StarBuffer::getAlignment(sizeof(UniformBufferObject), minProp);

	for (int i = 0; i < this->numSwapChainImages; i++) {
		auto descriptors = std::vector<vk::DescriptorSet>();

		//add descriptors from engine
		descriptors.push_back(enginePerImageDescriptors.at(i));

		//where in the buffer is the ubo info for this object
		bufferInfo = vk::DescriptorBufferInfo{
			this->uniformBuffers[i]->getBuffer(),
			minAlignmentOfUBOElements * objectOffset,
			sizeof(UniformBufferObject)
		};

		auto writer = StarDescriptorWriter(this->device, *this->globalSetLayout, *this->descriptorPool)
			.writeBuffer(0, &bufferInfo);

		vk::DescriptorSet globalSet;
		writer.build(globalSet);
		descriptors.push_back(globalSet);

		set.push_back(descriptors); 
	}

	return set;
}

void StarRenderGroup::createPipelineLayout(StarDescriptorSetLayout& engineSetLayout) {
	auto descriptorLayouts = std::vector<vk::DescriptorSetLayout>{
		engineSetLayout.getDescriptorSetLayout(),
		this->globalSetLayout->getDescriptorSetLayout(),
		this->largestObjectDescriptorSetLayout->getDescriptorSetLayout()
	}; 

	/* Pipeline Layout */
	//uniform values in shaders need to be defined here 
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	this->pipelineLayout = this->device.getDevice().createPipelineLayout(pipelineLayoutInfo);
	if (!this->pipelineLayout) {
		throw std::runtime_error("failed to create pipeline layout");
	}
}
}
