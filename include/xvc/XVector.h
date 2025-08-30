#ifndef X_VECTOR_H
#define X_VECTOR_H

#ifdef DEBUG
    #include <iostream>
    #include <cassert>
    #define XVECTOR_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                std::cerr << "Assertion failed: " << (message) \
                          << " at " << __FILE__ << ":" << __LINE__ \
                          << " in " << __FUNCTION__ << std::endl; \
                std::abort(); \
            } \
        } while(0)
    #define XVECTOR_BOUNDS_CHECK(idx) \
        XVECTOR_ASSERT(idx < size_, "Index out of bounds")
    #define XVECTOR_EMPTY_CHECK() \
        XVECTOR_ASSERT(size_ > 0, "Operation on empty array")
#else
    #define XVECTOR_ASSERT(condition, message) ((void)0)
    #define XVECTOR_BOUNDS_CHECK(idx) ((void)0)
    #define XVECTOR_EMPTY_CHECK() ((void)0)
#endif

#include <stddef.h>    // size_t
#include <initializer_list> // std::initializer_list
#include <stdexcept>   // std::out_of_range
#include <utility>     // std::exchange, std::forward, std::swap
#include <type_traits> // std::is_nothrow_move_constructible_v, std::is_nothrow_copy_constructible_v, std::is_trivially_destructible_v
#include <new>         // ::operator new, ::operator delete
#include <iterator>    // std::reverse_iterator
#include <cstring>     // std::memcpy, std::memmove
#include <algorithm>   // std::min

namespace xvc {

template<typename T>
class XVector
{
private:
    // member variables
    size_t size_;
    size_t capacity_;
    T* data_;

    // private methods
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

    // constructors and destructor
    XVector();
    explicit XVector(size_t, const T& = T{});
    template<size_t N>
    XVector(T (&a)[N]);
    XVector(const XVector<T>&);
    XVector(XVector<T>&&) noexcept;
    XVector(std::initializer_list<T> init);
    ~XVector();

    // element access
    XVector<T>& operator=(const XVector<T>&);
    XVector<T>& operator=(XVector<T>&&) noexcept;
    T& operator[](size_t) noexcept;
    const T& operator[](size_t) const noexcept;
    [[nodiscard]] T& at(size_t n);
    [[nodiscard]] const T& at(size_t n) const;
    T& front() noexcept;
    const T& front() const noexcept;
    T& back() noexcept;
    const T& back() const noexcept;
    [[nodiscard]] T* data() noexcept;
    [[nodiscard]] const T* data() const noexcept;

    // iterators
    T* begin() noexcept;
    const T* begin() const noexcept;
    T* end() noexcept;
    const T* end() const noexcept;
    std::reverse_iterator<T*> rbegin() noexcept;
    std::reverse_iterator<const T*> rbegin() const noexcept;
    std::reverse_iterator<T*> rend() noexcept;
    std::reverse_iterator<const T*> rend() const noexcept;

    // capacity
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    
    // modifiers
    void append(const T&);
    void append(T&&);
    void pop_back();
    void concatenate(const XVector<T>&);
    void clear() noexcept;
    void swap(XVector<T>& other) noexcept;
    void reserve(size_t);
    void resize(size_t, const T& = T{});
    void shrink_to_fit();
    template<typename... Args>
    void emplace_back(Args&&...);
};

template<typename T>
size_t XVector<T>::nextPowerOf2(size_t n)
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
void XVector<T>::fillTrivial(T* dest, size_t count, const T& value) {
    if (count == 0) return;

    std::memcpy(dest, &value, sizeof(T));
    size_t filled = 1;
    while (filled < count)
    {
        const size_t chunk = std::min(filled, count - filled);
        std::memcpy(dest + filled, dest, chunk * sizeof(T));
        filled += chunk;
    }
}


template<typename T>
void XVector<T>::reallocate(bool doubleCap)
{
    const size_t newCap = doubleCap ? (capacity_ ? capacity_ * 2 : 1) : nextPowerOf2(size_); // for append could just be capacity_ *= 2, for concatenate need more
    T* newData = static_cast<T*>(::operator new(newCap * sizeof(T)));

    // compile-time check to avoid try-catch block if T has a noexcept move constructor
    if constexpr (std::is_trivially_copyable_v<T>) 
    {
        std::memcpy(newData, data_, size_ * sizeof(T));
    }
    else if constexpr (std::is_nothrow_move_constructible_v<T>)
    {
        for (size_t i = 0; i < size_; ++i)
            new (&newData[i]) T(std::move(data_[i]));
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < size_; ++i)
                new (&newData[i]) T(std::move(data_[i]));
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    newData[j].~T();
            }

