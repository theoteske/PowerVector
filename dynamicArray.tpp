#ifndef DYNAMIC_ARRAY_TPP
#define DYNAMIC_ARRAY_TPP

#include <stddef.h>    // size_t
#include <stdexcept>   // std::out_of_range
#include <utility>     // std::exchange, std::forward, std::swap
#include <type_traits> // std::is_nothrow_move_constructible_v, std::is_nothrow_copy_constructible_v, std::is_trivially_destructible_v
#include <new>         // ::operator new, ::operator delete
#include <iterator>    // std::reverse_iterator
#include <cstring>     // std::memcpy, std::memmove
#include <algorithm>   // std::min

template<typename T>
size_t dynamicArray<T>::nextPowerOf2(size_t n)
{
    if (n <= 1) return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if constexpr (sizeof(size_t) > 4) n |= n >> 32; // compile-time check to avoid undefined behavior on 32-bit systems
    return n+1;
}

template<typename T>
void dynamicArray<T>::reallocate(bool doubleCap)
{
    const size_t newCap = doubleCap ? (cap ? cap * 2 : 1) : nextPowerOf2(len); // for append could just be cap *= 2, for concatenate need more
    T* newArr = static_cast<T*>(::operator new(newCap * sizeof(T)));

    // compile-time check to avoid try-catch block if T has a noexcept move constructor
    if constexpr (std::is_trivially_copyable_v<T>) 
    {
        std::memcpy(newArr, arr, len * sizeof(T));
    }
    else if constexpr (std::is_nothrow_move_constructible_v<T>)
    {
        for (size_t i = 0; i < len; ++i)
            new (&newArr[i]) T(std::move(arr[i]));
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < len; ++i)
                new (&newArr[i]) T(std::move(arr[i]));
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    newArr[j].~T();
            }

            ::operator delete(newArr);
            throw;
        }
    }
    
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < len; i++)
            arr[i].~T();
    }
    
    ::operator delete(arr);

    arr = newArr;
    cap = newCap;
}

template<typename T>
dynamicArray<T>::dynamicArray() 
    : len(0), cap(1), arr(static_cast<T*>(::operator new(sizeof(T)))) {}

template<typename T>
dynamicArray<T>::dynamicArray(size_t count, const T& value)
    : len(count), cap(nextPowerOf2(count)), arr(static_cast<T*>(::operator new(cap * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        if (len > 0) {
            // write one, then replicate exponentially
            std::memcpy(arr, &value, sizeof(T));
            size_t filled = 1;
            while (filled < len) {
                const size_t chunk = std::min(filled, len - filled);
                std::memcpy(arr + filled, arr, chunk * sizeof(T));
                filled += chunk;
            }
        }
    }
    else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile-time check if T has a noexcept copy constructor
    {
        for (size_t i = 0; i < len; ++i)
            new (&arr[i]) T(value);
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < len; ++i)
                new (&arr[i]) T(value);
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    arr[j].~T();
            }

            ::operator delete(arr);
            throw;
        }
    }
}

template<typename T>
template<size_t N>
dynamicArray<T>::dynamicArray(T (&a)[N])
    : len(N), cap(nextPowerOf2(N)), arr(static_cast<T*>(::operator new(cap * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        std::memcpy(arr, a, len * sizeof(T));
    }
    else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile time check if T has a noexcept copy constructor
    {
        for (size_t i = 0; i < len; ++i)
            new (&arr[i]) T(a[i]);
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < len; ++i)
                new (&arr[i]) T(a[i]);
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    arr[j].~T();
            }

            ::operator delete(arr);
            throw;
        }
    }
}

template<typename T>
dynamicArray<T>::~dynamicArray()
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < len; i++)
            arr[i].~T();
    }
    
    ::operator delete(arr);
}

