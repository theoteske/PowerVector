#ifndef DYNAMIC_ARRAY_TPP
#define DYNAMIC_ARRAY_TPP

#include <stddef.h> // size_t
#include <stdexcept> // std::out_of_range
#include <utility> // std::exchange
#include <algorithm> // std::copy_n, std::fill_n

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
    size_t newCap = doubleCap ? cap * 2 : nextPowerOf2(len); // for append could just be cap *= 2, for concatenate need more
    T* newArr = static_cast<T*>(::operator new(newCap * sizeof(T)));

    // compile-time check to avoid try-catch block if T has a noexcept move constructor
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
            for (size_t j = 0; j < i; ++j)
                newArr[j].~T();
                
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
    if constexpr (std::is_nothrow_move_constructible_v<T>) // compile time check if T has a noexcept move constructor
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
            for (size_t j = 0; j < i; ++j)
                arr[j].~T();

            ::operator delete(newArr);
            throw;
        }
    }
}

template<typename T>
template<size_t N>
dynamicArray<T>::dynamicArray(T (&a)[N])
    : len(N), cap(nextPowerOf2(N)), arr(static_cast<T*>(::operator new(cap * sizeof(T))))
{
    if constexpr (std::is_nothrow_move_constructible_v<T>) // compile time check if T has a noexcept move constructor
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
            for (size_t j = 0; j < i; ++j)
                arr[j].~T();

            ::operator delete(newArr);
            throw;
        }
    }
}

template<typename T>
dynamicArray<T>::~dynamicArray()
{
    for (size_t i = 0; i < len; ++i)
        arr[i].~T();
    
    ::operator delete(arr);
}

template<typename T>
dynamicArray<T>::dynamicArray(const dynamicArray<T>& other) // copy constructor
    : len(other.len), cap(other.cap), arr(new T[cap])
{
    std::copy_n(other.arr, other.len, arr);
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
        T* newArr = new T[other.cap]; // for exception safety
        std::copy_n(other.arr, other.len, newArr);

        delete[] arr;
        arr = newArr;
        len = other.len;
        cap = other.cap;
    }
    return *this;
}

template<typename T>
dynamicArray<T>& dynamicArray<T>::operator=(dynamicArray<T>&& other) noexcept // move assignment operator
{
    if (this != &other)
    {
        delete[] arr;
        len = other.len;
        cap = other.cap;
        arr = other.arr;
        other.arr = nullptr;
        other.len = 0;
        other.cap = 0;
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
T* dynamicArray<T>::end() noexcept { return arr + len; }

template<typename T>
const T* dynamicArray<T>::end() const noexcept { return arr + len; }

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
    arr[len] = item;
    ++len;
}

template<typename T>
void dynamicArray<T>::append(T&& item)
{
    if (len >= cap) reallocate(true);
    arr[len] = std::move(item);
    ++len;
}

template<typename T>
void dynamicArray<T>::pop_back()
{
    if (len > 0) // no bounds checking for performance
        arr[--len].~T();
}

template<typename T>
void dynamicArray<T>::concatenate(const dynamicArray<T>& other)
{
    if (other.empty()) return;

    const size_t oldLen = len;
    len += other.len;
    if (len > cap) reallocate();
    std::copy_n(other.arr, other.len, arr + oldLen);
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
    for (size_t i = 0; i < len; i++)
        arr[i].~T();
    len = 0;
}

template<typename T>
void dynamicArray<T>::reserve(size_t newCap)
{
    if (newCap > cap)
    {
        cap = nextPowerOf2(newCap);
        T* nA = new T[cap];
        std::copy_n(arr, len, nA);
        delete[] arr;
        arr = nA;
    }
}

template<typename T>
void dynamicArray<T>::resize(size_t newLen, const T& value)
{
    if (newLen > cap) reserve(newLen);

    if (newLen > len) std::fill(arr + len, arr + newLen, value);
    len = newLen;
}

template <typename T>
void dynamicArray<T>::shrink_to_fit()
{
    size_t newCap = nextPowerOf2(len);
    if (newCap < cap) // shrink if we can reduce capacity
    {
        T* nA = new T[newCap];
        std::copy_n(arr, len, nA);
        delete[] arr;
        arr = nA;
        cap = newCap;
    }
}

template <typename T>
template <typename... Args>
void dynamicArray<T>::emplace_back(Args... args)
{
    if (len >= cap) reallocate(true);
    new (&arr[len]) T(std::forward<Args>(args)...);
    ++len;
}

#endif