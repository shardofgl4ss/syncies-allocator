//
// Created by SyncShard on 10/16/25.
//
// ReSharper disable CppParameterMayBeConst
// ide codechecker is a pos sometimes

#ifndef ARENA_ALLOCATOR_DEBUG_H
#define ARENA_ALLOCATOR_DEBUG_H

#include "structs.h"
#include <stdio.h>

/// @brief Formats a string + variadic list then prints to stdout.
/// @param string The string to format and print.
/// @param arg_list Variadic function parameter list.
extern void log_stdout(const char *string, va_list arg_list);

/// @brief Formats a string + variadic list then prints to stderr.
/// @param string The string to format and print.
/// @param arg_list Variadic function parameter list.
extern void log_stderr(const char *restrict string, va_list arg_list);

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
[[maybe_unused, gnu::visibility("hidden")]]
extern void
log_to_console(void (*log_and_stream)(const char *format, va_list va_args), const char *str, ...);

/// @brief Prints all important statistics about the arena at any given time.
[[maybe_unused, gnu::visibility("protected")]]
void debug_print_memory_usage();

[[maybe_unused, gnu::visibility("hidden")]]
extern debug_vtable_t sync_alloc_log;

#endif //ARENA_ALLOCATOR_DEBUG_H