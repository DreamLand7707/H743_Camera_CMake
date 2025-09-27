/*
 * memory_pool_allocator.hpp
 *
 *  Created on: Dec 18, 2024
 *      Author: DrL潇湘
 */

#ifndef MEMORY_POOL_MEMORY_POOL_ALLOCATOR_HPP_
#define MEMORY_POOL_MEMORY_POOL_ALLOCATOR_HPP_


#include "memory_pool.hpp"

#include <vector>
#include <string>
#include <deque>
#include <list>
#include <forward_list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

template <class T>
class mpc_allocator {
    memory_pool_context *ptr_mpc;

 public:
    using pointer                                = T *;
    using const_pointer                          = const T *;
    using void_pointer                           = void *;
    using const_void_pointer                     = const void *;
    using value_type                             = T;
    using size_type                              = size_t;
    using difference_type                        = std::ptrdiff_t;

    mpc_allocator()                              = delete;
    mpc_allocator(const mpc_allocator<T> &other) = default;
    explicit mpc_allocator(memory_pool_context &instance) : ptr_mpc(&instance) {}
    explicit mpc_allocator(memory_pool_context *ptr) : ptr_mpc(ptr) {}
    ~mpc_allocator() = default;
    template <class U>
    explicit mpc_allocator(const mpc_allocator<U> &other) : ptr_mpc(other.ptr_mpc) {}

    pointer allocate(size_type n) const {
        return static_cast<pointer>(memory_pool_malloc_context(ptr_mpc, (uintptr_t)(n * sizeof(T))));
    }

    void deallocate(pointer p, size_type n) const {
        memory_pool_free_context(ptr_mpc, p);
    }
};

class mpc_deleter {
    memory_pool_context *ptr_mpc;

 public:
    mpc_deleter()  = delete;
    ~mpc_deleter() = default;
    explicit mpc_deleter(memory_pool_context *ptr) : ptr_mpc(ptr) {}
    explicit mpc_deleter(memory_pool_context &instance) : ptr_mpc(&instance) {}

    void operator()(void *ptr) {
        memory_pool_free_context(ptr_mpc, ptr);
    }
};

template <class Key>
class mpc_hash;

template <class T>
using mpc_vector = std::vector<T, mpc_allocator<T>>;

template <class T>
using mpc_deque = std::deque<T, mpc_allocator<T>>;

template <class T>
using mpc_list = std::list<T, mpc_allocator<T>>;

template <class T>
using mpc_forward_list = std::forward_list<T, mpc_allocator<T>>;

template <class T>
using mpc_set = std::set<T, std::less<T>, mpc_allocator<T>>;

template <class T>
using mpc_multiset = std::multiset<T, std::less<T>, mpc_allocator<T>>;

template <class Key, class Value>
using mpc_map = std::map<Key, Value, std::less<Key>, mpc_allocator<std::pair<const Key, Value>>>;

template <class Key, class Value>
using mpc_multimap = std::multimap<Key, Value, std::less<Key>, mpc_allocator<std::pair<const Key, Value>>>;

template <class T>
using mpc_unordered_set = std::unordered_set<T, mpc_hash<T>, std::equal_to<T>, mpc_allocator<T>>;

template <class T>
using mpc_unordered_multiset = std::unordered_multiset<T, mpc_hash<T>, std::equal_to<T>, mpc_allocator<T>>;

template <class Key, class Value>
using mpc_unordered_map = std::unordered_map<Key, Value, mpc_hash<Key>, std::equal_to<Key>, mpc_allocator<std::pair<const Key, Value>>>;

template <class Key, class Value>
using mpc_unordered_multimap = std::unordered_multimap<Key, Value, mpc_hash<Key>, std::equal_to<Key>, mpc_allocator<std::pair<const Key, Value>>>;

using mpc_string             = std::basic_string<char, std::char_traits<char>, mpc_allocator<char>>;
using mpc_wstring            = std::basic_string<wchar_t, std::char_traits<wchar_t>, mpc_allocator<wchar_t>>;

template <class Key>
class mpc_hash : public std::hash<Key> {
};

template <>
class mpc_hash<mpc_string> {
 public:
    size_t operator()(const mpc_string &str) const {
        return std::hash<std::string_view> {}({str.data(), str.size()});
    }
};

template <>
class mpc_hash<mpc_wstring> {
 public:
    size_t operator()(const mpc_wstring &str) const {
        return std::hash<std::wstring_view> {}({str.data(), str.size()});
    }
};

#endif /* MEMORY_POOL_MEMORY_POOL_ALLOCATOR_HPP_ */
