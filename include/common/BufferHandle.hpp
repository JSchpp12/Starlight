#pragma once 

#include "Handle.hpp"
#include "StarDevice.hpp"

namespace star {
	struct BufferHandle : public Handle {
		BufferHandle(const uint32_t &id, const size_t &targetBufferOffset) : Handle(star::Handle_Type::buffer, id), targetBufferOffset(targetBufferOffset) {}
		BufferHandle(const uint32_t id) : Handle(star::Handle_Type::buffer, id) {};

		size_t targetBufferOffset = 0; 
	};
}