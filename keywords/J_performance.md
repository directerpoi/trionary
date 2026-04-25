# J. Performance / Runtime

> Features that affect execution efficiency, compilation, and concurrency.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| J01 | `tim` | function | Measure wall-clock time of an expression | `tim fn heavy_calc` | v0.5 |
| J02 | `lzy` | keyword | Mark an expression for lazy evaluation | `lzy x = expensive_fn` | v1.0 |
| J03 | `csh` | keyword | Cache the result of a pure expression | `csh fn fib n` | v1.0 |
| J04 | `par` | keyword | Execute pipeline branches in parallel | `par lst [a,b] \| trn fn` | v1.0 |
| J05 | `byt` | keyword | Compile a function to bytecode ahead-of-time | `byt fn hot_path` | v1.0 |
| J06 | `opt` | keyword | Hint the optimizer for a code block | `opt level 2` | v1.0 |
| J07 | `mem` | function | Return current heap memory usage in bytes | `mem` | v1.0 |
| J08 | `spwn` | keyword | Spawn a concurrent background task | `spwn fn worker data` | v1.0 |
