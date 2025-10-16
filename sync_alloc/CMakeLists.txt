include_directories(include)

add_library(sync_alloc SHARED
			include/defs.h
			include/structs.h
			include/alloc_lib.h
			include/internal_alloc.h
			include/helper_functions.h
			src/alloc_lib.c
			src/internal_alloc.c
			include/types.h
			src/helper_functions.c
			src/handle.c
			include/handle.h
			src/debug.c
			include/debug.h
			src/alloc_init.c
			include/alloc_init.h
)
