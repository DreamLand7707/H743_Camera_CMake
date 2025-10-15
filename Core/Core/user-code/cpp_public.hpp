/*
 * cpppublic.hpp
 *
 *  Created on: Dec 17, 2024
 *      Author: DrL潇湘
 */

#ifndef INC_CPP_PUBLIC_HPP_
#define INC_CPP_PUBLIC_HPP_

#ifdef __cplusplus

    #include <vector>
    #include <deque>
    #include <string>
    #include <set>
    #include <map>
    #include <unordered_map>
    #include <unordered_set>
    #include <list>
    #include <forward_list>

    #include <functional>

    #include "memory_pool_allocator.hpp"

template <class T>
class freertos_allocator {
 public:
    using pointer            = T *;
    using const_pointer      = const T *;
    using void_pointer       = void *;
    using const_void_pointer = const void *;
    using value_type         = T;
    using size_type          = size_t;
    using difference_type    = std::ptrdiff_t;

    freertos_allocator()     = default;
    ~freertos_allocator()    = default;
    template <class U>
    explicit freertos_allocator(const freertos_allocator<U> &other) {}

    [[nodiscard]] pointer allocate(size_type n) const {
        return static_cast<pointer>(pvPortMalloc((size_type)n * sizeof(T)));
    }

    void deallocate(pointer p, size_type n) const {
        vPortFree((void *)p);
    }
};

class freertos_deleter {
 public:
    void operator()(void *ptr) {
        vPortFree(ptr);
    }
};

template <class Key>
class os_hash;

using os_string  = std::basic_string<char, std::char_traits<char>, freertos_allocator<char>>;
using os_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, freertos_allocator<wchar_t>>;

template <class T>
using os_vector = std::vector<T, freertos_allocator<T>>;

template <class T>
using os_deque = std::deque<T, freertos_allocator<T>>;

template <class T>
using os_list = std::list<T, freertos_allocator<T>>;

template <class T>
using os_forward_list = std::forward_list<T, freertos_allocator<T>>;

template <class T>
using os_set = std::set<T, std::less<T>, freertos_allocator<T>>;

template <class T>
using os_multiset = std::multiset<T, std::less<T>, freertos_allocator<T>>;

template <class Key, class Value>
using os_map = std::map<Key, Value, std::less<Key>, freertos_allocator<std::pair<const Key, Value>>>;

template <class Key, class Value>
using os_multimap = std::multimap<Key, Value, std::less<Key>, freertos_allocator<std::pair<const Key, Value>>>;

template <class T>
using os_unordered_set = std::unordered_set<T, os_hash<T>, std::equal_to<T>, freertos_allocator<T>>;

template <class T>
using os_unordered_multiset = std::unordered_multiset<T, os_hash<T>, std::equal_to<T>, freertos_allocator<T>>;

template <class Key, class Value>
using os_unordered_map = std::unordered_map<Key, Value, os_hash<Key>, std::equal_to<Key>, freertos_allocator<std::pair<const Key, Value>>>;

template <class Key, class Value>
using os_unordered_multimap = std::unordered_multimap<Key, Value, os_hash<Key>, std::equal_to<Key>, freertos_allocator<std::pair<const Key, Value>>>;

template <class Key>
class os_hash : public std::hash<Key> {
};

template <>
class os_hash<os_string> {
 public:
    size_t operator()(const os_string &str) const {
        return std::hash<std::string_view> {}({str.data(), str.size()});
    }
};

template <>
class os_hash<os_wstring> {
 public:
    size_t operator()(const os_wstring &str) const {
        return std::hash<std::wstring_view> {}({str.data(), str.size()});
    }
};

namespace litesys_transparent_map_details
{
    struct transparent_map_hash : std::hash<std::string_view>, std::hash<std::string> {
        using is_transparent = void;
        using std::hash<std::string>::operator();
        using std::hash<std::string_view>::operator();
    };

    struct transparent_map_equal_to {
        using is_transparent = void;

        template <class L, class R>
        bool operator()(const L &lhs, const R &rhs) const {
            return (std::string_view(lhs) == std::string_view(rhs));
        }
    };
} // namespace litesys_transparent_map_details


template <typename T, std::size_t Alignment = 32>
class aligned_allocator {
 public:
    using value_type                       = T;
    using size_type                        = std::size_t;
    using difference_type                  = std::ptrdiff_t;

    static constexpr std::size_t alignment = Alignment;

    aligned_allocator() noexcept           = default;

    template <typename U>
    explicit aligned_allocator(const aligned_allocator<U, Alignment> &) noexcept {}

    template <typename U>
    struct rebind {
        using other = aligned_allocator<U, Alignment>;
    };

    T *allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) {
            return nullptr;
        }

        std::size_t size = n * sizeof(T);
        void       *ptr  = ::operator new(size, std::align_val_t {Alignment});
        return static_cast<T *>(ptr);
    }

    void deallocate(T *ptr, std::size_t n) noexcept {
        ::operator delete(ptr, n * sizeof(T), std::align_val_t {Alignment});
    }
};

template <typename T, std::size_t AlignmentT, typename U, std::size_t AlignmentU>
bool operator==(const aligned_allocator<T, AlignmentT> &,
                const aligned_allocator<U, AlignmentU> &) noexcept {
    return AlignmentT == AlignmentU;
}

template <typename T, std::size_t AlignmentT, typename U, std::size_t AlignmentU>
bool operator!=(const aligned_allocator<T, AlignmentT> &,
                const aligned_allocator<U, AlignmentU> &) noexcept {
    return AlignmentT != AlignmentU;
}


#endif

#endif /* INC_CPP_PUBLIC_HPP_ */
