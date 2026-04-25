#include "output.h"
#include <stdio.h>
#include <math.h>

void emit_value(double v) {
    // Check if it's an integer (within range and no fractional part)
    if (v == floor(v) && v >= -9007199254740992.0 && v <= 9007199254740992.0) {
        printf("%.0f\n", v);
    } else {
        // Print float with up to 6 significant figures
        printf("%.6g\n", v);
    }
}

/* emit_value_no_newline: print the value without a trailing newline.
 * Used by the sep-controlled pipeline output path. */
void emit_value_no_newline(double v) {
    if (v == floor(v) && v >= -9007199254740992.0 && v <= 9007199254740992.0) {
        printf("%.0f", v);
    } else {
        printf("%.6g", v);
    }
}

/* emit_labeled_value: print "label value\n" on a single line. */
void emit_labeled_value(const char* label, double v) {
    if (v == floor(v) && v >= -9007199254740992.0 && v <= 9007199254740992.0) {
        printf("%s %.0f\n", label, v);
    } else {
        printf("%s %.6g\n", label, v);
    }
}