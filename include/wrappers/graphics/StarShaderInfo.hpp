#pragma once 

#include "StarDescriptorBuilders.hpp"
#include "BufferModifier.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"
#include "ManagerDescriptorPool.hpp"

#include <memory>
#include <vector>

namespace star {
	class StarShaderInfo {
	private:
		struct ShaderInfo {

			ShaderInfo(const BufferModifier& bufferModifier) 
				: bufferModifier(bufferModifier) {};

			ShaderInfo(const StarImage& textureInfo, const vk::ImageLayout& expectedLayout) 
				: textureInfo(textureInfo), expectedLayout(expectedLayout) {};

			~ShaderInfo() = default;

			std::optional<std::reference_wrapper<const BufferModifier>> bufferModifier = std::optional<std::reference_wrapper<const BufferModifier>>();
			std::optional<std::reference_wrapper<const StarImage>> textureInfo = std::optional<std::reference_wrapper<const StarImage>>();
			std::optional<vk::ImageLayout> expectedLayout = std::optional<vk::ImageLayout>();
		};

		struct ShaderInfoSet {
			std::vector<ShaderInfo> shaderInfos = std::vector<ShaderInfo>();

			ShaderInfoSet(StarDevice& device, StarDescriptorSetLayout& setLayout) : device(device), layout(setLayout) {};

			void add(const ShaderInfo& shaderInfo) {
				this->shaderInfos.push_back(shaderInfo);
			};

			void buildIndex(const int& index) {
				assert(this->descriptorWriter && "Dependencies must have been built first");
				this->setNeedsRebuild = true; 

				if (shaderInfos[index].bufferModifier.has_value()) {
					auto& handle = shaderInfos[index].bufferModifier.value().get().getHandle();
					auto& buffer = star::ManagerBuffer::getBuffer(handle);

					auto bufferInfo = vk::DescriptorBufferInfo{
						buffer.getBuffer(),
						0,
						buffer.getBufferSize()
					};
					this->descriptorWriter->writeBuffer(index, bufferInfo);
				}
				else if (shaderInfos[index].textureInfo.has_value()) {
					auto textureInfo = vk::DescriptorImageInfo{
						shaderInfos[index].textureInfo.value().get().getSampler(),
						shaderInfos[index].textureInfo.value().get().getImageView(),
						shaderInfos[index].expectedLayout.value()
					};

					this->descriptorWriter->writeImage(index, textureInfo);
				}
			}

			void build() {
				{
					this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->layout, ManagerDescriptorPool::getPool());
					for (int i = 0; i < this->shaderInfos.size(); i++) {
						buildIndex(i); 
					}
				}
			};

			vk::DescriptorSet getDescriptorSet() {
				if (this->setNeedsRebuild)
					rebuildSet();

				return *this->descriptorSet;
			};

		private:
			StarDevice& device;
			StarDescriptorSetLayout& layout; 
			bool setNeedsRebuild;
			std::shared_ptr<vk::DescriptorSet> descriptorSet = std::shared_ptr<vk::DescriptorSet>();
			std::shared_ptr<StarDescriptorWriter> descriptorWriter = std::shared_ptr<StarDescriptorWriter>();

			void rebuildSet() {
				this->descriptorSet = std::make_shared<vk::DescriptorSet>(this->descriptorWriter->build());
				this->setNeedsRebuild = false; 
			};
		};

		StarDevice& device;

		std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets = std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
		std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts = std::vector<std::shared_ptr<StarDescriptorSetLayout>>();

	public:
		StarShaderInfo(StarDevice& device, std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts, 
			std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets) 
			: device(device), layouts(layouts), shaderInfoSets(shaderInfoSets) {
			for (auto& set : this->shaderInfoSets){
				for (int i = 0; i < set.size(); i++) {
					set[i]->build();
				}
			}
		};

		std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts() {
			std::vector<vk::DescriptorSetLayout> fLayouts = std::vector<vk::DescriptorSetLayout>();
			for (auto& set : this->layouts) {
				fLayouts.push_back(set->getDescriptorSetLayout());
			}
			return fLayouts;
		};

		std::vector<vk::DescriptorSet> getDescriptors(const int& frameInFlight) {
			for (auto& set : this->shaderInfoSets[frameInFlight]) {
				for (int i = 0; i < set->shaderInfos.size(); i++) {
					if (set->shaderInfos.at(i).bufferModifier.has_value()) {
						//check if buffer has changed
						if (set->shaderInfos.at(i).bufferModifier.value().get().getBufferHasChanged()) {
							set->buildIndex(i);
						}
					}
				}
			}

			auto allSets = std::vector<vk::DescriptorSet>(); 
			for (int i = 0; i < this->shaderInfoSets[frameInFlight].size(); i++) {
				allSets.push_back(this->shaderInfoSets[frameInFlight].at(i)->getDescriptorSet()); 
			}

			return allSets; 
		};

		class Builder {
		public:
			Builder(StarDevice& device, const int numFramesInFlight) : device(device),
				sets(numFramesInFlight) {
			};

			Builder(Builder& other) : device(other.device) {
				this->layouts = other.layouts;
				this->sets = other.sets;
			};
			
			Builder& addSetLayout(std::shared_ptr<StarDescriptorSetLayout> layout) {
				this->layouts.push_back(layout);
				return *this;
			};

			Builder& startOnFrameIndex(const int& frameInFlight) {
				this->activeSet = &this->sets[frameInFlight];
				return *this; 
			}
			Builder& startSet() {
				auto size = this->activeSet->size();
				auto& test = this->layouts[size]; 
				this->activeSet->push_back(std::make_shared<ShaderInfoSet>(this->device, *test));
				return *this;
			};

			Builder& add(const BufferModifier& bufferModifier) {
				this->activeSet->back()->add(ShaderInfo(bufferModifier));
				return *this;
			};

			Builder& add(const StarImage& texture, const vk::ImageLayout& desiredLayout) {
				this->activeSet->back()->add(ShaderInfo(texture, desiredLayout));
				return *this; 
			};

			std::unique_ptr<StarShaderInfo> build() {
				return std::make_unique<StarShaderInfo>(this->device, std::move(this->layouts), std::move(this->sets));
			};

			std::vector<std::shared_ptr<StarDescriptorSetLayout>> getCurrentSetLayouts() {
				return this->layouts;
			};

		private:
			StarDevice& device;
			std::vector<std::shared_ptr<ShaderInfoSet>>* activeSet = nullptr; 

			std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> layouts = std::vector<std::shared_ptr<StarDescriptorSetLayout>>();
			std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> sets = std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
		};
	};
}