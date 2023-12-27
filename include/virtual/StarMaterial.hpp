#pragma once

#include "StarMaterialMesh.hpp"
#include "StarShader.hpp"
#include "StarDevice.hpp"
#include "StarTexture.hpp"
#include "StarCommandBuffer.hpp"
#include "RenderResourceModifier.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace star {
	class StarMaterial : private RenderResourceModifier{
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

		/// <summary>
		/// Engine entrypoint for cleanup operations
		/// </summary>
		void cleanupRender(StarDevice& device); 

		/// <summary>
		/// Entry point for rendering preparations
		/// </summary>
		/// <param name="device"></param>
		void prepRender(StarDevice& device); 

		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) = 0; 

		/// @brief Create descriptor sets which will be used when this material is bound. Make sure that all global sets are provided 
		/// @param device 
		/// @param groupLayout 
		/// @param groupPool 
		/// @param globalSets 
		/// @param numSwapChainImages 
		virtual void finalizeDescriptors(StarDevice& device, std::vector<std::reference_wrapper<StarDescriptorSetLayout>> groupLayouts,
			StarDescriptorPool& groupPool, std::vector<std::unordered_map<int, vk::DescriptorSet>> globalSets,
			int numSwapChainImages);

		virtual void bind(StarCommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex); 

	protected:

		/// <summary>
		/// Function which should contain processes to create all needed functionalities
		/// for rendering operations. Example is creating needed rendering textures for 
		/// and gpu memory. 
		/// </summary>
		/// <param name="device">Device that is being used in rendering operations</param>
		virtual void prep(StarDevice& device) = 0;

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


		/// <summary>
		/// Cleanup any vulkan objects created by this material
		/// </summary>
		/// <param name="device"></param>
		virtual void cleanup(StarDevice& device)=0;

		virtual void initResources(int numFramesInFlight) override {};

	private:
		//flag to determine if the material has been prepped for rendering operations
		bool isPrepared = false;
	};
}