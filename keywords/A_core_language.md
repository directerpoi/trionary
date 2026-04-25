# A. Core Language Features

> Current keywords: `lst`, `whn`, `trn`, `sum`, `emt`, `fn`, `end`, `use`, `inpt`

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| A01 | `if` | keyword | Begin a conditional block | `if x > 0` | v0.4 |
| A02 | `els` | keyword | Else branch of a conditional | `els` | v0.4 |
| A03 | `elif` | keyword | Else-if branch | `elif x == 0` | v0.5 |
| A04 | `for` | keyword | Counted / range loop | `for i 1 10` | v0.4 |
| A05 | `whl` | keyword | While loop (condition-driven) | `whl x < 100` | v0.4 |
| A06 | `each` | keyword | Iterate over every element of a list | `each item arr` | v0.4 |
| A07 | `brk` | keyword | Break out of the nearest loop | `brk` | v0.4 |
| A08 | `nxt` | keyword | Skip to next iteration (continue) | `nxt` | v0.4 |
| A09 | `ret` | keyword | Explicitly return a value from `fn` | `ret x * 2` | v0.4 |
| A10 | `not` | operator | Logical NOT | `not x > 0` | v0.4 |
| A11 | `and` | operator | Logical AND | `x > 0 and x < 10` | v0.4 |
| A12 | `or` | operator | Logical OR | `x < 0 or x > 10` | v0.4 |
| A13 | `in` | operator | Membership test (value in list/string) | `x in arr` | v0.5 |
| A14 | `let` | keyword | Declare an immutable (constant) binding | `let pi = 3.14159` | v0.5 |
| A15 | `rpt` | keyword | Repeat a block N times | `rpt 5` | v0.4 |
