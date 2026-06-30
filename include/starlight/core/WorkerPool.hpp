#pragma once

#include <cstdint>

namespace star::core
{
class WorkerPool
{
  public:
    explicit WorkerPool(uint8_t numSystemThreads)
        : m_numSystemThreads(numSystemThreads), m_numAvailable(numSystemThreads)
    {
    }

    bool allocateWorker()
    {
        if (m_numAvailable > 0)
        {
            m_numAvailable--;
            return true;
        }

        return false;
    }

    //simple counter -- trust that worker thread has actually be shutdown
    void returnWorker()
    {
        m_numAvailable++; 
    }

    uint8_t getNumAvailableWorkers() const
    {
        return m_numAvailable;
    }

    uint8_t getNumSystemThreads() const
    {
        return m_numSystemThreads;
    }

  private:
    uint8_t m_numSystemThreads;
    uint8_t m_numAvailable;
};
} // namespace star::core