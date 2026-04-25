# G. Module System Expansion

> Evolves the current `use math` / `use io` system into a general import mechanism.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| G01 | `imp` | keyword | Import a module (replaces/extends `use`) | `imp math` | v0.4 |
| G02 | `as` | keyword | Alias an imported module or symbol | `imp math as m` | v0.5 |
| G03 | `frm` | keyword | Import a specific symbol from a module | `frm math imp sqrt` | v0.5 |
| G04 | `exp` | keyword | Export a symbol from the current file | `exp fn add` | v0.5 |
| G05 | `pkg` | keyword | Declare the current file's package name | `pkg utils` | v0.5 |
| G06 | `std` | keyword | Reference the standard library root | `use std.str` | v0.4 |
| G07 | `nsp` | keyword | Define a named namespace block | `nsp geometry` | v1.0 |
| G08 | `bnd` | keyword | Bind an external C function | `bnd fn c_memcpy` | v1.0 |
