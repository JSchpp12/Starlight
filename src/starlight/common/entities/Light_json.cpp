#include "starlight/common/entities/Light_json.hpp"

#include "starlight/core/json/glm_json.hpp"

namespace star
{
void to_json(nlohmann::json &j, const Type::Light &t)
{
    switch (t)
    {
    case Type::Light::point:
        j = "point";
        break;
    case Type::Light::directional:
        j = "directional";
        break;
    case Type::Light::spot:
        j = "spot";
        break;
    default:
        j = "unknown";
        break;
    }
}

void from_json(const nlohmann::json &j, Type::Light &t)
{
    if (j.is_number_integer())
    {
        // Handle legacy integer values
        switch (j.get<int>())
        {
        case 0:
            t = Type::Light::point;
            break;
        case 1:
            t = Type::Light::directional;
            break;
        case 2:
            t = Type::Light::spot;
            break;
        default:
            throw std::invalid_argument("Unknown Type::Light integer value: " + std::to_string(j.get<int>()));
        }
    }
    else if (j.is_string())
    {
        const std::string s = j.get<std::string>();
        if (s == "point")
            t = Type::Light::point;
        else if (s == "directional")
            t = Type::Light::directional;
        else if (s == "spot")
            t = Type::Light::spot;
        else
            throw std::invalid_argument("Unknown Type::Light string value: " + s);
    }
    else
    {
        throw nlohmann::json::type_error::create(302, "Type::Light must be a string or integer", &j);
    }
}

void to_json(nlohmann::json &j, const Light &light)
{
    j = nlohmann::json{// StarEntity base field
                       {"position", light.getPosition()},

                       // Light-specific fields
                       {"direction", light.getDirection()},
                       {"ambient", light.getAmbient()},
                       {"diffuse", light.getDiffuse()},
                       {"specular", light.getSpecular()},
                       {"innerDiameter", light.getInnerDiameter()},
                       {"outerDiameter", light.getOuterDiameter()},
                       {"luminance", light.getLuminance()},
                       {"type", light.getType()},
                       {"enabled", light.getEnabled()}};
}

void from_json(const nlohmann::json &j, Light &light)
{
    // StarEntity base field
    if (j.contains("position"))
        light.setPosition(j.at("position").get<glm::vec3>());

    // Light-specific fields
    if (j.contains("direction"))
        light.setDirection(j.at("direction").get<glm::vec3>());

    if (j.contains("ambient"))
        light.setAmbient(j.at("ambient").get<glm::vec3>());

    if (j.contains("diffuse"))
        light.setDiffuse(j.at("diffuse").get<glm::vec3>());

    if (j.contains("specular"))
        light.setSpecular(j.at("specular").get<glm::vec3>());

    if (j.contains("innerDiameter"))
        light.setInnerDiameter(j.at("innerDiameter").get<float>());

    if (j.contains("outerDiameter"))
        light.setOuterDiameter(j.at("outerDiameter").get<float>());

    if (j.contains("luminance"))
        light.setLuminance(j.at("luminance").get<uint32_t>());

    if (j.contains("type"))
        light.setType(j.at("type").get<Type::Light>());

    if (j.contains("enabled"))
        light.setEnabled(j.at("enabled").get<bool>());
}

} // namespace star
