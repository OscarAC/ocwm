// <util/singleton.hpp>
// 5/24/2015 - File Creation
// Copyright(c) 2015-present, Oscar A. Carrera.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef UTIL_SINGLETON_H
#define UTIL_SINGLETON_H

#include <type_traits>

namespace util
{

/**
 * Scott Meyers Singleton Implementation
 * @tparam T - Type for the singleton
 */
template <typename T>
class singleton
{

    static_assert(std::is_class<T>::value, "Singleton needs a Class T.");

public:
    using type_ref = T &;
    using type_ptr = T *;

public:
    /**
     * Returns a Reference to the instance of this Singleton
     * @return Type Reference
     */
    template <typename... Args>
    static type_ref instance(Args... arg)
    {
        static T _instance{arg...};
        return _instance;
    }

private:
    /**
     * Private Constructor
     */
    singleton(){};

    /**
     * No Copy
     */
    singleton(singleton &) = delete;

    singleton &operator=(singleton &) = delete;

    /**
     * Private Descructor
     */
    ~singleton() {}
};

} // namespace util

#endif