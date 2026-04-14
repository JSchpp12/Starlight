#pragma once

#include "starlight/common/entities/Light.hpp"

#include <nlohmann/json.hpp>

namespace star::service::scene_loader
{
class LightWriter
{
  public:
    nlohmann::json write(const std::vector<Light> &light) const noexcept; 
};
} // namespace star::service::scene_loader