#pragma once 

#include "StarDescriptorBuilders.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"

#include "Handle.hpp"

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

			void add(const ShaderInfo& shaderInfo);

			void buildIndex(const int& index);

			void build();

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

			void rebuildSet();
		};

		StarDevice& device;

		std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets = std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>>();
		std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts = std::vector<std::shared_ptr<StarDescriptorSetLayout>>();

	public:
		StarShaderInfo(StarDevice& device, std::vector<std::shared_ptr<StarDescriptorSetLayout>> layouts, 
			std::vector<std::vector<std::shared_ptr<ShaderInfoSet>>> shaderInfoSets) 
			: device(device), layouts(layouts), shaderInfoSets(shaderInfoSets) {
		};

		bool isReady(const uint8_t& frameInFlight);

		std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts();

		std::vector<vk::DescriptorSet> getDescriptors(const int& frameInFlight);

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