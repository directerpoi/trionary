# I. Developer Experience

> Tools that improve debugging, testing, and introspection without polluting production output.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| I01 | `dbg` | keyword | Dump a value and its variable name to stderr | `dbg x` | v0.4 |
| I02 | `log` | keyword | Emit a labeled message to stderr | `log "step reached"` | v0.4 |
| I03 | `typ` | function | Return the type name of a value as a string | `typ x` | v0.4 |
| I04 | `tst` | keyword | Declare an inline unit test | `tst "add" add 1 2 == 3` | v0.4 |
| I05 | `trc` | keyword | Trace every pipeline stage value to stderr | `trc on` | v0.5 |
| I06 | `doc` | keyword | Attach a documentation string to a function | `doc "Computes the mean"` | v0.5 |
| I07 | `chk` | keyword | Assert a runtime type check | `chk x str` | v0.5 |
| I08 | `bpt` | keyword | Pause execution and enter interactive debug | `bpt` | v1.0 |
