#pragma once

#include "Handle.hpp"

#include <memory>

namespace star {
class StarManager {
public:
	StarManager() = default; 

	virtual void destroy(const Handle& resourceHandle) = 0; 

protected:
};
}