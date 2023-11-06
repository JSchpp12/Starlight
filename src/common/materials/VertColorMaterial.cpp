#include "VertColorMaterial.hpp"

void star::VertColorMaterial::prepRender(StarDevice& device)
{

}

void star::VertColorMaterial::getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout)
{

}

void star::VertColorMaterial::cleanupRender(StarDevice& device)
{

}

vk::DescriptorSet star::VertColorMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool)
{
	return vk::DescriptorSet();
}
