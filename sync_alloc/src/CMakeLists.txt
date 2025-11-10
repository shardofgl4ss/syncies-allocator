add_subdirectory(builtin)

add_library(sync_alloc SHARED
			include/defs.h
			include/structs.h
			include/internal_alloc.h
			include/helper_functions.h
			include/types.h
			include/handle.h
			include/alloc_init.h
			include/alloc_utils.h
			builtin/include/syn_memops.h
			builtin/syn_memops.c
			sync_alloc.c
			internal_alloc.c
			handle.c
			debug.c
			alloc_init.c
			alloc_utils.c
)
target_include_directories(sync_alloc PRIVATE include builtin/include)
#target_compile_options(sync_alloc PRIVATE ${DEBUG_COMPILE_FLAGS} -Wno-format-nonliteral -Wno-ignored-attributes)
#target_link_options(sync_alloc PUBLIC -fsanitize=address -fsanitize=undefined)
