#pragma once

#include "starlight/material/IColorProvider.hpp"

namespace star::material
{
class DefaultColorProvider : public IColorProvider
{
    std::vector<star::Color> m_colors; 

  public:
    explicit DefaultColorProvider(std::vector<star::Color> colors) : m_colors(std::move(colors))
    {
    }

    virtual std::vector<star::Color> getColors() const override
    {
        return m_colors; 
    }
};
}