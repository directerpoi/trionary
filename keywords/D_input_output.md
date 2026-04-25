# D. Input / Output

> Extends the current `emt` (emit) and `inpt` (input) system.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| D01 | `say` | keyword | Print value(s) followed by a newline | `say "Hello, World!"` | v0.4 |
| D02 | `prt` | keyword | Print value(s) without trailing newline | `prt "Loading..."` | v0.4 |
| D03 | `ask` | keyword | Prompt user with a string and read input | `ask "Enter name: "` | v0.4 |
| D04 | `frd` | keyword | Read entire contents of a file | `frd "data.txt"` | v0.5 |
| D05 | `fwr` | keyword | Write (overwrite) content to a file | `fwr "out.txt" data` | v0.5 |
| D06 | `fap` | keyword | Append content to a file | `fap "log.txt" line` | v0.5 |
| D07 | `fex` | function | Check if a file path exists | `fex "data.txt"` | v0.5 |
| D08 | `fls` | function | List files in a directory | `fls "."` | v1.0 |
| D09 | `csv` | keyword | Parse a CSV file into a list of lists | `csv "data.csv"` | v1.0 |
| D10 | `jrd` | keyword | Read and parse a JSON file | `jrd "config.json"` | v1.0 |
