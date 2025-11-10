add_subdirectory(src)
target_include_directories(sync_alloc PRIVATE include)

target_sources(sync_alloc INTERFACE
			   include/sync_alloc.h
			   PRIVATE
			   include/debug.h
			   src/builtin/include/syn_memops.h
)