template<typename T>
dynamicArray<T>::dynamicArray(const dynamicArray<T>& other) // copy constructor
    : len(other.len), cap(other.cap), arr(static_cast<T*>(::operator new(other.cap * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        std::memcpy(arr, other.arr, len * sizeof(T));
    }
    else if constexpr (std::is_nothrow_copy_constructible_v<T>)
    {
        for (size_t i = 0; i < len; ++i)
            new (&arr[i]) T(other.arr[i]);
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < len; ++i)
                new (&arr[i]) T(other.arr[i]);
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    arr[j].~T();
            }

            ::operator delete(arr);
            throw;
        }
    }
}

template<typename T>
dynamicArray<T>::dynamicArray(dynamicArray<T>&& other) noexcept // move constructor
    : len(other.len), cap(other.cap), arr(std::exchange(other.arr, nullptr)) 
{
    other.len = 0;
    other.cap = 0;
}

template<typename T>
dynamicArray<T>& dynamicArray<T>::operator=(const dynamicArray<T>& other)
{
    if (this != &other)
    {
        if (other.len <= cap)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(arr, other.arr, other.len * sizeof(T));
            }
            else
            {
                size_t i = 0;
                for (; i < std::min(len, other.len); ++i)
                    arr[i] = other.arr[i];
                
                if constexpr (std::is_nothrow_copy_constructible_v<T>)
                {
                    for (; i < other.len; ++i)
                        new (&arr[i]) T(other.arr[i]);
                }
                else
                {
                    try
                    {
                        for (; i < other.len; ++i)
                            new (&arr[i]) T(other.arr[i]);
                    }
                    catch (...)
                    {
                        if constexpr (!std::is_trivially_destructible_v<T>)
                        {
                            for (size_t i = 0; i < len; ++i)
                                arr[i].~T();
                        }

                        ::operator delete(arr);
                        throw;
                    }
                }
            }

            len = other.len;
        }
        else 
        {
            T* newArr = static_cast<T*>(::operator new(other.cap * sizeof(T))); // for exception safety
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(newArr, other.arr, other.len * sizeof(T));
            }
            else if constexpr (std::is_nothrow_copy_constructible_v<T>)
            {
                for (size_t i = 0; i < other.len; ++i)
                    new (&newArr[i]) T(other.arr[i]);
            }
            else
            {
                size_t i = 0;
                try
                {
                    for (; i < other.len; ++i)
                        new (&newArr[i]) T(other.arr[i]);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newArr[j].~T();
                    }

                    ::operator delete(newArr);
                    throw;
                }
            }
            
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t i = 0; i < len; i++)
                    arr[i].~T();
            }

            ::operator delete(arr);
            arr = newArr;
            len = other.len;
            cap = other.cap;
        }
    }
    return *this;
}

template<typename T>
dynamicArray<T>& dynamicArray<T>::operator=(dynamicArray<T>&& other) noexcept // move assignment operator
{
    if (this != &other)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < len; i++)
                arr[i].~T();
        }

        ::operator delete(arr);

        len = std::exchange(other.len, 0);
        cap = std::exchange(other.cap, 0);
        arr = std::exchange(other.arr, nullptr);
    }
    return *this;
}

template<typename T>
T& dynamicArray<T>::operator[](size_t idx) { return arr[idx]; } // no bounds checking for performance

template<typename T>
const T& dynamicArray<T>::operator[](size_t idx) const { return arr[idx]; }

template<typename T>
T* dynamicArray<T>::begin() noexcept { return arr; }

template<typename T>
const T* dynamicArray<T>::begin() const noexcept { return arr; }

template<typename T>
T* dynamicArray<T>::end() noexcept { return (arr ? arr + len : nullptr); }

template<typename T>
const T* dynamicArray<T>::end() const noexcept { return (arr ? arr + len : nullptr); }

template<typename T>
std::reverse_iterator<T*> dynamicArray<T>::rbegin() noexcept { return std::reverse_iterator<T*>(end()); }

template<typename T>
std::reverse_iterator<const T*> dynamicArray<T>::rbegin() const noexcept { return std::reverse_iterator<const T*>(end()); }

template<typename T>
std::reverse_iterator<T*> dynamicArray<T>::rend() noexcept { return std::reverse_iterator<T*>(begin()); }

template<typename T>
std::reverse_iterator<const T*> dynamicArray<T>::rend() const noexcept { return std::reverse_iterator<const T*>(begin()); }

