#include "device/managers/ManagerShader.hpp"

#include "job/tasks/TaskFactory.hpp"

#include <cassert>

star::Handle star::core::device::manager::Shader::submit(job::TaskManager &taskSystem, StarShader shader)
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
    }
    else
    {
        nextSpace = m_nextSpace;
        m_nextSpace++; 
    }

    m_shaders.at(nextSpace) = Record(shader);

    Handle handle = Handle(star::Handle_Type::shader, nextSpace); 
    submitTask(taskSystem, m_shaders.at(nextSpace), handle); 

    return handle;
}

star::core::device::manager::Shader::Record &star::core::device::manager::Shader::get(const Handle &handle)
{
    assert(handle.getType() == star::Handle_Type::shader);
    assert(handle.getID() < m_shaders.size());

    return m_shaders.at(handle.getID());
}

bool star::core::device::manager::Shader::isReady(const Handle &handle){
    return get(handle).compiledShader != nullptr; 
}

void star::core::device::manager::Shader::submitTask(job::TaskManager &taskSystem, Record &storedRecord, const Handle &handle)
{
    taskSystem.submitTask(
        job::tasks::task_factory::CreateCompileShader(storedRecord.shader.getPath(), storedRecord.shader.getStage(), handle));
}