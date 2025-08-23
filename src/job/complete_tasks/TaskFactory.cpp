#include "complete_tasks/TaskFactory.hpp"

star::job::complete_tasks::CompleteTask<> star::job::complete_tasks::task_factory::createStandardSuccess(){
    return complete_tasks::CompleteTask<>::Builder<SuccessPayload>()
        .setPayload(SuccessPayload())
        .setCompleteFunction([](void *p){
            auto *payload = static_cast<SuccessPayload *>(p);
        })
        .build();
}