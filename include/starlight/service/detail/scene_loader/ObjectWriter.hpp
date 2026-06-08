#pragma once

#include "starlight/object/StarObject.hpp"

#include <nlohmann/json.hpp>

namespace star::service::scene_loader
{
class ObjectWriter
{
  public:
    nlohmann::json write(const StarObject &object) const noexcept;
};
} // namespace star::service::scene_loader