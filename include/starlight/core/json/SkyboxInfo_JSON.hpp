#pragma once

#include "starlight/core/json/SkyboxEnums_JSON.hpp"
#include "starlight/structs/SkyboxInfo.hpp"

#include <nlohmann/json.hpp>

namespace star
{
void to_json(nlohmann::json &j, const SkyboxInfo &info);
void from_json(const nlohmann::json &j, SkyboxInfo &info);
} // namespace star
