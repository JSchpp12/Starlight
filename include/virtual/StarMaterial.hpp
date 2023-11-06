#pragma once

#include "StarMaterialMesh.hpp"
#include "StarShader.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace star {
	class StarMaterial {
	public:
		glm::vec4 surfaceColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 highlightColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 ambient{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 diffuse{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 specular{ 0.5f, 0.5f, 0.5f, 1.0f };
		int shinyCoefficient = 1;

		StarMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny) : surfaceColor(surfaceColor), highlightColor(highlightColor),
			ambient(ambient), diffuse(diffuse),
			specular(specular), shinyCoefficient(shiny) {}; 

		StarMaterial() = default; 

		virtual ~StarMaterial() = default; 

		//virtual std::unique_ptr<StarMaterialMesh> getMeshMaterials(StarDevice& device) = 0; 

		/// <summary>
		/// Entry point for rendering preparations
		/// </summary>
		/// <param name="device"></param>
		void prepareRender(StarDevice& device); 

		virtual void getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout) = 0;

		virtual void buildDescriptorSets(StarDevice& device, StarDescriptorSetLayout& groupLayout,
			StarDescriptorPool& groupPool, std::vector<std::vector<vk::DescriptorSet>> globalSets, 
			int numSwapChainImages);

		virtual void bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex); 

		/// <summary>
		/// Cleanup any vulkan objects created by this material
		/// </summary>
		/// <param name="device"></param>
		virtual void cleanupRender(StarDevice& device)=0; 


	protected:

		/// <summary>
		/// Function which should contain processes to create all needed functionalities
		/// for rendering operations. Example is creating needed rendering textures for 
		/// and gpu memory. 
		/// </summary>
		/// <param name="device">Device that is being used in rendering operations</param>
		virtual void prepRender(StarDevice& device) = 0;

		//Map of swap chain image index to each descriptor set 
		std::unordered_map<int, std::vector<vk::DescriptorSet>> descriptorSets;

		/// <summary>
		/// Each material should build a descriptor set for each swap chain image. This 
		/// function should provide a single descriptor set for use. 
		/// </summary>
		/// <param name="device"></param>
		/// <param name=""></param>
		/// <returns></returns>
		virtual vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout,
			StarDescriptorPool& groupPool) = 0;

	private:
		//flag to determine if the material has been prepped for rendering operations
		bool isPrepared = false;
	};
}