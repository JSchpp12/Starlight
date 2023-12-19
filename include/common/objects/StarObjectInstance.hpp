#pragma once 

#include "StarBuffer.hpp"
#include "StarEntity.hpp"
#include "StarDescriptors.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.hpp>

namespace star {
	/// @brief Class which contains object per instance information
	class StarObjectInstance : public StarEntity {
	public:
		StarObjectInstance(int instanceIndex) : instanceIndex(instanceIndex) {};
		~StarObjectInstance() = default;

		/// @brief Update the information in the corresponding buffer for this object.
		/// @param buffer The buffer to write to
		/// @param bufferIndex The corresponding index for this buffer. Will match the requested buffer info from getBufferInfoSize().
		virtual void updateBufferData(StarBuffer& buffer, int bufferIndex);

		/// @brief Size of memory needed for each instance. Number returned needs to match number of requested buffers
		/// @return 
		virtual std::vector<vk::DeviceSize> getBufferInfoSize() {
			return std::vector<vk::DeviceSize>{
				sizeof(glm::mat4),
				sizeof(glm::mat4)
			};
		};

	protected:
		/// @brief The index where this instance will be drawn. This is important when writing proper buffer info.
		int instanceIndex; 

		/// <summary>
		/// Object type to be used per render object, updated each frame
		/// </summary>
		struct InstanceUniformBufferObject {
			glm::mat4 modelMatrix;
			glm::mat4 normalMatrix;
		};

	};
}