#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h> // size_t

template<typename T>
class dynamicArray
{
    private:
        size_t len;
        size_t cap;
        T* arr;

        static size_t nextPowerOf2(size_t);
        void reallocate(bool = false);
    public:
        dynamicArray();
        explicit dynamicArray(size_t, const T& = T{});
        template<size_t N>
        dynamicArray(T (&a)[N]);
        ~dynamicArray();
        dynamicArray(const dynamicArray<T>&);
        dynamicArray(dynamicArray<T>&&) noexcept;
        dynamicArray<T>& operator=(const dynamicArray<T>&);
        dynamicArray<T>& operator=(dynamicArray<T>&&) noexcept;
        T& operator[](size_t);
        const T& operator[](size_t) const;
        T* begin() noexcept;
        const T* begin() const noexcept;
        T* end() noexcept;
        const T* end() const noexcept;
        size_t length() const noexcept;
        size_t capacity() const noexcept;
        T* data() noexcept;
        const T* data() const noexcept;
        void append(const T&);
        void append(T&&);
        void pop_back();
        void concatenate(const dynamicArray<T>&);
        T& at(size_t n);
        const T& at(size_t n) const;
        bool empty() const noexcept;
        void clear() noexcept;
        void reserve(size_t);
        void resize(size_t, const T& = T{});
};

#include "dynamicArray.tpp"

#endif