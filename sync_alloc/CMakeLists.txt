include_directories(include)

add_library(sync_alloc STATIC
			include/defs.h
			include/structs.h
			include/alloc_lib.h
			include/internal_alloc.h
			include/helper_functions.h
			alloc_lib.c
			internal_alloc.c
)
