#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h> // size_t
#include <iterator> // std::reverse_iterator

template<typename T>
class dynamicArray
{
    private:
        size_t len;
        size_t cap;
        T* arr;

        static size_t nextPowerOf2(size_t);
        static void fillTrivial(T* dest, size_t count, const T& value);
        void reallocate(bool = false);
    public:
        // type aliases for STL compatibility
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        dynamicArray();
        explicit dynamicArray(size_t, const T& = T{});
        template<size_t N>
        dynamicArray(T (&a)[N]);
        ~dynamicArray();
        dynamicArray(const dynamicArray<T>&);
        dynamicArray(dynamicArray<T>&&) noexcept;
        dynamicArray<T>& operator=(const dynamicArray<T>&);
        dynamicArray<T>& operator=(dynamicArray<T>&&) noexcept;
        T& operator[](size_t) noexcept;
        const T& operator[](size_t) const noexcept;
        T* begin() noexcept;
        const T* begin() const noexcept;
        T* end() noexcept;
        const T* end() const noexcept;
        std::reverse_iterator<T*> rbegin() noexcept;
        std::reverse_iterator<const T*> rbegin() const noexcept;
        std::reverse_iterator<T*> rend() noexcept;
        std::reverse_iterator<const T*> rend() const noexcept;
        [[nodiscard]] size_t length() const noexcept;
        [[nodiscard]] size_t capacity() const noexcept;
        [[nodiscard]] T* data() noexcept;
        [[nodiscard]] const T* data() const noexcept;
        void append(const T&);
        void append(T&&);
        void pop_back();
        void concatenate(const dynamicArray<T>&);
        [[nodiscard]] T& at(size_t n);
        [[nodiscard]] const T& at(size_t n) const;
        T& front() noexcept;
        const T& front() const noexcept;
        T& back() noexcept;
        const T& back() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        void clear() noexcept;
        void swap(dynamicArray<T>& other) noexcept;
        void reserve(size_t);
        void resize(size_t, const T& = T{});
        void shrink_to_fit();
        template<typename... Args>
        void emplace_back(Args&&...);
};

#include "dynamicArray.tpp"

#endif