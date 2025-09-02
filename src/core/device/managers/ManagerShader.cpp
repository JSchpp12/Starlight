#include "managers/ManagerShader.hpp"

#include <cassert> 

star::Handle star::core::device::manager::Shader::add(const StarShader &shader)
{
    uint32_t nextSpace;

    if (m_nextSpace >= m_shaders.size())
    {
        if (m_skippedSpaces.empty())
        {
            throw std::runtime_error("Shader storage is full");
        }
        else
        {
            nextSpace = m_skippedSpaces.top();
            m_skippedSpaces.pop(); 
        }
    }else{
        m_nextSpace++;
    }

    m_shaders.at(m_nextSpace) = Record{
        .shader = shader,
        .compiledShader = nullptr
    };

    return star::Handle(star::Handle_Type::shader, m_nextSpace); 
}

star::core::device::manager::Shader::Record &star::core::device::manager::Shader::get(const Handle &handle){
    assert(handle.getType() == star::Handle_Type::shader); 
    assert(handle.getID() < m_shaders.size()); 

    return m_shaders.at(handle.getID()); 
}