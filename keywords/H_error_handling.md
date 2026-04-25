# H. Error Handling

> Adds safe, recoverable error paths without breaking the minimal philosophy.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| H01 | `try` | keyword | Begin a guarded block | `try` | v0.5 |
| H02 | `ctch` | keyword | Catch an error from the preceding `try` | `ctch e` | v0.5 |
| H03 | `thr` | keyword | Throw a runtime error | `thr "Value must be positive"` | v0.5 |
| H04 | `err` | keyword / type | Named error value | `err "Division by zero"` | v0.5 |
| H05 | `ok` | function | Return true if a value is not an error | `ok result` | v0.5 |
| H06 | `orr` | keyword | Pipeline safe-fallback operator | `x / y orr 0` | v0.5 |
| H07 | `asrt` | keyword | Assert a condition; abort on failure | `asrt x > 0` | v0.4 |
| H08 | `dflt` | keyword | Provide a default when a value is nil/error | `x dflt 0` | v0.4 |
