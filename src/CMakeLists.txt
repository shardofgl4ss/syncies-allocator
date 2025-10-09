add_executable(arena_alloc main.c
			   ../include/structs.h
			   ../include/sync-alloc.h
			   ../include/helper_functions.h
			   ../include/defs.h
			   internal_alloc.c
			   ../include/internal_alloc.h
			   alloc_lib.c
			   ../include/alloc_lib.h)
target_link_libraries(arena_alloc m)