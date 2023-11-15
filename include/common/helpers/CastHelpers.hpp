#pragma once
#include <iostream>
#include <climits>

namespace star {
	class CastHelpers {
	public:
		static uint32_t size_t_to_unsigned_int(size_t org) {
			if (org > UINT_MAX) {
				throw std::runtime_error("Invalid cast");
			}
			return static_cast<uint32_t>(org);
		}
	};
}