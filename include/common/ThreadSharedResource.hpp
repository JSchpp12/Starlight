#pragma once

#include <boost/thread/mutex.hpp>

namespace star{
    template<typename T>
    class ThreadSharedResource{
        public:
        ThreadSharedResource(T* resource = nullptr) : resource(resource){}; 

        virtual ~ThreadSharedResource(){
            this->resource = nullptr;
        }

        void giveMeResource(boost::unique_lock<boost::mutex>& lock, T*& inResource){
            lock = boost::unique_lock<boost::mutex>(this->mutex); 

            if (this->resource != nullptr)
                inResource = this->resource; 
        }

        protected:
        T* resource = nullptr;
        boost::mutex mutex = boost::mutex();
    };
}