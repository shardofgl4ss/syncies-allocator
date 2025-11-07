add_executable(alloc_tester main.c)
#target_link_libraries(alloc_tester sync_alloc)
target_link_libraries(alloc_tester syn_lib_test)
target_include_directories(alloc_tester PUBLIC
						   ../sync_alloc/include
						   ../sync_alloc/src/include
						   ../sync_alloc/src/builtin/include
)

#target_compile_options(alloc_tester PRIVATE ${DEBUG_COMPILE_FLAGS})
