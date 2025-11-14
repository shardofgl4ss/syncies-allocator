target_include_directories(sync_alloc PUBLIC include)
target_include_directories(tester PUBLIC include)

target_sources(sync_alloc
			   PUBLIC
			   sync_alloc.c
			   PUBLIC
			   FILE_SET public_headers
			   TYPE HEADERS
			   BASE_DIRS include
			   FILES include/sync_alloc.h
)

add_subdirectory(src)
add_subdirectory(builtin)
