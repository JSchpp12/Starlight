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
void printExecute(void *p)
{
  auto* payload = static_cast<PrintPayload*>(p); 
  std::cout << "Print task: " << payload->message << std::endl; 
}

star::job::Task<> createPrintTask(std::string message){
  return star::job::Task<>::Builder<PrintPayload>()
    .setPayload(PrintPayload(std::move(message)))
    .setExecute(&printExecute)
    .build();
}
} // namespace TaskFactory
} // namespace star::Job
