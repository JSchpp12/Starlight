#include "VertColorMaterial.hpp"

void star::VertColorMaterial::getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout)
{

}


vk::DescriptorSet star::VertColorMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool)
{
	return vk::DescriptorSet();
}

void star::VertColorMaterial::cleanup(StarDevice& device)
{

}

void star::VertColorMaterial::prep(StarDevice& device)
{

}
