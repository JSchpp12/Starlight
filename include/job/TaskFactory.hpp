#pragma once

#include "Task.hpp"

#include <iostream>
#include <string>

namespace star::job
{
struct PrintPayload
{
    std::string message;
    PrintPayload() = default;
    PrintPayload(std::string message) : message(std::move(message))
    {
    }
    ~PrintPayload() = default;
};

namespace TaskFactory
{
void printExecute(void *p);

star::job::Task<> createPrintTask(std::string message);
} // namespace TaskFactory
} // namespace star::Job
