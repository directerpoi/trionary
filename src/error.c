#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void error_at(int line, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, " at line %d\n", line);
    exit(1);
}
