add_executable(alloc_tester main.c)
target_link_libraries(alloc_tester sync_alloc)
target_include_directories(alloc_tester PUBLIC
						   ../sync_alloc/include
						   ../sync_alloc/src/include
						   ../sync_alloc/src/builtin/include
)

target_compile_options(alloc_tester PUBLIC ${DEBUG_COMPILE_FLAGS})
