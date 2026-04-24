#ifndef ERROR_H
#define ERROR_H

/* error_at: print a formatted error message to stderr and exit with code 1.
 *
 * The message is printed in the canonical format:
 *   Error: <message> at line <N>
 *
 * This is the single, centralised error-reporting point for the entire
 * interpreter.  All parse-time and runtime errors must go through here
 * so that the format stays consistent and fail-fast behaviour is enforced.
 *
 * Declared noreturn so the compiler can omit dead code after every call site.
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
void error_at(int line, const char *fmt, ...);

#endif
