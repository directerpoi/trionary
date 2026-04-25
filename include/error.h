#ifndef ERROR_H
#define ERROR_H

/* error_at: print a formatted error message to stderr and exit with code 1.
 *
 * The message is printed in the canonical format:
 *   Error: <message> at line <N>
 *
 * If set_error_source() has been called with a non-NULL buffer, the offending
 * source line is also printed below the error message, followed by a '^' caret
 * at the column indicated by the most recent set_error_col() call (if any).
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

/* set_error_hint: register a one-line hint to be printed after the next
 * error_at() call.  The hint is automatically cleared after it is printed.
 * Call this immediately before error_at() at any site that can provide
 * actionable guidance (e.g. "Did you mean 'lst'?").
 */
void set_error_hint(const char *hint);

/* set_error_source: store a pointer to the raw source buffer so that
 * error_at() can print the offending source line for context.
 * The pointer must remain valid until the program exits or error_at() is called.
 * Passing NULL disables source-context printing.
 */
void set_error_source(const char *src);

/* set_error_col: record the 1-based column of the next error so that
 * error_at() can print a '^' caret under the offending character.
 * The column is automatically cleared after it is printed.
 * If not called (or called with 0), no caret is shown.
 */
void set_error_col(int col);

#endif
