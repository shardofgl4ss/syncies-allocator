cmake_minimum_required(VERSION 3.27)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, defaulting to Debug")
	set(CMAKE_BUILD_TYPE "Debug")
endif()

set(DEBUG_FLAGS
	-Wno-ignored-attributes
	-Wall
	-Wextra
	-Wundef
	-Wstrict-overflow=5
	-Wshadow
	-Werror
	-fno-common
	-fno-omit-frame-pointer
	-ftrapv
	-fstack-protector-all
	-fno-strict-aliasing
	-Wno-sign-compare
	-g3
	-Og
)
set(RELEASE_FLAGS
	-O3
	-Wl,-s
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_STANDARD 23)

project(arena_allocator C)

include_directories(include)
include_directories(src/include)


add_subdirectory(sync_alloc)
add_subdirectory(alloc_tester)

target_compile_options(sync_alloc PRIVATE 
	"$<$<CONFIG:Debug>:${DEBUG_FLAGS}>"
	"$<$<CONFIG:Release>:${RELEASE_FLAGS}>"
)
