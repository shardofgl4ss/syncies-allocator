target_include_directories(sync_alloc PRIVATE include)
target_include_directories(syn_memops_test PRIVATE include)

target_sources(syn_memops_test PUBLIC
			   syn_memops.c
			   PUBLIC
			   FILE_SET public_headers
			   TYPE HEADERS
			   BASE_DIRS "include"
			   FILES
			   "include/syn_memops.h"
)
target_sources(sync_alloc PRIVATE
			   syn_memops.c
			   PRIVATE
			   FILE_SET private_headers
			   TYPE HEADERS
			   BASE_DIRS "include"
			   FILES
			   "include/syn_memops.h"
)
