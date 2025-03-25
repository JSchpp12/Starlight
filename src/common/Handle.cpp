#include "Handle.hpp"

#include <boost/functional/hash/hash.hpp>

size_t star::Handle::id_counter = 0;

size_t star::HandleHash::operator()(const star::Handle& handle) const noexcept{
    size_t result = 0;
    boost::hash_combine(result, handle.getGlobalID());
    boost::hash_combine(result, handle.getID()); 
    boost::hash_combine(result, handle.getType());
    return result;
}