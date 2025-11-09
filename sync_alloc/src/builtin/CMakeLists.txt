add_library(syn_lib_test STATIC
			include/syn_memops.h
			syn_memops.c
)

target_include_directories(syn_lib_test PRIVATE
						   include
						   ../include
						   ../../include
)
#target_link_options(syn_lib_test PUBLIC -fsanitize=address -fsanitize=undefined)
target_compile_options(syn_lib_test PUBLIC ${DEBUG_COMPILE_FLAGS} -Wno-format-nonliteral -Wno-ignored-attributes -O3 -march=native)
