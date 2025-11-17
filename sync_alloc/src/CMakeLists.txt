target_include_directories(sync_alloc PUBLIC include)
target_include_directories(syn_memops_test PUBLIC include)

target_sources(sync_alloc
			   PRIVATE
			   internal_alloc.c
			   handle.c
			   debug.c
			   deadzone.c
			   alloc_init.c
			   alloc_utils.c
			   free_node.c
			   huge_page.c
			   slab.c
			   PRIVATE
			   FILE_SET private_headers
			   TYPE HEADERS
			   BASE_DIRS include
			   FILES
			   include/types.h
			   include/defs.h
			   include/structs.h
			   include/globals.h
			   include/handle.h
			   include/deadzone.h
			   include/alloc_init.h
			   include/internal_alloc.h
			   include/free_node.h
			   include/huge_page.h
			   include/slab.h
			   include/alloc_utils.h
			   include/debug.h
)
