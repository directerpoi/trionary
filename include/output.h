#ifndef OUTPUT_H
#define OUTPUT_H

#include "exec.h"

void emit_value(Value v);
void emit_value_no_newline(Value v);
void emit_labeled_value(const char* label, Value v);

#endif