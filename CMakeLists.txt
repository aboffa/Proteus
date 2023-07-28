cmake_minimum_required(VERSION 3.13)
project(my_Proteus)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)
add_subdirectory(bench)
add_subdirectory(workloads)
