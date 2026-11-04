#pragma once

#include "starlight/structs/SkyboxInfo.hpp"
#include "starlight/structs/SkyboxTextureOptions.hpp"

#include <nlohmann/json.hpp>

namespace star
{
void to_json(nlohmann::json &j, const SkyboxLayout &layout);
void from_json(const nlohmann::json &j, SkyboxLayout &layout);

void to_json(nlohmann::json &j, const SkyboxFormatScaling &formatScaling);
void from_json(const nlohmann::json &j, SkyboxFormatScaling &formatScaling);
} // namespace star