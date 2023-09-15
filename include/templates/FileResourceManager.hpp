#pragma once 

#include "Handle.hpp"
#include "StarMemoryManager.hpp"
#include "FileResourceContainer.hpp"

#include <string> 
#include <memory>
#include <map>
#include <vector> 
#include <assert.h>

namespace star {
    template<typename T>
    class FileResourceManager : private StarMemoryManager<T> {
    public:
        FileResourceManager() : StarMemoryManager<T>() { }
        virtual ~FileResourceManager() {};

        virtual Handle addResource(const std::string& path, std::unique_ptr<T> newResource) {
            if (fileContainer.contains(path)) {
                return fileContainer.getHandle(path);
            }
            else {
                Handle newHandle = this->StarMemoryManager<T>::addResource(std::move(newResource));
                newHandle.isOnDisk = true;
                this->fileContainer.add(path, newHandle);
                return newHandle;
            }
        }

        virtual Handle addResource(std::unique_ptr<T> newResource) override { return this->StarMemoryManager<T>::addResource(std::move(newResource)); }
        /// <summary>
        /// Get reference to in memory resource 
        /// </summary>
        /// <param name="resourceHandle"></param>
        /// <returns></returns>
        virtual T& resource(const Handle& resourceHandle) {
            assert((resourceHandle.type == this->handleType() || resourceHandle.type == Handle_Type::defaultHandle) && "The requested resource handle does not match the type of resources managed by this manager.");
            return this->StarMemoryManager<T>::resource(resourceHandle);
        }
    protected:
        FileResourceContainer<T> fileContainer;
        virtual Handle createAppropriateHandle() override = 0;
        virtual Handle_Type handleType() = 0;

    private:

    };
}