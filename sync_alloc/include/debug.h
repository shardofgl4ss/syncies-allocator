//
// Created by SyncShard on 10/16/25.
//

#ifndef ARENA_ALLOCATOR_DEBUG_H
#define ARENA_ALLOCATOR_DEBUG_H

#include <stdio.h>
#include "structs.h"

extern Debug_VTable logger;

/// @brief Logs a message to the console.
/// @param msg The message to print. Adds a newline automatically.
/// @param err If true, prints to stderr, else, prints to stdout.
/// @note The output condition depends on the logger's debug level.
/// @details
/// debug_level 0 == do not log
/// @details
/// debug_level 1 == only log errors
/// @details
/// debug_level 2 == log all
[[maybe_unused]]
inline static void
log_to_console(const char *msg, const bool err)
{
	if (SYNC_ALLOC_DEBUG_LVL == 0)
		return;
	if (SYNC_ALLOC_DEBUG_LVL == 1 && !err)
		return;
	if (err) {
		fprintf(stderr, "SYNC_ALLOC [ERR] >> %s\n", msg);
		fflush_unlocked(stderr);
		return;
	}
	fprintf(stdout, "SYNC_ALLOC [LOG] >> %s\n", msg);
	fflush_unlocked(stdout);
}

[[maybe_unused]]
static void
debug_print_memory_usage(const Arena *restrict arena);

#endif //ARENA_ALLOCATOR_DEBUG_H