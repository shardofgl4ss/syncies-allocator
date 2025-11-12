target_include_directories(sync_alloc PRIVATE include)
target_include_directories(syn_memops_test PRIVATE include)

target_sources(sync_alloc
			   INTERFACE
			   sync_alloc.c
			   PRIVATE
			   internal_alloc.c
			   handle.c
			   debug.c
			   alloc_init.c
			   alloc_utils.c
			   PRIVATE
			   FILE_SET private_headers
			   TYPE HEADERS
			   BASE_DIRS "include"
			   FILES
			   "include/types.h"
			   "include/defs.h"
			   "include/structs.h"
			   "include/handle.h"
			   "include/alloc_init.h"
			   "include/internal_alloc.h"
			   "include/alloc_utils.h"
			   "include/helper_functions.h"
			   "include/debug.h"
)
