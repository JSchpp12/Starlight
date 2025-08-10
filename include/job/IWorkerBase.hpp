#pragma once

namespace star::Job{
    class IWorkerBase{
        public:
            virtual ~IWorkerBase() = default;
            virtual void start() = 0; 
            virtual void stop() = 0; 
    };
}