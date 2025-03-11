#pragma once 

#include "StarDescriptorBuilders.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"
#include "ManagerDescriptorPool.hpp"
#include "Handle.hpp"
#include "ManagerRenderResource.hpp"

#include <memory>
#include <vector>

namespace star {
	class StarShaderInfo {
	private:
		struct ShaderInfo {
			struct BufferInfo{
				BufferInfo(const Handle& handle, const vk::Buffer& currentBuffer)
				: handle(handle), currentBuffer(currentBuffer){}

				BufferInfo(const Handle& handle) : handle(handle){}
				Handle handle;
				std::optional<vk::Buffer> currentBuffer;
			};

			struct TextureInfo{
				TextureInfo(const Handle& handle, const vk::ImageLayout& expectedLayout)
				: handle(handle), expectedLayout(expectedLayout){}

				TextureInfo(const StarTexture* texture, const vk::ImageLayout& expectedLayout) : texture(texture), expectedLayout(expectedLayout){}

				std::optional<const Handle> handle = std::nullopt;
				std::optional<vk::Image> currentImage = std::nullopt;
				
				std::optional<const StarTexture*> texture = std::nullopt;
				const vk::ImageLayout expectedLayout;
			};

			ShaderInfo(const BufferInfo& bufferInfo) 
				: bufferInfo(bufferInfo) {}

			ShaderInfo(const TextureInfo& textureInfo) 
				: textureInfo(textureInfo){};

			~ShaderInfo() = default;

			std::optional<BufferInfo> bufferInfo = std::nullopt;
			std::optional<TextureInfo> textureInfo 	= std::nullopt;
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

				if (shaderInfos[index].bufferInfo.has_value()) {
					auto& info = shaderInfos[index].bufferInfo.value();
					if (!ManagerRenderResource::isReady(info.handle)){
						ManagerRenderResource::waitForReady(info.handle);
					}
					auto& buffer = star::ManagerRenderResource::getBuffer(info.handle);

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
					else 
						texture = &star::ManagerRenderResource::getTexture(shaderInfos[index].bufferInfo.value().handle);

					auto textureInfo = vk::DescriptorImageInfo{
						texture->getSampler(),
						texture->getImageView(),
						shaderInfos[index].textureInfo.value().expectedLayout
					};

					this->descriptorWriter->writeImage(index, textureInfo);
				}
			}

			void build() {
				{
					this->isBuilt = true; 
					
					this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->layout, ManagerDescriptorPool::getPool());
					for (int i = 0; i < this->shaderInfos.size(); i++) {
						buildIndex(i); 
					}
				}
			};

			vk::DescriptorSet getDescriptorSet() {
				if (this->setNeedsRebuild){
					rebuildSet();
				}

				return *this->descriptorSet;
			};

			bool getIsBuilt() const {return this->isBuilt;}

		private:
			StarDevice& device;
			StarDescriptorSetLayout& layout; 
			bool setNeedsRebuild = true;
			bool isBuilt = false;
			std::shared_ptr<vk::DescriptorSet> descriptorSet = std::shared_ptr<vk::DescriptorSet>();
			std::shared_ptr<StarDescriptorWriter> descriptorWriter = std::shared_ptr<StarDescriptorWriter>();

			void rebuildSet() {
				if (!this->descriptorWriter){
					this->descriptorWriter = std::make_unique<StarDescriptorWriter>(this->device, this->layout, ManagerDescriptorPool::getPool());
				}

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
		};

		bool isReady(const uint8_t& frameInFlight){
			for (auto& set : this->shaderInfoSets[frameInFlight]){
				for (int i = 0; i < set->shaderInfos.size(); i++){
					if (set->shaderInfos.at(i).bufferInfo.has_value()){
						if (!ManagerRenderResource::isReady(set->shaderInfos.at(i).bufferInfo.value().handle))
						return false;
					}
				}
			}

			return true; 
		}

		std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts() {
			std::vector<vk::DescriptorSetLayout> fLayouts = std::vector<vk::DescriptorSetLayout>();
			for (auto& set : this->layouts) {
				fLayouts.push_back(set->getDescriptorSetLayout());
			}
			return fLayouts;
		};

		std::vector<vk::DescriptorSet> getDescriptors(const int& frameInFlight) {
			for (auto& set : this->shaderInfoSets[frameInFlight]) {
				if (!set->getIsBuilt())
					set->build();
				else{
					for (int i = 0; i < set->shaderInfos.size(); i++) {
						if (set->shaderInfos.at(i).bufferInfo.has_value()) {
							//check if buffer has changed
							auto& info = set->shaderInfos.at(i).bufferInfo.value();
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

			Builder& add(const Handle& bufferHandle) {
				this->activeSet->back()->add(ShaderInfo(ShaderInfo::BufferInfo{bufferHandle}));
				return *this;
			};

			Builder& add(const StarTexture& texture, const vk::ImageLayout& desiredLayout) {
				this->activeSet->back()->add(ShaderInfo(ShaderInfo::TextureInfo{&texture, desiredLayout}));
				return *this; 
			};

			Builder add(const Handle& textureHandle, const vk::ImageLayout& desiredLayout){
				this->activeSet->back()->add(ShaderInfo(ShaderInfo::TextureInfo{textureHandle, desiredLayout})); 
				return *this;
			}

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