#pragma once

#include "StarShader.hpp"

#include <Handle.hpp>

#include <array>
#include <memory>
#include <vector>
#include <stack>

namespace star::core::device::manager
{
class Shader
{
  public:
    struct Record
    {
        StarShader shader; 
        std::unique_ptr<std::vector<uint32_t>> compiledShader; 
    };

    Handle add(const StarShader &shader); 

    Record& get(const Handle &handle); 

  private:
    std::stack<size_t> m_skippedSpaces; 
    std::array<Record, 50> m_shaders;
    uint32_t m_nextSpace = 0; 
};
} // namespace star::core::device::manager