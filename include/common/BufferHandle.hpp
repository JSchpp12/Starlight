#pragma once 

#include "Handle.hpp"
#include "StarDevice.hpp"

namespace star {
	struct BufferHandle : public Handle {
		BufferHandle(const uint32_t id, const size_t targetBufferOffset) : Handle(id, star::Handle_Type::buffer), targetBufferOffset(targetBufferOffset) {}
		BufferHandle(const uint32_t id) : Handle(id, star::Handle_Type::buffer) {};

		size_t targetBufferOffset = 0; 

	};
}