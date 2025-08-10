#pragma once

#include "Task.hpp"

#include <iostream>
#include <string>

namespace star::Job
{
class PrintTask : public Task<std::string>
{
  public:
    PrintTask(std::string myString) : Task<std::string>(myString)
    {
    }
    virtual ~PrintTask() = default;

  protected:
    virtual void run(std::string &payload) override
    {
        std::cout << payload << std::endl;
    }
};
} // namespace star::Job