            ::operator delete(newData);
            throw;
        }
    }
    
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < size_; i++)
            data_[i].~T();
    }
    
    ::operator delete(data_);

    data_ = newData;
    capacity_ = newCap;
}

template<typename T>
XVector<T>::XVector() 
    : size_(0), capacity_(1), data_(static_cast<T*>(::operator new(sizeof(T)))) {}

template<typename T>
XVector<T>::XVector(size_t count, const T& value)
    : size_(count), capacity_(nextPowerOf2(count)), data_(static_cast<T*>(::operator new(capacity_ * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        fillTrivial(data_, size_, value);
    }
    else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile-time check if T has a noexcept copy constructor
    {
        for (size_t i = 0; i < size_; ++i)
            new (&data_[i]) T(value);
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < size_; ++i)
                new (&data_[i]) T(value);
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    data_[j].~T();
            }

            ::operator delete(data_);
            throw;
        }
    }
}

template<typename T>
template<size_t N>
XVector<T>::XVector(T (&a)[N])
    : size_(N), capacity_(nextPowerOf2(N)), data_(static_cast<T*>(::operator new(capacity_ * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        std::memcpy(data_, a, size_ * sizeof(T));
    }
    else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile time check if T has a noexcept copy constructor
    {
        for (size_t i = 0; i < size_; ++i)
            new (&data_[i]) T(a[i]);
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < size_; ++i)
                new (&data_[i]) T(a[i]);
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    data_[j].~T();
            }

            ::operator delete(data_);
            throw;
        }
    }
}

template<typename T>
XVector<T>::XVector(const XVector<T>& other)
    : size_(other.size_), capacity_(other.capacity_), data_(static_cast<T*>(::operator new(other.capacity_ * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        std::memcpy(data_, other.data_, size_ * sizeof(T));
    }
    else if constexpr (std::is_nothrow_copy_constructible_v<T>)
    {
        for (size_t i = 0; i < size_; ++i)
            new (&data_[i]) T(other.data_[i]);
    }
    else
    {
        size_t i = 0;
        try
        {
            for (; i < size_; ++i)
                new (&data_[i]) T(other.data_[i]);
        }
        catch (...)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t j = 0; j < i; ++j)
                    data_[j].~T();
            }

            ::operator delete(data_);
            throw;
        }
    }
}

template<typename T>
XVector<T>::XVector(XVector<T>&& other) noexcept
    : size_(other.size_), capacity_(other.capacity_), data_(std::exchange(other.data_, nullptr)) 
{
    other.size_ = 0;
    other.capacity_ = 0;
}

template<typename T>
XVector<T>::XVector(std::initializer_list<T> init)
    : size_(init.size()), capacity_(nextPowerOf2(init.size())), 
      data_(static_cast<T*>(::operator new(capacity_ * sizeof(T))))
{
    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(data_, init.begin(), size_ * sizeof(T));
    } else {
        size_t i = 0;
        try {
            for (const auto& val : init) {
                new (&data_[i++]) T(val);
            }
        } catch (...) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_t j = 0; j < i; ++j)
                    data_[j].~T();
            }
            ::operator delete(data_);
            throw;
        }
    }
}

template<typename T>
XVector<T>::~XVector()
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < size_; i++)
            data_[i].~T();
    }
    
    ::operator delete(data_);
}

