# F. Functional Features

> Evolves the pipeline model towards higher-order programming.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| F01 | `lmb` | keyword | Define an anonymous (lambda) function | `lmb x -> x * 2` | v0.5 |
| F02 | `mpa` | function | Apply a function to each list element | `lst [1,2,3] \| mpa lmb x -> x*2` | v0.4 |
| F03 | `fil` | function | Keep elements satisfying a predicate | `lst [1,2,3] \| fil lmb x -> x>1` | v0.4 |
| F04 | `red` | function | Reduce a list to a single value | `lst [1,2,3] \| red lmb a x -> a+x` | v0.5 |
| F05 | `cmp` | function | Compose two functions into one | `cmp f g` | v0.5 |
| F06 | `apl` | function | Apply a function with an argument list | `apl fn [1, 2]` | v0.5 |
| F07 | `part` | function | Partially apply a function | `part add 1` | v1.0 |
| F08 | `once` | keyword | Wrap a function so it executes only once | `once fn init` | v1.0 |
