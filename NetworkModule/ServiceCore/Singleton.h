#pragma once

template <typename T> 
struct singleton_default
{ 
public: 
    typedef T object_type; 
    static object_type& instance() 
    { 
        static object_type obj; 
        create_object.do_nothing();
        return obj; 
    } 

private: 
    struct object_creator 
    { 
        object_creator() { singleton_default<T>::instance(); }
        inline void do_nothing() const {} 
    }; 
    static object_creator create_object; 
    singleton_default(); 
}; 

template <typename T> 
typename singleton_default<T>::object_creator 
singleton_default<T>::create_object;

#define INSTANCE(class_name) singleton_default<class_name>::instance()
