target_include_directories(sync_alloc PUBLIC include)
target_include_directories(alloc_tester PUBLIC include)

target_sources(sync_alloc
			   INTERFACE
			   FILE_SET interface_headers
			   TYPE HEADERS
			   BASE_DIRS "include"
			   FILES "include/sync_alloc.h"
)

add_subdirectory(src)
add_subdirectory(builtin)
