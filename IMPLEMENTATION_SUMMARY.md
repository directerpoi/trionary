# Trionary v0 Implementation Summary

## Overview
Trionary v0 is a minimal, readable programming language with a focus on pipeline-based data transformations. The implementation is a single, zero-dependency C11 binary.

## Project Structure
```
/workspace/
├── main.c          - CLI entry point and statement execution loop
├── reader.c/h      - File reading into memory buffer
├── lexer.c/h       - Tokenizer/lexical analyzer
├── parser.c/h      - Pattern-based parser with AST generation
├── exec.c/h        - Execution engine with symbol table
├── output.c/h      - Output formatting system
├── Makefile        - Build configuration
└── test_*.tri      - Test files
```

## Keywords (5 total)
1. **lst** - List source: Initiates a pipeline with literal values
2. **whn** - Filter: Applies condition to filter elements
3. **trn** - Transform: Applies arithmetic operation to elements
4. **sum** - Summarise: Reduces pipeline to single sum
5. **emt** - Emit: Outputs result to stdout

## Language Features

### Arithmetic
```tri
5 + 10 -> emt          # Output: 15
3 * 4 + 2 -> emt       # Output: 14 (precedence: * before +)
```

### Variables
```tri
a = 10
a + 5 -> emt           # Output: 15
```

### Pipelines
```tri
lst [1,2,3,4,5] | whn >2 | trn *10 | sum
emt                    # Output: 120
```

## Implementation Details

### Lexer
- Single-pass O(n) character scanner
- Recognizes exactly 5 keywords (no synonyms)
- Supports numbers (integers and floats), identifiers, and operators
- Handles comments (# to end of line)
- Error reporting with line numbers

### Parser
- Fixed-pattern matching (not recursive descent)
- Three statement types:
  1. **Assign**: `IDENT = NUMBER`
  2. **Arith-emit**: `expr '->' 'emt'`
  3. **Pipeline**: `lst [values] stage* emt`
- Proper operator precedence (PEMDAS)
- AST node structures for each statement type

### Execution Engine
- Direct AST walk (no bytecode or VM)
- Symbol table for variable storage (max 256 variables)
- Single-pass pipeline execution (O(1) memory for aggregate operations)
- Processes each element sequentially through filter → transform → sum

### Output System
- Integers printed without decimal point
- Floats printed with up to 6 significant figures
- All output to stdout with newline

## Test Results

### test_arith.tri
```
Expected: 15, 14
Actual:   15, 14 ✓
```

### test_vars.tri
```
Expected: 15, 70
Actual:   15, 70 ✓
```

### test_pipeline.tri
```
Expected: 120
Actual:   120 ✓
```

## Build & Run
```bash
# Compile
gcc -std=c11 -Wall -Wextra -O2 main.c reader.c lexer.c parser.c exec.c output.c -o tri -lm

# Run
./tri run file.tri
```

## Design Principles Met
✓ Readable like English, parsed like code
✓ Minimal vocabulary (exactly 5 keywords)
✓ No synonyms (one concept = one keyword)
✓ Strict structure (no ambiguity)
✓ Clarity > flexibility
✓ Zero external dependencies
✓ Single binary executable
✓ POSIX-compatible

## Validation Criteria
- ✓ All three test files produce expected output
- ✓ Invalid keywords detected and reported
- ✓ Malformed syntax generates errors
- ✓ Proper error messages with line numbers
- ✓ Consistent behavior across operations

## Notes
- Pipeline without `sum` emits each element individually
- Operator precedence follows standard PEMDAS rules
- Comments are supported with `#` to end of line
- Variables are single-pass (no re-declaration in scope)
- All arithmetic uses double precision internally