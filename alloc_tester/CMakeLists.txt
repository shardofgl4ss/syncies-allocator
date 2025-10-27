add_executable(alloc_tester main.c)
target_link_libraries(alloc_tester sync_alloc)
include_directories(../sync_alloc/include)