template<typename T>
XVector<T>& XVector<T>::operator=(const XVector<T>& other)
{
    if (this != &other)
    {
        if (other.size_ <= capacity_)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(data_, other.data_, other.size_ * sizeof(T));
            }
            else
            {
                size_t i = 0;
                for (; i < std::min(size_, other.size_); ++i)
                    data_[i] = other.data_[i];
                
                if constexpr (std::is_nothrow_copy_constructible_v<T>)
                {
                    for (; i < other.size_; ++i)
                        new (&data_[i]) T(other.data_[i]);
                }
                else
                {
                    try
                    {
                        for (; i < other.size_; ++i)
                            new (&data_[i]) T(other.data_[i]);
                    }
                    catch (...)
                    {
                        if constexpr (!std::is_trivially_destructible_v<T>)
                        {
                            for (size_t i = 0; i < size_; ++i)
                                data_[i].~T();
                        }

                        ::operator delete(data_);
                        throw;
                    }
                }
            }

            size_ = other.size_;
        }
        else 
        {
            T* newData = static_cast<T*>(::operator new(other.capacity_ * sizeof(T))); // for exception safety
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(newData, other.data_, other.size_ * sizeof(T));
            }
            else if constexpr (std::is_nothrow_copy_constructible_v<T>)
            {
                for (size_t i = 0; i < other.size_; ++i)
                    new (&newData[i]) T(other.data_[i]);
            }
            else
            {
                size_t i = 0;
                try
                {
                    for (; i < other.size_; ++i)
                        new (&newData[i]) T(other.data_[i]);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newData[j].~T();
                    }

                    ::operator delete(newData);
                    throw;
                }
            }
            
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t i = 0; i < size_; i++)
                    data_[i].~T();
            }

            ::operator delete(data_);
            data_ = newData;
            size_ = other.size_;
            capacity_ = other.capacity_;
        }
    }
    return *this;
}

template<typename T>
XVector<T>& XVector<T>::operator=(XVector<T>&& other) noexcept
{
    if (this != &other)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < size_; i++)
                data_[i].~T();
        }

        ::operator delete(data_);

        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        data_ = std::exchange(other.data_, nullptr);
    }
    return *this;
}

template<typename T>
T& XVector<T>::operator[](size_t idx) noexcept {
    XVECTOR_BOUNDS_CHECK(idx);
    return data_[idx]; // no bounds checking for performance
}

template<typename T>
const T& XVector<T>::operator[](size_t idx) const noexcept {
    XVECTOR_BOUNDS_CHECK(idx);
    return data_[idx]; 
}

template<typename T>
T* XVector<T>::begin() noexcept { return data_; }

template<typename T>
const T* XVector<T>::begin() const noexcept { return data_; }

template<typename T>
T* XVector<T>::end() noexcept { return (data_ ? data_ + size_ : nullptr); }

template<typename T>
const T* XVector<T>::end() const noexcept { return (data_ ? data_ + size_ : nullptr); }

template<typename T>
std::reverse_iterator<T*> XVector<T>::rbegin() noexcept { return std::reverse_iterator<T*>(end()); }

template<typename T>
std::reverse_iterator<const T*> XVector<T>::rbegin() const noexcept { return std::reverse_iterator<const T*>(end()); }

template<typename T>
std::reverse_iterator<T*> XVector<T>::rend() noexcept { return std::reverse_iterator<T*>(begin()); }

template<typename T>
std::reverse_iterator<const T*> XVector<T>::rend() const noexcept { return std::reverse_iterator<const T*>(begin()); }

template<typename T>
size_t XVector<T>::size() const noexcept { return size_; }

template<typename T>
size_t XVector<T>::capacity() const noexcept { return capacity_; }

template<typename T>
T* XVector<T>::data() noexcept { return data_; }

template<typename T>
const T* XVector<T>::data() const noexcept { return data_; }

template<typename T>
void XVector<T>::append(const T& item)
{
    if (size_ >= capacity_) reallocate(true);
    new (&data_[size_]) T(item);
    ++size_;
}

