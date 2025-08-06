# Additional Enhancements

1. Consider making `nextPowerOf2` static

2. Choose which of the following useful methods to implement
```cpp
void reserve(size_t newCapacity);  // Pre-allocate capacity
void shrink_to_fit();              // Reduce capacity to length
void clear();                      // Remove all elements
size_t getCapacity() const { return capacity; }
```

3. Choose between two possible implementations for the move constructor and move assignment operator
Minimal guarantees (more common):
```cpp
// Just ensure it won't crash when destroyed
other.arr = nullptr;
other.length = 0;
other.capacity = 0; // Or whatever prevents crashes
```

Make moved-from objects equivalent to default-constructed objects (my current approach):
```cpp
// Reset to empty but fully functional state
other.arr = new T[1];
other.length = 0;
other.capacity = 1;
```