template<typename T>
size_t dynamicArray<T>::length() const noexcept { return len; }

template<typename T>
size_t dynamicArray<T>::capacity() const noexcept { return cap; }

template<typename T>
T* dynamicArray<T>::data() noexcept { return arr; }

template<typename T>
const T* dynamicArray<T>::data() const noexcept { return arr; }

template<typename T>
void dynamicArray<T>::append(const T& item)
{
    if (len >= cap) reallocate(true);
    new (&arr[len]) T(item);
    ++len;
}

template<typename T>
void dynamicArray<T>::append(T&& item)
{
    if (len >= cap) reallocate(true);
    new (&arr[len]) T(std::move(item));
    ++len;
}

template<typename T>
void dynamicArray<T>::pop_back()
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        if (len > 0) arr[--len].~T();
    }
    else
    {
        if (len > 0) --len;
    }
}

template<typename T>
void dynamicArray<T>::concatenate(const dynamicArray<T>& other)
{
    if (other.empty()) return;

    const size_t newLen = len + other.len;
    if (newLen > cap)
    {
        const size_t newCap = nextPowerOf2(newLen);
        T* newArr = static_cast<T*>(::operator new(newCap * sizeof(T)));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(newArr, arr, len * sizeof(T));
            std::memcpy(newArr + len, other.arr, other.len * sizeof(T));
        }
        else 
        {
            if constexpr (std::is_nothrow_move_constructible_v<T>)
            {
                for (size_t i = 0; i < len; ++i)
                    new (&newArr[i]) T(std::move(arr[i]));
            }
            else
            {
                size_t i = 0;
                try
                {
                    for (; i < len; ++i)
                        new (&newArr[i]) T(std::move(arr[i]));
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newArr[j].~T();
                    }

                    ::operator delete(newArr);
                    throw;
                }
            }
            
            if constexpr (std::is_nothrow_copy_constructible_v<T>)
            {
                for (size_t i = len; i < newLen; ++i)
                    new (&newArr[i]) T(other.arr[i-len]);
            }
            else
            {
                size_t i = len;
                try
                {
                    for (; i < newLen; ++i)
                        new (&newArr[i]) T(other.arr[i-len]);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newArr[j].~T();
                    }

                    ::operator delete(newArr);
                    throw;
                }
            }
        }
        
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < len; i++)
                arr[i].~T();
        }
        
        ::operator delete(arr);

        arr = newArr;
        cap = newCap;
        len = newLen;
    }
    else
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memmove(arr + len, other.arr, other.len * sizeof(T));
        }
        else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile-time check if T has a noexcept copy constructor
        {
            for (size_t i = len; i < newLen; ++i)
                new (&arr[i]) T(other.arr[i-len]);
        }
        else
        {
            size_t i = len;
            try
            {
                for (; i < newLen; ++i)
                    new (&arr[i]) T(other.arr[i-len]);
            }
            catch (...)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (size_t j = len; j < i; ++j)
                        arr[j].~T();
                }

                throw;
            }
        }
        len = newLen;
    }
}

template<typename T>
const T& dynamicArray<T>::at(size_t idx) const
{
    if (idx >= len) throw std::out_of_range("Array index out of bounds.");
    
    return arr[idx];
}

template<typename T>
T& dynamicArray<T>::at(size_t idx)
{
    if (idx >= len) throw std::out_of_range("Array index out of bounds.");
    
    return arr[idx];
}

template<typename T>
bool dynamicArray<T>::empty() const noexcept
{
    return (len == 0);
}

template<typename T>
void dynamicArray<T>::clear() noexcept 
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < len; i++)
            arr[i].~T();
    }
    len = 0;
}

template<typename T>
void dynamicArray<T>::swap(dynamicArray<T>& other) noexcept {
    std::swap(arr, other.arr);
    std::swap(len, other.len);
    std::swap(cap, other.cap);
}

