#pragma once

#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <memory>
#include <unordered_map>

namespace star {
class StarDescriptorSetLayout {
public:
	class Builder {
	public:
		Builder(StarDevice& device) : starDevice(device) {}

		Builder& addBinding(
			uint32_t binding,
			vk::DescriptorType descriptorType,
			vk::ShaderStageFlags stageFlags,
			uint32_t count = 1);
		std::unique_ptr<StarDescriptorSetLayout> build() const;

	protected:

	private:
		StarDevice& starDevice;
		std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings{};

	};

	StarDescriptorSetLayout(StarDevice& device, std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings);

	~StarDescriptorSetLayout();

	bool isCompatibleWith(const StarDescriptorSetLayout& compare); 

	vk::DescriptorSetLayout getDescriptorSetLayout();

	const std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding>& getBindings() {
		return this->bindings;
	}

protected:
	virtual void build();

private:
	StarDevice& starDevice;
	vk::DescriptorSetLayout descriptorSetLayout;
	std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings;

	//allow access to the descriptor writer 
	friend class StarDescriptorWriter;
};

class StarDescriptorPool {
public:
	class Builder {
	public:
		Builder(StarDevice& device) : starDevice(device) {};

		Builder& addPoolSize(vk::DescriptorType descriptorType, uint32_t count);
		Builder& setPoolFlags(vk::DescriptorPoolCreateFlags flags);
		Builder& setMaxSets(uint32_t count);
		std::unique_ptr<StarDescriptorPool> build() const;

	protected:

	private:
		StarDevice& starDevice;
		std::vector<vk::DescriptorPoolSize> poolSizes;
		uint32_t maxSets = 50;
		vk::DescriptorPoolCreateFlags poolFlags{};
	};

	StarDescriptorPool(StarDevice& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags poolFlags, const std::vector<vk::DescriptorPoolSize>& poolSizes);
	~StarDescriptorPool();

	vk::DescriptorPool getDescriptorPool();

	bool allocateDescriptorSet(const vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorSet& descriptorSets);

	void freeDescriptors(std::vector<vk::DescriptorSet>& descriptors) const;

	void resetPool();

protected:

private:
	StarDevice& starDevice;
	vk::DescriptorPool descriptorPool;

	//allow this class to read the private info of StarDescriptorSetLayout for construction 
	friend class StarDescriptorWriter;
};
 
class StarDescriptorWriter {
public:
	StarDescriptorWriter(StarDevice& device, StarDescriptorSetLayout& setLayout, StarDescriptorPool& pool);

	StarDescriptorWriter& writeBuffer(uint32_t binding, vk::DescriptorBufferInfo& bufferInfos);

	StarDescriptorWriter& writeImage(uint32_t binding, vk::DescriptorImageInfo& imageInfo);

	vk::DescriptorSet build();

	void overwrite(vk::DescriptorSet& set);

private:
	struct FullDescriptorInfo {
		std::optional<vk::DescriptorBufferInfo> bufferInfo = std::optional<vk::DescriptorBufferInfo>(); 
		std::optional<vk::DescriptorImageInfo> imageInfo = std::optional<vk::DescriptorImageInfo>();
		vk::DescriptorType type; 
		uint32_t binding; 

		FullDescriptorInfo(vk::DescriptorBufferInfo& bufferInfo, vk::DescriptorType type, uint32_t binding) : bufferInfo(bufferInfo), type(type), binding(binding) {
		}; 

		FullDescriptorInfo(vk::DescriptorImageInfo& imageInfo, vk::DescriptorType type, uint32_t binding) : imageInfo(imageInfo), type(type), binding(binding) {
		};

		vk::WriteDescriptorSet getSetInfo() {
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

	StarDevice& starDevice;
	StarDescriptorSetLayout& setLayout;
	StarDescriptorPool& pool;
	std::vector<FullDescriptorInfo> writeSets;

};
}