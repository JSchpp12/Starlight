#pragma once 

#include <star_common/Handle.hpp>
#include "StarManager.hpp"

#include <memory>

namespace star {
class ManagerPlug {
public:
	ManagerPlug() = default; 

	Handle& getHandle() const; 

	void setHandle(const Handle& resource); 

protected:
	std::unique_ptr<Handle> handle = nullptr;

};
}