template<typename T>
void dynamicArray<T>::reserve(size_t space)
{
    if (space > cap)
    {
        const size_t newCap = nextPowerOf2(space);
        T* newArr = static_cast<T*>(::operator new(newCap * sizeof(T)));
        
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(newArr, arr, len * sizeof(T));
        }
        else if constexpr (std::is_nothrow_move_constructible_v<T>)
        {
            for (size_t i = 0; i < len; ++i)
                new (&newArr[i]) T(std::move(arr[i]));
        }
        else
        {
            size_t i = 0;
            try
            {
                for (; i < len; ++i)
                    new (&newArr[i]) T(std::move(arr[i]));
            }
            catch (...)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (size_t j = 0; j < i; ++j)
                        newArr[j].~T();
                }

                ::operator delete(newArr);
                throw;
            }
        }
        
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < len; i++)
                arr[i].~T();
        }

        ::operator delete(arr);
        
        arr = newArr;
        cap = newCap;
    }
}

template<typename T>
void dynamicArray<T>::resize(size_t newLen, const T& value)
{
    if (newLen > len)
    {
        if (newLen > cap)
        {
            const size_t newCap = nextPowerOf2(newLen);
            T* newArr = static_cast<T*>(::operator new(newCap * sizeof(T)));

            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(newArr, arr, len * sizeof(T));
                const size_t grow = newLen - len;
                if (grow > 0) {
                    // write one, then replicate exponentially
                    std::memcpy(newArr + len, &value, sizeof(T));
                    size_t filled = 1;
                    while (filled < grow) {
                        const size_t chunk = std::min(filled, grow - filled);
                        std::memcpy(newArr + len + filled, newArr + len, chunk * sizeof(T));
                        filled += chunk;
                    }
                }
            }
            else if constexpr (std::is_nothrow_move_constructible_v<T>)
            {
                for (size_t i = 0; i < len; ++i)
                    new (&newArr[i]) T(std::move(arr[i]));
            }
            else
            {
                size_t i = 0;
                try
                {
                    for (; i < len; ++i)
                        new (&newArr[i]) T(std::move(arr[i]));
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newArr[j].~T();
                    }

                    ::operator delete(newArr);
                    throw;
                }
            }

            if constexpr (std::is_nothrow_copy_constructible_v<T>)
            {
                for (size_t i = len; i < newLen; ++i)
                    new (&newArr[i]) T(value);
            }
            else
            {
                size_t i = len;
                try
                {
                    for (; i < newLen; ++i)
                        new (&newArr[i]) T(value);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newArr[j].~T();
                    }

                    ::operator delete(newArr);
                    throw;
                }
            }
            
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t i = 0; i < len; i++)
                    arr[i].~T();
            }
            
            ::operator delete(arr);

            arr = newArr;
            cap = newCap;
        }
        else
        {
            if constexpr (std::is_trivially_copyable_v<T>) {
                const size_t grow = newLen - len;
                if (grow > 0) {
                    std::memcpy(arr + len, &value, sizeof(T));
                    size_t filled = 1;
                    while (filled < grow) {
                        const size_t chunk = std::min(filled, grow - filled);
                        std::memcpy(arr + len + filled, arr + len, chunk * sizeof(T));
                        filled += chunk;
                    }
                }
            } 
            else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile-time check if T has a noexcept copy constructor
            {
                for (size_t i = len; i < newLen; ++i)
                    new (&arr[i]) T(value);
            }
            else
            {
                size_t i = len;
                try
                {
                    for (; i < newLen; ++i)
                        new (&arr[i]) T(value);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = len; j < i; ++j)
                            arr[j].~T();
                    }

                    throw;
                }
            }
        }
    }
    else if (newLen < len)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = newLen; i < len; i++)
                arr[i].~T();
        }
    }
    len = newLen;
}

template <typename T>
void dynamicArray<T>::shrink_to_fit()
{
    if (nextPowerOf2(len) < cap) reallocate(); // shrink if we can reduce capacity
}

template <typename T>
template <typename... Args>
void dynamicArray<T>::emplace_back(Args&&... args)
{
    if (len >= cap) reallocate(true);
    new (&arr[len]) T(std::forward<Args>(args)...);
    ++len;
}

#endif