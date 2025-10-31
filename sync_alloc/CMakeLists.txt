add_library(sync_alloc STATIC
			src/include/defs.h
			src/include/structs.h
			include/sync_alloc.h
			src/include/internal_alloc.h
			src/include/helper_functions.h
			src/sync_alloc.c
			src/internal_alloc.c
			src/include/types.h
			src/handle.c
			src/include/handle.h
			src/debug.c
			include/debug.h
			src/alloc_init.c
			src/include/alloc_init.h
			src/alloc_utils.c
			src/include/alloc_utils.h
)
target_include_directories(sync_alloc PUBLIC include PRIVATE src/include)
target_compile_options(sync_alloc PRIVATE ${DEBUG_COMPILE_FLAGS} -Wno-format-nonliteral -Wno-ignored-attributes)
target_link_options(sync_alloc PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak)
