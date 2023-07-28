cmake_minimum_required(VERSION 3.13)
project(my_Proteus)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -pedantic")

include_directories(../include)
add_library(my_utils SHARED ../include/louds_dense.hpp
        ../include/util.cpp
        ../include/config.cpp
        ../include/prefixbf.cpp
        ../include/clhash.cpp
        ../include/MurmurHash3.cpp
        ../include/modeling.cpp)

add_executable(my_Proteus bench.cpp)
target_link_libraries(my_Proteus my_utils)
#target_sources(my_Proteus PRIVATE my_utils)