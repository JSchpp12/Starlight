#pragma once 

#include "StarEntity.hpp"

namespace star {
	/// @brief Class which contains object per instance information
	class StarObjectInstance : public StarEntity {
	public:
		StarObjectInstance() = default; 
		virtual ~StarObjectInstance() = default;
	};
}