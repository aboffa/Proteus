cmake_minimum_required(VERSION 3.13)
project(my_Proteus)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -pedantic")

include_directories(../include)

add_executable(int_workload_gen int_workload_gen.cpp)
#add_executable(str_workload_gen str_workload_gen.cpp)
