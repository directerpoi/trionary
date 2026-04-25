#include "output.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

static void emit_val_internal(Value v) {
    switch (v.type) {
        case VAL_NIL: printf("nil"); break;
        case VAL_BOOL: printf(v.as.boolean ? "true" : "fls"); break;
        case VAL_INT: printf("%lld", v.as.integer); break;
        case VAL_FLOAT: printf("%.6g", v.as.float_val); break;
        case VAL_STRING: printf("%s", v.as.string); break;
        case VAL_ARRAY:
            printf("[");
            for (int i=0; i<v.as.list.length; i++) {
                emit_val_internal(v.as.list.elements[i]);
                if (i < v.as.list.length-1) printf(", ");
            }
            printf("]");
            break;
        case VAL_SET:
            printf("{");
            for (int i=0; i<v.as.list.length; i++) {
                emit_val_internal(v.as.list.elements[i]);
                if (i < v.as.list.length-1) printf(", ");
            }
            printf("}");
            break;
        case VAL_TUPLE:
            printf("(");
            for (int i=0; i<v.as.list.length; i++) {
                emit_val_internal(v.as.list.elements[i]);
                if (i < v.as.list.length-1) printf(", ");
            }
            printf(")");
            break;
        case VAL_PAIR:
            if (v.as.pair.key) emit_val_internal(*v.as.pair.key);
            printf(": ");
            if (v.as.pair.value) emit_val_internal(*v.as.pair.value);
            break;
        case VAL_MAP:
            printf("{");
            for (int i=0; i<v.as.map.length; i++) {
                emit_val_internal(v.as.map.keys[i]);
                printf(": ");
                emit_val_internal(v.as.map.values[i]);
                if (i < v.as.map.length-1) printf(", ");
            }
            printf("}");
            break;
        default: break;
    }
}

void emit_value(Value v) {
    emit_val_internal(v);
    printf("\n");
}

void emit_value_no_newline(Value v) {
    emit_val_internal(v);
}

void emit_labeled_value(const char* label, Value v) {
    printf("%s ", label);
    emit_val_internal(v);
    printf("\n");
}
