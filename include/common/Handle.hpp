#pragma once 
#include "Enums.hpp"

#include <optional>
#include <stdint.h>

#include <boost/functional/hash/hash.hpp>

//template<>
//struct std::hash<star::Handle> {
//    size_t operator()(const star::Handle& handle) const {
//        size_t result = 0;
//        boost::hash_combine(result, handle.id);
//        boost::hash_combine(result, handle.type);
//        return result;
//    }
//};

namespace star {
    struct Handle {
        static Handle getDefault() {
            Handle newHandle;
            newHandle.type = Handle_Type::defaultHandle;
            return newHandle;
        }
        Handle() : global_id(id_counter++), id(0), type(Handle_Type::defaultHandle) {};
        Handle(const Handle& handle) = default;
        Handle(const size_t id, const Handle_Type type) : global_id(id_counter++), id(id), type(type) {};

		bool operator==(const Handle& other) const {
			return global_id == other.global_id && id == other.id && type == other.type;
		}   

        static size_t id_counter; 
        size_t id; 
        Handle_Type type = Handle_Type::null;
        size_t global_id;
    };

	struct HandleHash {
		size_t operator()(const Handle& handle) const noexcept  {
			size_t result = 0;
			boost::hash_combine(result, handle.global_id);
            boost::hash_combine(result, handle.id); 
			boost::hash_combine(result, handle.type);
			return result;
		}
	};
}