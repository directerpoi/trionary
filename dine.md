# v0.4.0 Core Language Features — Control Flow & New Operators

## What Was Done

Implemented 15 new keywords and significantly evolved the Trionary interpreter from a statement-slicing model to a traditional recursive descent parser with block-level control flow.

---

## New Keywords

| Keyword | Description | Example |
|---------|-------------|---------|
| `if`, `els`, `elif` | Conditional branching | `if x > 0 ... elif x == 0 ... els ... end` |
| `for` | Counted range loop | `for i 1 10 ... end` |
| `whl` | Condition-driven loop | `whl x < 100 ... end` |
| `each` | Iterate over list elements | `each item arr ... end` |
| `rpt` | Repeat a block N times | `rpt 5 ... end` |
| `brk`, `nxt` | Loop control | `brk`, `nxt` |
| `ret` | Return from function | `ret x * 2` |
| `not`, `and`, `or` | Logical operators | `if not (x > 0) and (y < 0)` |
| `in` | Membership test | `if x in [1, 2, 3]` |
| `let` | Immutable variable declaration | `let pi = 3.14159` |

---

## Technical Changes

### Parser Evolution
- **Removed Statement Slicing:** The interpreter now parses the entire source file into a hierarchical AST of blocks and statements.
- **Recursive Descent:** Implemented a full expression parser with proper operator precedence (including logical operators).
- **Block Nodes:** Added support for nested blocks of code, allowing for nested loops and conditionals.

### Value System
- **Value Struct:** Introduced a `Value` struct that handles both `Number` and `List` types.
- **Immutability:** Added an `is_immutable` flag to variables to support the `let` keyword.
- **List Literals:** Added support for `[e1, e2, ...]` as standalone expressions.

### Execution Engine
- **Control Status:** The execution engine now tracks `STATUS_NORMAL`, `STATUS_BREAK`, `STATUS_CONTINUE`, and `STATUS_RETURN` to correctly manage flow across nested structures.
- **Scope Management:** Updated the symbol table to store `Value` objects and handle immutability checks.

---

## Grammar Changes

```
program    → block
block      → statement*
statement  → if_stmt | for_stmt | whl_stmt | each_stmt | rpt_stmt 
           | let_stmt | assign_stmt | pipeline | arith_stmt | fn_def 
           | use_stmt | inpt_stmt | 'brk' | 'nxt' | ret_stmt
if_stmt    → 'if' expr NEWLINE? block ('elif' expr NEWLINE? block)* ('els' NEWLINE? block)? 'end'
for_stmt   → 'for' IDENT expr expr NEWLINE? block 'end'
whl_stmt   → 'whl' expr NEWLINE? block 'end'
each_stmt  → 'each' IDENT IDENT NEWLINE? block 'end'
rpt_stmt   → 'rpt' expr NEWLINE? block 'end'
let_stmt   → 'let' IDENT '=' expr
ret_stmt   → 'ret' expr?
expr       → ... (proper operator precedence)
primary    → NUMBER | IDENT | '(' expr ')' | '[' expr_list? ']' | IDENT primary* (function call)
```

---

## Backward Compatibility
- Existing pipelines (`lst ... | ... -> emt`) and arithmetic statements still work.
- Existing function definitions (`fn ... end`) remain compatible.
- The symbol table continues to persist variables between statements.
