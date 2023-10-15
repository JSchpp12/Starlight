#pragma once

#include "CastHelpers.hpp"
#include "FileHelpers.hpp"
#include "StarMesh.hpp"
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

		const std::vector<std::unique_ptr<StarMesh>>& getMeshes() override { return this->meshes; };
	protected:
		BasicObject(std::string objectFilePath);

		std::string objectFilePath;
		std::vector<BumpMaterial> materials; 
		std::vector<std::unique_ptr<StarMesh>> meshes;


		/// <summary>
		/// Load meshes from file
		/// </summary>
		/// <param name="objectFilePath">Path of the file to load</param>
		/// <returns></returns>
		void loadFromFile(const std::string objectFilePath);

		// Inherited via StarObject
		void prepRender(StarDevice& device) override;
		void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constLayout) override;
		void initDescriptors(StarDevice& device, StarDescriptorSetLayout& constLayout, StarDescriptorPool& descriptorPool) override;
		void render(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum) override;
	};
}