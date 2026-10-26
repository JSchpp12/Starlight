#include "starlight/core/ShaderMacroDefinitions.hpp"

#include <utility>

namespace star::core
{
ShaderMacroDefinitions &ShaderMacroDefinitions::addMacroDefinition(ShaderMacroDefinition &&macroDefinition)
{
    m_macroDefinitions.emplace_back(std::move(macroDefinition));
    return *this;
}

ShaderMacroDefinitions &ShaderMacroDefinitions::addMacroDefinition(const ShaderMacroDefinition &macroDefinition)
{
    m_macroDefinitions.emplace_back(macroDefinition);
    return *this;
}

const star::core::ShaderMacroDefinition &ShaderMacroDefinitions::getMacroDefinition(const std::size_t &index) const
{
    return m_macroDefinitions.at(index);
}

std::size_t ShaderMacroDefinitions::getMacroDefinitionCount() const
{
    return m_macroDefinitions.size();
}
} // namespace star::core
