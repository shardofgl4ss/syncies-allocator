include_directories(include)
include_directories(src/include)

add_library(sync_alloc SHARED
			src/include/defs.h
			src/include/structs.h
			include/alloc_lib.h
			src/include/internal_alloc.h
			src/include/helper_functions.h
			src/alloc_lib.c
			src/internal_alloc.c
			src/include/types.h
			src/helper_functions.c
			src/handle.c
			src/include/handle.h
			src/debug.c
			include/debug.h
			src/alloc_init.c
			src/include/alloc_init.h
)
target_include_directories(sync_alloc PUBLIC include PRIVATE src/include)
target_compile_options(sync_alloc PRIVATE -Wno-format-nonliteral)