cmake_minimum_required(VERSION 3.29)
project(arena_allocator C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wshadow -Wformat=2 -Wconversion -fno-common -Wundef -Wdouble-promotion -Werror")
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Wshadow -Wformat=2 -Wconversion -fno-common -Wundef -Wdouble-promotion -Werror")

add_subdirectory(src)
include_directories(include)