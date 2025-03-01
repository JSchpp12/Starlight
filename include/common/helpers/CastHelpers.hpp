#pragma once

#include <iostream>
#include <limits>

namespace star {
	class CastHelpers {
	public:
		static uint32_t size_t_to_unsigned_int(size_t org) {
			if (org > std::numeric_limits<uint32_t>::max()) {
				std::cerr << "Error: Size too large for uint32_t" << std::endl;
				exit(1);
			}

			return static_cast<uint32_t>(org);
		}
	};
}