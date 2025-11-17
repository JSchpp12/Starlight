#pragma once

#include <starlight/common/Handle.hpp>
#include "device/StarDevice.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace star
{
/// @brief Container for handle matched resources
template <typename T> class ManagerStorageContainer
{
  public:
    ManagerStorageContainer() : allElements(std::vector<std::unique_ptr<T>>(4000)) {};
    ~ManagerStorageContainer() = default;

    ManagerStorageContainer(const ManagerStorageContainer &) = delete;
    ManagerStorageContainer &operator=(const ManagerStorageContainer &) = delete;

    ManagerStorageContainer(const ManagerStorageContainer &&other)
        : staticCounter(other.staticCounter), updateableCounter(other.updateableCounter),
          elementCounter(other.elementCounter), allElements(std::move(other.allElements)),
          handleToDynamicMap(other.handleToDynamicMap), handleToStaticMap(other.handleToStaticMap)
    {
    }

    ManagerStorageContainer &operator=(const ManagerStorageContainer &&other)
    {
        if (this != other)
        {
            staticCounter = other.staticCounter;
            updateableCounter = other.updateableCounter;
            elementCounter = other.elementCounter;
            allElements = std::move(other.allElements);
            handleToDynamicMap = other.handleToDynamicMap;
            handleToStaticMap = other.handleToStaticMap;
        }

        return *this;
    }

    std::unique_ptr<T> &get(const Handle &handle)
    {
        if (isStatic(handle))
            return this->allElements.at(this->handleToStaticMap[handle]);
        else
            return this->allElements.at(this->handleToDynamicMap[handle]);
    }

    std::unique_ptr<T> &at(const size_t &index)
    {
        assert(index < this->allElements.size() && "Beyond index of elements");

        return this->allElements.at(index);
    }

    void add(std::unique_ptr<T> element, const bool &isStatic, Handle &handle)
    {
        this->allElements.at(this->elementCounter) = std::move(element);

        if (isStatic)
        {
            handle.id = this->staticCounter;
            this->staticCounter += 2;

            this->handleToStaticMap.emplace(std::make_pair(handle, this->elementCounter));
        }
        else
        {
            handle.id = this->updateableCounter;
            this->updateableCounter += 2;

            this->handleToDynamicMap.emplace(std::make_pair(handle, this->elementCounter));
        }

        this->elementCounter++;
    }

    bool isStatic(const Handle &handle) const
    {
        return handle.getID() % 2 == 0;
    }

    void destroy(const Handle &handle)
    {
        std::unique_ptr<T> *container = nullptr;

        if (isStatic(handle))
        {
            container = &this->allElements.at(this->handleToStaticMap.find(handle)->second);
        }
        else
        {
            container = &this->allElements.at(this->handleToDynamicMap.find(handle)->second);
        }
        container->reset();
    }

    std::unordered_map<Handle, size_t, HandleHash> &getDynamicMap()
    {
        return this->handleToDynamicMap;
    }

    void cleanup(core::device::StarDevice &device){
        for (int i = 0; i < allElements.size(); i++){
            allElements.at(i).reset(); 
        }
    }

  private:
    int staticCounter = 0;
    int updateableCounter = 1;
    int elementCounter = 0;
    std::vector<std::unique_ptr<T>> allElements = std::vector<std::unique_ptr<T>>();
    std::unordered_map<Handle, size_t, HandleHash> handleToDynamicMap =
        std::unordered_map<Handle, size_t, HandleHash>();
    std::unordered_map<Handle, size_t, HandleHash> handleToStaticMap = std::unordered_map<Handle, size_t, HandleHash>();
};
} // namespace star
