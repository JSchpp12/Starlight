#pragma once

#include "StarDevice.hpp"
#include "StarObject.hpp"

#include <tiny_obj_loader.h>

#include <memory>
#include <string>
#include <vector>

namespace star {
	class BasicObject : public StarObject {
	public:
		static std::unique_ptr<BasicObject> New(std::string objPath); 

		virtual ~BasicObject() = default; 

		void prepRender(StarDevice& device) override;

		void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constLayout) override;

		void initDescriptors(StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool) override;

		void render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum) override;

	protected:
		BasicObject(std::vector<std::unique_ptr<Mesh>> meshes)
			: StarObject(std::move(meshes)) {}; 

		/// <summary>
		/// Load meshes from file
		/// </summary>
		/// <param name="objectFilePath">Path of the file to load</param>
		/// <returns></returns>
		static std::vector<std::unique_ptr<Mesh>> loadFromFile(const std::string objectFilePath);
	};
}