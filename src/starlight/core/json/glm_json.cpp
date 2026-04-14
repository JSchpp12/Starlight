#include "starlight/core/json/glm_json.hpp"

namespace glm
{

// ---- to_json ----

void to_json(nlohmann::json &j, const glm::vec2 &v)
{
    j = nlohmann::json{{"x", v.x}, {"y", v.y}};
}

void to_json(nlohmann::json &j, const glm::vec3 &v)
{
    j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

void to_json(nlohmann::json &j, const glm::vec4 &v)
{
    j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}};
}

void to_json(nlohmann::json &j, const glm::mat3 &m)
{
    j = nlohmann::json::array();
    for (int col = 0; col < 3; ++col)
        for (int row = 0; row < 3; ++row)
            j[col][row] = m[col][row];
}

void to_json(nlohmann::json &j, const glm::mat4 &m)
{
    j = nlohmann::json::array();
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            j[col][row] = m[col][row];
}

// ---- from_json ----

void from_json(const nlohmann::json &j, glm::vec2 &v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

void from_json(const nlohmann::json &j, glm::vec3 &v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

void from_json(const nlohmann::json &j, glm::vec4 &v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
    j.at("w").get_to(v.w);
}

void from_json(const nlohmann::json &j, glm::mat3 &m)
{
    for (int col = 0; col < 3; ++col)
        for (int row = 0; row < 3; ++row)
            m[col][row] = j.at(col).at(row).get<float>();
}

void from_json(const nlohmann::json &j, glm::mat4 &m)
{
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            m[col][row] = j.at(col).at(row).get<float>();
}

} // namespace glm