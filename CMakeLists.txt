cmake_minimum_required(VERSION 3.14)
project(XVector VERSION 1.0.0)

add_library(XVector INTERFACE)
target_include_directories(XVector INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# installation rules
install(DIRECTORY include/ DESTINATION include)