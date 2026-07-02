cmake_minimum_required(VERSION 4.0)
project(ProPro)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

add_executable(ProPro main.cpp)
target_link_libraries(ProPro PRIVATE Threads::Threads)
