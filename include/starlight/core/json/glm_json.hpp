#pragma once

#include <nlohmann/json.hpp>

#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace glm
{

void to_json(nlohmann::json &j, const glm::vec2 &v);
void to_json(nlohmann::json &j, const glm::vec3 &v);
void to_json(nlohmann::json &j, const glm::vec4 &v);
void to_json(nlohmann::json &j, const glm::mat3 &m);
void to_json(nlohmann::json &j, const glm::mat4 &m);

void from_json(const nlohmann::json &j, glm::vec2 &v);
void from_json(const nlohmann::json &j, glm::vec3 &v);
void from_json(const nlohmann::json &j, glm::vec4 &v);
void from_json(const nlohmann::json &j, glm::mat3 &m);
void from_json(const nlohmann::json &j, glm::mat4 &m);

} // namespace glm