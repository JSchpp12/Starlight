#pragma once

#include <optional>
#include <string>
#include <vector>

namespace star::core
{
struct ShaderMacroDefinition
{
    std::string name;
    std::optional<std::string> value;
};

class ShaderMacroDefinitions
{
    std::vector<ShaderMacroDefinition> m_macroDefinitions;

  public:
    ShaderMacroDefinitions &addMacroDefinition(ShaderMacroDefinition &&macroDefinition);
    ShaderMacroDefinitions &addMacroDefinition(const ShaderMacroDefinition &macroDefinition);
    const ShaderMacroDefinition &getMacroDefinition(const std::size_t &index) const;
    std::size_t getMacroDefinitionCount() const;
};
} // namespace star::core
