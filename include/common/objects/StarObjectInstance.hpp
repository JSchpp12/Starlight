#pragma once 

#include "StarBuffers/Buffer.hpp"
#include "StarEntity.hpp"
#include "StarDescriptorBuilders.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.hpp>

namespace star {
	/// @brief Class which contains object per instance information
	class StarObjectInstance : public StarEntity {
	public:
		StarObjectInstance() = default; 
		virtual ~StarObjectInstance() = default;
	};
}