template<typename T>
void XVector<T>::append(T&& item)
{
    if (size_ >= capacity_) reallocate(true);
    new (&data_[size_]) T(std::move(item));
    ++size_;
}

template<typename T>
void XVector<T>::pop_back()
{
    XVECTOR_EMPTY_CHECK();
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        data_[--size_].~T(); // no check for performance
    }
    else
    {
        --size_;
    }
}

template<typename T>
void XVector<T>::concatenate(const XVector<T>& other)
{
    if (other.empty()) return;

    const size_t newSize = size_ + other.size_;
    if (newSize > capacity_)
    {
        const size_t newCap = nextPowerOf2(newSize);
        T* newData = static_cast<T*>(::operator new(newCap * sizeof(T)));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(newData, data_, size_ * sizeof(T));
            std::memcpy(newData + size_, other.data_, other.size_ * sizeof(T));
        }
        else 
        {
            if constexpr (std::is_nothrow_move_constructible_v<T>)
            {
                for (size_t i = 0; i < size_; ++i)
                    new (&newData[i]) T(std::move(data_[i]));
            }
            else
            {
                size_t i = 0;
                try
                {
                    for (; i < size_; ++i)
                        new (&newData[i]) T(std::move(data_[i]));
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newData[j].~T();
                    }

                    ::operator delete(newData);
                    throw;
                }
            }
            
            if constexpr (std::is_nothrow_copy_constructible_v<T>)
            {
                for (size_t i = size_; i < newSize; ++i)
                    new (&newData[i]) T(other.data_[i-size_]);
            }
            else
            {
                size_t i = size_;
                try
                {
                    for (; i < newSize; ++i)
                        new (&newData[i]) T(other.data_[i-size_]);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newData[j].~T();
                    }

                    ::operator delete(newData);
                    throw;
                }
            }
        }
        
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < size_; i++)
                data_[i].~T();
        }
        
        ::operator delete(data_);

        data_ = newData;
        capacity_ = newCap;
        size_ = newSize;
    }
    else
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memmove(data_ + size_, other.data_, other.size_ * sizeof(T));
        }
        else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile-time check if T has a noexcept copy constructor
        {
            for (size_t i = size_; i < newSize; ++i)
                new (&data_[i]) T(other.data_[i-size_]);
        }
        else
        {
            size_t i = size_;
            try
            {
                for (; i < newSize; ++i)
                    new (&data_[i]) T(other.data_[i-size_]);
            }
            catch (...)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (size_t j = size_; j < i; ++j)
                        data_[j].~T();
                }

                throw;
            }
        }
        size_ = newSize;
    }
}

template<typename T>
const T& XVector<T>::at(size_t idx) const
{
    if (idx >= size_) throw std::out_of_range("XVector index out of bounds.");
    
    return data_[idx];
}

template<typename T>
T& XVector<T>::at(size_t idx)
{
    if (idx >= size_) throw std::out_of_range("XVector index out of bounds.");
    
    return data_[idx];
}

template<typename T>
T& XVector<T>::front() noexcept {
    XVECTOR_EMPTY_CHECK();
    return data_[0];
}

template<typename T>
const T& XVector<T>::front() const noexcept {
    XVECTOR_EMPTY_CHECK();
    return data_[0];
}

template<typename T>
T& XVector<T>::back() noexcept {
    XVECTOR_EMPTY_CHECK();
    return data_[size_ - 1];
}

template<typename T>
const T& XVector<T>::back() const noexcept {
    XVECTOR_EMPTY_CHECK();
    return data_[size_ - 1];
}

template<typename T>
bool XVector<T>::empty() const noexcept
{
    return (size_ == 0);
}

template<typename T>
void XVector<T>::clear() noexcept 
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < size_; i++)
            data_[i].~T();
    }
    size_ = 0;
}

