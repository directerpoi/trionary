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