#pragma once

#include "Handle.hpp"

#include <vector>
#include <memory>
#include <unordered_map>

namespace star{
    /// @brief Container for handle matched resources
    template <typename T>
    class ManagerStorageContainer{
        public:
        ManagerStorageContainer() : allElements(std::vector<std::unique_ptr<T>>(500)){};
        ~ManagerStorageContainer() = default;

        std::unique_ptr<T>& get(const Handle& handle){
            if (isStatic(handle))
                return *this->handleToStaticMap[handle];
            else
                return *this->handleToDynamicMap[handle];
        }

        void add(std::unique_ptr<T> element, const bool& isStatic, Handle& handle){
            this->allElements.at(this->elementCounter) = std::move(element);

            if (isStatic){
                handle.setID(this->staticCounter);
                this->staticCounter += 2;
        
                this->handleToStaticMap.emplace(std::make_pair(handle, &allElements.at(this->elementCounter)));
            }else{
                handle.setID(this->updateableCounter);
                this->updateableCounter += 2;
        
                this->handleToDynamicMap.emplace(std::make_pair(handle, &allElements.at(this->elementCounter)));
            }

            this->elementCounter++;
        }

        bool isStatic(const Handle& handle) const
        {
            return handle.getID() % 2 == 0;
        }

        void destroy(const Handle& handle){
            std::unique_ptr<T>* container = nullptr;

            if (isStatic(handle)){
                container = this->handleToStaticMap.find(handle)->second;
            }else{
                container = this->handleToDynamicMap.find(handle)->second;
            }
            container->reset();
        }

        std::unordered_map<Handle, std::unique_ptr<T>*, HandleHash>& getDynamicMap(){return this->handleToDynamicMap;}

        private:
        int staticCounter = 0;
        int updateableCounter = 1;
        int elementCounter = 0;
        std::vector<std::unique_ptr<T>> allElements = std::vector<std::unique_ptr<T>>();
        std::unordered_map<Handle, std::unique_ptr<T>*, HandleHash> handleToDynamicMap = std::unordered_map<Handle, std::unique_ptr<T>*, HandleHash>();
        std::unordered_map<Handle, std::unique_ptr<T>*, HandleHash> handleToStaticMap = std::unordered_map<Handle, std::unique_ptr<T>*, HandleHash>();
    };
}
