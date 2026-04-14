#include "starlight/service/detail/scene_loader/LightReader.hpp"

#include "starlight/common/entities/Light_json.hpp"

namespace star::service::scene_loader
{
bool LightReader::canLoad(const nlohmann::json &lightData, const std::string &uniqueName) const noexcept
{
    return true;
}

void LightReader::load(const nlohmann::json &lightDatas, const std::string &uniqueName, std::vector<Light> &light) const noexcept
{
    if (!lightDatas.contains(uniqueName))
    {
        return;
    }

    const auto &lData = lightDatas[uniqueName]; 
    light = lData.get<std::vector<Light>>(); 

    //TODO: need to properly handle rotation applications like the object reader... omitting due to time constraints
}
} // namespace star::service::scene_writer