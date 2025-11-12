include_directories(include)

target_sources(sync_alloc
			   PUBLIC
			   FILE_SET public_headers
			   TYPE HEADERS
			   BASE_DIRS "include"
			   FILES "include/sync_alloc.h"
)

add_subdirectory(src)
add_subdirectory(builtin)
