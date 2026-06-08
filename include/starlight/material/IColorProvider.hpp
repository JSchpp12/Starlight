#pragma once


#include <starlight/structs/Color.hpp>

#include <vector>

namespace star::material
{
class IColorProvider
{
  public:
    virtual ~IColorProvider() = default; 
    virtual std::vector<star::Color> getColors() const = 0; 
};
}