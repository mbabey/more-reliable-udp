#include "../include/error.h"
#include <stdio.h>
#include <string.h>

void fatal_errno(const char *file, const char *func, const size_t line, int err_code) // NOLINT(bugprone-easily-swappable-parameters)
{
    const char *msg;
    
    msg = strerror(err_code);                                                                  // NOLINT(concurrency-mt-unsafe)
    fprintf(stderr, "Error (%s @ %s:%zu %d) - %s\n", file, func, line, err_code, msg);  // NOLINT(cert-err33-c)
    errno = ENOTRECOVERABLE;                                                                           // NOLINT(concurrency-mt-unsafe)
}

void advise_usage(const char *usage_message)
{
    fprintf(stderr, "Usage: %s\n", usage_message); // NOLINT(cert-err33-c) : val not needed
    errno = ENOTRECOVERABLE;
}
