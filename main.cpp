#include "dynamicArray.h"

#include <iostream>

int main()
{
    dynamicArray<int> arr;
    arr.append(3);
    arr.append(2);
    for (int i = 0; i < arr.size(); i++)
        std::cout << arr.at(i) << " ";
    return 0;
}