#pragma once 

#include "glm/glm.hpp"

namespace star {
	struct Ray {
		glm::vec3 org{}, dir{}, invDir{};
		int sign[3];

		Ray(const glm::vec3& org, const glm::vec3& dir)
			: org(org), dir(dir), invDir(1.0f / dir),
			sign{invDir.x < 0, invDir.y < 0, invDir.z < 0}
		{
		}
	};
}