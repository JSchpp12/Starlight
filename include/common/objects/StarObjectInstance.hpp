#pragma once 

#include "StarBuffer.hpp"
#include "StarEntity.hpp"
#include "StarDescriptorBuilders.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.hpp>

namespace star {
	/// @brief Class which contains object per instance information
	class StarObjectInstance : public StarEntity {
	public:
		StarObjectInstance(int instanceIndex) : instanceIndex(instanceIndex) {};
		~StarObjectInstance() = default;

	protected:
		/// @brief The index where this instance will be drawn. This is important when writing proper buffer info.
		int instanceIndex; 
	};
}