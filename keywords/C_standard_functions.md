# C. Standard Functions

> Functions callable inside expressions and pipelines.
> Current built-ins: `floor`, `ceil`, `abs`, `sqrt`, `pow` (math); `print`, `read_line` (io).

## Math Functions

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| C01 | `sin` | function | Sine of angle in radians | `sin 1.57` | v0.5 |
| C02 | `cos` | function | Cosine of angle in radians | `cos 0` | v0.5 |
| C03 | `tan` | function | Tangent of angle in radians | `tan 0.78` | v0.5 |
| C04 | `log` | function | Natural logarithm | `log 2.71` | v0.5 |
| C05 | `log10` | function | Base-10 logarithm | `log10 100` | v0.5 |
| C06 | `exp` | function | Euler's number raised to a power | `exp 1` | v0.5 |
| C07 | `mod` | function / operator | Modulo (remainder) | `mod 10 3` | v0.4 |
| C08 | `round` | function | Round to nearest integer | `round 3.7` | v0.4 |
| C09 | `min` | function | Minimum of two or more values | `min 3 7 2` | v0.4 |
| C10 | `max` | function | Maximum of two or more values | `max 3 7 2` | v0.4 |
| C11 | `clmp` | function | Clamp value between low and high | `clmp x 0 100` | v0.5 |
| C12 | `rnd` | function | Random float in [0, 1) | `rnd` | v0.5 |
| C13 | `rndi` | function | Random integer in range | `rndi 1 6` | v0.5 |

## List / Array Functions

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| C14 | `srt` | function | Sort a list ascending | `arr \| srt` | v0.4 |
| C15 | `srtd` | function | Sort a list descending | `arr \| srtd` | v0.4 |
| C16 | `rev` | function | Reverse a list | `arr \| rev` | v0.4 |
| C17 | `cnt` | function | Count / length of a list | `cnt arr` | v0.4 |
| C18 | `avg` | function | Arithmetic mean of a list | `avg arr` | v0.4 |
| C19 | `unq` | function | Remove duplicate values | `arr \| unq` | v0.5 |
| C20 | `zip` | function | Zip two lists element-by-element | `zip a b` | v0.5 |
| C21 | `fnd` | function | Find first matching element | `fnd arr x` | v0.5 |
| C22 | `idx` | function | Index of element in list | `idx arr x` | v0.5 |
| C23 | `slc` | function | Slice a sub-list from start to end | `slc arr 1 3` | v0.5 |
| C24 | `flat` | function | Flatten a nested list one level | `flat arr` | v1.0 |
| C25 | `push` | function | Append element to end of list | `push arr x` | v0.4 |
| C26 | `pop` | function | Remove and return last element | `pop arr` | v0.4 |

## String Functions

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| C27 | `cat` | function | Concatenate strings | `cat "Hello" " " "World"` | v0.4 |
| C28 | `len` | function | Length of a string | `len "hello"` | v0.4 |
| C29 | `sub` | function | Extract a substring | `sub "hello" 1 3` | v0.4 |
| C30 | `upr` | function | Convert string to uppercase | `upr "hello"` | v0.5 |
| C31 | `lwr` | function | Convert string to lowercase | `lwr "HELLO"` | v0.5 |
| C32 | `trm` | function | Trim leading and trailing whitespace | `trm " hi "` | v0.5 |
| C33 | `spl` | function | Split string by delimiter | `spl "a,b,c" ","` | v0.5 |
| C34 | `has` | function | Check if string contains a substring | `has "hello" "ell"` | v0.5 |
| C35 | `rep` | function | Replace occurrences in a string | `rep "aab" "a" "x"` | v0.5 |
| C36 | `fmt` | function | Format a string with values | `fmt "x={}" x` | v0.5 |
| C37 | `num` | function | Parse a number from a string | `num "3.14"` | v0.5 |
| C38 | `tostr` | function | Convert any value to its string form | `tostr 42` | v0.4 |
