#pragma once

#include "Enums.hpp"

namespace star {
    struct Handle {
        static Handle getDefault() {
            Handle newHandle;
            newHandle.type = Handle_Type::defaultHandle;
            return newHandle;
        }
        Handle() : global_id(id_counter++), id(0), type(Handle_Type::defaultHandle) {};
        Handle(const Handle& other) = default;
        Handle(const size_t id, const Handle_Type type) : isInit(true), global_id(id_counter++), id(id), type(type) {};
        Handle(const Handle_Type& type) : global_id(id_counter++), type(type) {};
        ~Handle() = default;


		bool operator==(const Handle& other) const {
			return global_id == other.global_id && id == other.id && type == other.type;
		}   

        Handle& operator=(const Handle& other){
            if (this != &other) {
                global_id = other.global_id;
                id = other.id;
                type = other.type;
                isInit = other.isInit;
            }
            return *this;
        }

        const bool& isInitialized(){return isInit;}

        const size_t& getID() const {return id;}
        const size_t& getGlobalID() const {return global_id;}
        const Handle_Type& getType() const {return type;}
        void setType(const Handle_Type& type) {
            this->isInit = true;
            this->type = type;
        }
        void setID(const size_t& id) {
            this->isInit = true;
            this->id = id;
        }

        private:

        static size_t id_counter; 
        bool isInit = false; 
        size_t id; 
        Handle_Type type = Handle_Type::null;
        size_t global_id;
    };

	struct HandleHash {
		size_t operator()(const Handle& handle) const noexcept;
    };

    inline bool operator<(const Handle& lhs, const Handle& rhs)
    {
        return lhs.getGlobalID() < rhs.getGlobalID();
    }
}