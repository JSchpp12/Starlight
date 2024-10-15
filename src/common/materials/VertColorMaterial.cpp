#include "VertColorMaterial.hpp"

void star::VertColorMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
}

vk::DescriptorSet star::VertColorMaterial::buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool, const int& imageInFlightIndex)
{
	return vk::DescriptorSet();
}

void star::VertColorMaterial::cleanup(StarDevice& device)
{

}

void star::VertColorMaterial::prep(StarDevice& device)
{

}

void star::VertColorMaterial::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
}
