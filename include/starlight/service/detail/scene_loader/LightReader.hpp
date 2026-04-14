#pragma once

#include "starlight/service/detail/scene_loader/IEntityReader.hpp"
#include "starlight/common/entities/Light.hpp"

#include <nlohmann/json.hpp>

namespace star::service::scene_loader
{
class LightReader : public IEntityReader
{
  public:
    virtual bool canLoad(const nlohmann::json &lightData, const std::string &uniqueName) const noexcept override; 

    void load(const nlohmann::json &lightDatas, const std::string &uniqueName, std::vector<Light> &light) const noexcept; 
};
} // namespace star::service::scene_writer