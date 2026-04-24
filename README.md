# Trionary

> A minimal, readable programming language for pipeline-based data transformations.

Trionary is a statically-structured scripting language built around five keywords. It is intentionally small: no runtime, no dependencies, no ambiguity. The entire language is a single C11 binary.

---

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Build from Source](#build-from-source)
- [Usage](#usage)
- [Language Reference](#language-reference)
  - [Comments](#comments)
  - [Variables](#variables)
  - [Arithmetic](#arithmetic)
  - [Keywords](#keywords)
  - [Pipelines](#pipelines)
- [Examples](#examples)
- [Project Structure](#project-structure)
- [Design Principles](#design-principles)
- [Contributing](#contributing)
- [License](#license)

---

## Features

- **5 keywords** — `lst`, `whn`, `trn`, `sum`, `emt`
- **Pipeline-oriented** — chain filters, transforms, and aggregations
- **Zero dependencies** — pure C11, no libraries beyond the C standard math library
- **Single binary** — one executable, no installer or runtime required
- **POSIX-compatible** — runs on Linux, macOS, and any POSIX-compliant system
- **Clear error messages** — line-number-aware diagnostics

---

## Installation

### Prerequisites

| Tool | Minimum Version |
|------|----------------|
| GCC (or any C11-compatible compiler) | 4.8+ |
| GNU Make | 3.81+ |

### Build from Source

```bash
git clone https://github.com/api-bridges/trionary.git
cd trionary
make
```

This produces a `tri` binary in the project root.

To remove the compiled binary:

```bash
make clean
```

---

## Usage

```
tri run <file.tri>
```

| Argument | Description |
|----------|-------------|
| `run` | Execute a Trionary source file |
| `<file.tri>` | Path to the source file |

**Example:**

```bash
tri run tests/demo.tri
```

---

## Language Reference

Trionary source files use the `.tri` extension.

### Comments

```tri
# This is a comment — everything after # is ignored
```

### Variables

Variables are assigned with `=`. Only numeric values (integer or float) are supported. A variable must be assigned before it is used.

```tri
price    = 100
discount = 20
```

### Arithmetic

Standard arithmetic expressions with proper operator precedence (PEMDAS). Emit the result with `-> emt`.

```tri
5 + 10 -> emt          # 15
3 * 4 + 2 -> emt       # 14  (* binds tighter than +)
```

Variables can appear in expressions:

```tri
price = 100
discount = 20
price - discount -> emt   # 80
```

### Keywords

| Keyword | Role | Description |
|---------|------|-------------|
| `lst` | Source | Opens a pipeline with a literal list of values |
| `whn` | Filter | Keeps only elements that satisfy a condition |
| `trn` | Transform | Applies an arithmetic operation to each element |
| `sum` | Aggregate | Reduces the pipeline to a single sum |
| `emt` | Output | Prints the current value(s) to stdout |

#### `lst` — List source

```tri
lst [1, 2, 3, 4, 5]
```

Begins a pipeline with a comma-separated list of numbers.

#### `whn` — Filter

```tri
whn >5
whn <3
```

Keeps elements that satisfy the given comparison. Supported operators: `>`, `<`, `>=`, `<=`, `=`.

#### `trn` — Transform

```tri
trn *2
trn +10
trn -1
```

Applies an arithmetic operation to every element in the pipeline.

#### `sum` — Aggregate

```tri
sum
```

Collapses the pipeline into a single value equal to the sum of all remaining elements.

#### `emt` — Emit

```tri
emt
-> emt
```

Prints the current pipeline value(s) or the result of an expression to stdout.

### Pipelines

Stages are chained with `|`. A pipeline begins with `lst` and ends with `emt` (either standalone on the next line, or inline with `-> emt`).

```tri
lst [values] | stage | stage | ... -> emt
```

Or, when using `sum` as the final reduction stage:

```tri
lst [values] | stage | stage | sum
emt
```

---

## Examples

### Hello, arithmetic

```tri
10 + 5 -> emt        # 15
3 * 4 + 2 -> emt     # 14
```

### Variables

```tri
a = 10
b = 7
a + 5  -> emt    # 15
a * b  -> emt    # 70
```

### Filter a list

```tri
# Print numbers greater than 5
lst [1,2,3,4,5,6,7,8,9,10] | whn >5 -> emt
# Output: 6  7  8  9  10
```

### Filter and transform

```tri
# Double every number greater than 5
lst [1,2,3,4,5,6,7,8,9,10] | whn >5 | trn *2 -> emt
# Output: 12  14  16  18  20
```

### Sum a list

```tri
lst [1,2,3,4,5] | sum -> emt    # 15
```

### Complex pipeline

```tri
# Keep numbers >2, multiply each by 10, then sum: (3+4+5)*10 = 120
lst [1,2,3,4,5] | whn >2 | trn *10 | sum
emt
# Output: 120
```

---

## Project Structure

```
trionary/
├── src/
│   ├── main.c      # CLI entry point and statement execution loop
│   ├── reader.c    # File reading into memory buffer
│   ├── lexer.c     # Tokenizer / lexical analyser
│   ├── parser.c    # Pattern-based parser with AST generation
│   ├── exec.c      # Execution engine with symbol table
│   └── output.c    # Output formatting
├── include/
│   ├── reader.h
│   ├── lexer.h
│   ├── parser.h
│   ├── exec.h
│   └── output.h
├── tests/
│   ├── demo.tri          # Full feature demonstration
│   ├── test_arith.tri    # Arithmetic tests
│   ├── test_vars.tri     # Variable tests
│   ├── test_pipeline.tri # Pipeline tests
│   ├── test_all.tri      # Combined test suite
│   ├── test_error.tri    # Error case: missing emit
│   ├── test_invalid.tri  # Error case: unknown keyword
│   └── test_malformed.tri# Error case: malformed syntax
├── Makefile
└── IMPLEMENTATION_SUMMARY.md
```

---

## Design Principles

| Principle | Description |
|-----------|-------------|
| **Readable like English** | Keywords are short but self-describing |
| **One concept = one keyword** | No synonyms, no aliases |
| **Strict structure** | Every statement has exactly one valid form |
| **Clarity over flexibility** | Predictable behaviour is preferred over expressive power |
| **Zero dependencies** | The binary links only against the C standard math library |
| **Single executable** | No installer, no runtime, no package manager |

---

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Make your changes
4. Build and verify: `make && ./tri run tests/test_all.tri`
5. Open a pull request

Please keep changes focused and minimal. New language features should follow the existing design principles.

---

## License

This project is open source. See [LICENSE](LICENSE) for details.
