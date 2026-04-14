#pragma once

#include <nlohmann/json.hpp>

namespace star::service::scene_loader
{

class IEntityReader
{
  public:
    virtual ~IEntityReader() = default;

    virtual bool canLoad(const nlohmann::json &jData, const std::string &uniqueName) const noexcept = 0;
};
} // namespace star::service::scene_loader