template<typename T>
void XVector<T>::swap(XVector<T>& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

template<typename T>
void XVector<T>::reserve(size_t space)
{
    if (space > capacity_)
    {
        const size_t newCap = nextPowerOf2(space);
        T* newData = static_cast<T*>(::operator new(newCap * sizeof(T)));
        
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(newData, data_, size_ * sizeof(T));
        }
        else if constexpr (std::is_nothrow_move_constructible_v<T>)
        {
            for (size_t i = 0; i < size_; ++i)
                new (&newData[i]) T(std::move(data_[i]));
        }
        else
        {
            size_t i = 0;
            try
            {
                for (; i < size_; ++i)
                    new (&newData[i]) T(std::move(data_[i]));
            }
            catch (...)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (size_t j = 0; j < i; ++j)
                        newData[j].~T();
                }

                ::operator delete(newData);
                throw;
            }
        }
        
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < size_; i++)
                data_[i].~T();
        }

        ::operator delete(data_);
        
        data_ = newData;
        capacity_ = newCap;
    }
}

template<typename T>
void XVector<T>::resize(size_t newSize, const T& value)
{
    if (newSize > size_)
    {
        if (newSize > capacity_)
        {
            const size_t newCap = nextPowerOf2(newSize);
            T* newData = static_cast<T*>(::operator new(newCap * sizeof(T)));

            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(newData, data_, size_ * sizeof(T)); // copy existing elements
                fillTrivial(newData + size_, newSize - size_, value);
            }
            else if constexpr (std::is_nothrow_move_constructible_v<T>)
            {
                for (size_t i = 0; i < size_; ++i)
                    new (&newData[i]) T(std::move(data_[i]));
            }
            else
            {
                size_t i = 0;
                try
                {
                    for (; i < size_; ++i)
                        new (&newData[i]) T(std::move(data_[i]));
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newData[j].~T();
                    }

                    ::operator delete(newData);
                    throw;
                }
            }

            if constexpr (std::is_nothrow_copy_constructible_v<T>)
            {
                for (size_t i = size_; i < newSize; ++i)
                    new (&newData[i]) T(value);
            }
            else
            {
                size_t i = size_;
                try
                {
                    for (; i < newSize; ++i)
                        new (&newData[i]) T(value);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = 0; j < i; ++j)
                            newData[j].~T();
                    }

                    ::operator delete(newData);
                    throw;
                }
            }
            
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t i = 0; i < size_; i++)
                    data_[i].~T();
            }
            
            ::operator delete(data_);

            data_ = newData;
            capacity_ = newCap;
        }
        else
        {
            if constexpr (std::is_trivially_copyable_v<T>) {
                const size_t grow = newSize - size_;
                if (grow > 0) {
                    std::memcpy(data_ + size_, &value, sizeof(T));
                    size_t filled = 1;
                    while (filled < grow) {
                        const size_t chunk = std::min(filled, grow - filled);
                        std::memcpy(data_ + size_ + filled, data_ + size_, chunk * sizeof(T));
                        filled += chunk;
                    }
                }
            } 
            else if constexpr (std::is_nothrow_copy_constructible_v<T>) // compile-time check if T has a noexcept copy constructor
            {
                for (size_t i = size_; i < newSize; ++i)
                    new (&data_[i]) T(value);
            }
            else
            {
                size_t i = size_;
                try
                {
                    for (; i < newSize; ++i)
                        new (&data_[i]) T(value);
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_t j = size_; j < i; ++j)
                            data_[j].~T();
                    }

                    throw;
                }
            }
        }
    }
    else if (newSize < size_)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = newSize; i < size_; i++)
                data_[i].~T();
        }
    }
    size_ = newSize;
}

template <typename T>
void XVector<T>::shrink_to_fit()
{
    if (nextPowerOf2(size_) < capacity_) reallocate(); // shrink if we can reduce capacity
}

template <typename T>
template <typename... Args>
void XVector<T>::emplace_back(Args&&... args)
{
    if (size_ >= capacity_) reallocate(true);
    new (&data_[size_]) T(std::forward<Args>(args)...);
    ++size_;
}

} // namespace xvc

#endif // X_VECTOR_H