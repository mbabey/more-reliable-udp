#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * fatal_errno.
 * <p>
 * For calling when a significant unrecoverable error has occured. Prints
 * an error message.
 * Following a call to this function, program memory should be cleaned up and the
 * program should terminate.
 * </p>
 * // TODO: params
 */
void fatal_errno(const char *file, const char *func, const size_t line, int err_code);

/**
 * advise_usage.
 * <p>
 * For calling when a user enters the program command incorrectly. Prints
 * a message that advises the user on how to enter the program command
 * properly
 * </p>
 * @param usage_message - the advice message
 */
void advise_usage(const char *usage_message);
