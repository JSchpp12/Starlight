#pragma once

#include <boost/thread/mutex.hpp>

namespace star{
    template<typename T>
    class ThreadSharedResource{
        public:
        ThreadSharedResource(T* resource) : resource(resource){}; 

        virtual ~ThreadSharedResource(){
            this->destroyResource(); 
            this->resource = nullptr;
        }

        void giveMeResource(boost::unique_lock<boost::mutex>& lock, T* resource){
            assert(this->resource != nullptr && "Resource was not properly initalized.");

            lock = boost::unique_lock<boost::mutex>(this->mutex); 
            resource = this->resource; 
        }

        protected:
        virtual void destroyResource() = 0; 

        T* resource = nullptr;
        boost::mutex mutex = boost::mutex();
    };
}