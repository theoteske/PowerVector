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
    cap = doubleCap ? cap * 2 : nextPowerOf2(len); // for append could just be cap *= 2, for concatenate need more
    T* nA = new T[cap];
    std::copy_n(arr, len, nA);
    delete[] arr;
    arr = nA;
}

template<typename T>
dynamicArray<T>::dynamicArray() 
    : len(0), cap(1), arr(new T[1]) {}

template<typename T>
dynamicArray<T>::dynamicArray(size_t count, const T& value)
    : len(count), cap(nextPowerOf2(count)), arr(new T[cap])
{
    std::fill_n(arr, count, value);
}

template<typename T>
template<size_t N>
dynamicArray<T>::dynamicArray(T (&a)[N])
    : len(N), cap(nextPowerOf2(N)), arr(new T[cap])
{
    std::copy_n(a, N, arr);
}

template<typename T>
dynamicArray<T>::~dynamicArray()
{
    delete[] arr; // deallocate
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
    if (len > 0) --len; // no bounds checking for performance
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
void dynamicArray<T>::clear() noexcept { len = 0; }

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

#endif