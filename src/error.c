#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static char pending_hint[256] = "";
static const char *source_buf = NULL;
static int error_col = 0;

void set_error_hint(const char *hint) {
    snprintf(pending_hint, sizeof(pending_hint), "%s", hint);
}

void set_error_source(const char *src) {
    source_buf = src;
}

void set_error_col(int col) {
    error_col = col;
}

void error_at(int line, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, " at line %d\n", line);

    /* Print source context when the buffer is available and line is valid. */
    if (source_buf != NULL && line > 0) {
        /* Walk the source to find the start of the requested line. */
        const char *p = source_buf;
        int cur_line = 1;
        while (*p != '\0' && cur_line < line) {
            if (*p == '\n') cur_line++;
            p++;
        }

        /* Find the end of this line (stop before '\n' or '\0'). */
        const char *end = p;
        while (*end != '\0' && *end != '\n') end++;

        /* Compute the width of the line number for alignment. */
        int num_width = 1;
        int tmp = line;
        while (tmp >= 10) { tmp /= 10; num_width++; }

        fprintf(stderr, "  %d | %.*s\n", line, (int)(end - p), p);

        /* Print a caret under the offending column when one is recorded. */
        if (error_col > 0) {
            int prefix_len = 2 + num_width + 3; /* "  N | " */
            for (int k = 0; k < prefix_len + error_col - 1; k++)
                fputc(' ', stderr);
            fprintf(stderr, "^\n");
            error_col = 0;
        }
    }

    if (pending_hint[0] != '\0') {
        fprintf(stderr, "  Hint: %s\n", pending_hint);
        pending_hint[0] = '\0';
    }
    exit(1);
}
