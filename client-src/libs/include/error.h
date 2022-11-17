#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * fatal_errno.
 * <p>
 * For calling when a significant unrecoverable error has occurred. Prints an error message.
 * Following a call to this function, program memory should be cleaned up and the program should terminate.
 * Sets errno to ENOTRECOVERABLE.
 * </p>
 * @param file - file in which error occurred
 * @param func - function in which error occurred
 * @param line - line on which error occurred
 * @param err_code - the error code of the error which occurred
 */
void fatal_errno(const char *file, const char *func, const size_t line, int err_code);

/**
 * advise_usage.
 * <p>
 * For calling when a user enters the program command incorrectly. Prints
 * a message that advises the user on how to enter the program command
 * properly. Sets errno to ENOTRECOVERABLE.
 * </p>
 * @param usage_message - the advice message
 */
void advise_usage(const char *usage_message);
