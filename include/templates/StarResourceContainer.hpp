#pragma once 

#include "Handle.hpp"

#include <stdexcept>
#include <string> 
#include <memory>
#include <map>
#include <vector> 

namespace star {
    template<typename T>
    class StarResourceContainer {
    public:
        virtual ~StarResourceContainer() {};

        virtual size_t size() {
            return this->container.size();
        }

        virtual Handle add(std::unique_ptr<T> resource) {
            Handle newHandle;
            newHandle.containerIndex = container.size();
            this->container.push_back(std::move(resource));
            return newHandle;
        }

        virtual T& get(const Handle& handle) {
            if (handle.containerIndex < container.size()) {
                return *container[handle.containerIndex];
            }
            else {
                throw std::runtime_error("Requested a resource that is outside the range of the container");
            }
        }

    protected:
        std::vector<std::unique_ptr<T>> container;

    private:

    };
}