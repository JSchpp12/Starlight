#pragma once

#include "CompleteTask.hpp"

namespace star::job::complete_tasks
{
struct SuccessPayload
{
    bool wasSuccessful;
};

namespace task_factory
{
star::job::complete_tasks::CompleteTask<> createStandardSuccess();

}
}; // namespace star::job::complete_tasks