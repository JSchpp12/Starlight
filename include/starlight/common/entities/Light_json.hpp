#pragma once

#include "starlight/common/entities/Light.hpp"

#include <nlohmann/json.hpp>

namespace star
{
void to_json(nlohmann::json &j, const Light &l);
void from_json(const nlohmann::json &j, Light &l);
} // namespace star