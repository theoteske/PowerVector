cmake_minimum_required(VERSION 3.14)
project(PowerVector VERSION 1.0.0)

add_library(PowerVector INTERFACE)
target_include_directories(PowerVector INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# installation rules
install(DIRECTORY include/ DESTINATION include)