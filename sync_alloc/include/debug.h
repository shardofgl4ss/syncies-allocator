//
// Created by SyncShard on 10/16/25.
//
// ReSharper disable CppParameterMayBeConst
// ide codechecker is a pos sometimes

#ifndef ARENA_ALLOCATOR_DEBUG_H
#define ARENA_ALLOCATOR_DEBUG_H

#include "structs.h"
#include <pthread.h>
#include <string.h>

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

/// @brief Logs a message to the console.
/// @param log_and_stream log_stdout = prints to stdout. log_stderr = prints to stderr.
/// @param str The string to format and print.
/// @param ... format arguments
/// @note The output condition depends on the allocator's debug level.
/// @details
/// debug_level 0 == do not log/do not include the log functions in the code.
/// @details
/// debug_level 1 == only log errors
/// @details
/// debug_level 2 == log all
[[maybe_unused]] ATTR_PRIVATE extern void
log_to_console(void (*log_and_stream)(const char *format, va_list va_args), const char *str, ...);

/// @brief Prints all important statistics about the arena at any given time.
[[maybe_unused]] ATTR_PROTECTED void
debug_print_memory_usage();

/// @brief Formats a string + variadic list then prints to stdout.
/// @param string The string to format and print.
/// @param arg_list Variadic function parameter list.
[[maybe_unused]] inline static void
log_stdout(const char *string, va_list arg_list)
{
	const char *prefix = "SYNC_ALLOC [LOG] ";
	pthread_mutex_lock(&log_lock);
	fputs(prefix, stdout);
	vfprintf(stdout, string, arg_list);
    fflush_unlocked(stdout);
	pthread_mutex_unlock(&log_lock);
}

/// @brief Formats a string + variadic list then prints to stderr.
/// @param string The string to format and print.
/// @param arg_list Variadic function parameter list.
[[maybe_unused]] inline static void
log_stderr(const char *restrict string, va_list arg_list)
{
	const char *prefix = "SYNC_ALLOC [ERR] ";
	pthread_mutex_lock(&log_lock);
	fputs(prefix, stderr);
	vfprintf(stderr, string, arg_list);
    fflush_unlocked(stderr);
	pthread_mutex_unlock(&log_lock);
}


[[maybe_unused]] extern Debug_VTable sync_alloc_log ATTR_PRIVATE;

#endif //ARENA_ALLOCATOR_DEBUG_H
