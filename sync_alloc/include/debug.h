//
// Created by SyncShard on 10/16/25.
//
// ReSharper disable CppParameterMayBeConst
// ide codechecker is a pos sometimes

#ifndef ARENA_ALLOCATOR_DEBUG_H
#define ARENA_ALLOCATOR_DEBUG_H

#include "structs.h"
#include <string.h>
#include <unistd.h>

/// @brief Logs a message to the console.
/// @param log_and_stream log_stdout = prints to stdout. log_stderr = prints to stderr.
/// @param str The string to format and print.
/// @param ... format arguments
/// @note The output condition depends on the allocator's debug level.
/// @details
/// debug_level 0 == do not log
/// @details
/// debug_level 1 == only log errors
/// @details
/// debug_level 2 == log all
ATTR_PRIVATE extern void
log_to_console(void (*log_and_stream)(const char *format, va_list va_args), const char *str, ...);

/// @brief Prints all important statistics about the arena at any given time.
/// @param arena The arena to print statistics.
ATTR_PROTECTED void
debug_print_memory_usage(const Arena *restrict arena);

/// @brief Formats a string + variadic list then prints to stdout.
/// @param string The string to format and print.
/// @param arg_list Variadic function parameter list.
[[maybe_unused]] ATTR_PRIVATE ATTR_INLINE inline static void
log_stdout(const char *string, va_list arg_list)
{
	const char *prefix = "SYNC_ALLOC [LOG] ";
	write(1, prefix, strlen(prefix));
	vfprintf(stdout, string, arg_list);
    fflush_unlocked(stdout);
}

/// @brief Formats a string + variadic list then prints to stderr.
/// @param string The string to format and print.
/// @param arg_list Variadic function parameter list.
[[maybe_unused]] ATTR_PRIVATE ATTR_INLINE static void
log_stderr(const char *restrict string, va_list arg_list)
{
	const char *prefix = "SYNC_ALLOC [ERR] ";
	write(2, prefix, strlen(prefix));
	vfprintf(stderr, string, arg_list);
    fflush_unlocked(stderr);
}


ATTR_PRIVATE extern Debug_VTable sync_alloc_log;

#endif //ARENA_ALLOCATOR_DEBUG_H
