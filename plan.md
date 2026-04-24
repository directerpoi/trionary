You are a C compiler engineer and runtime developer.

I am upgrading my programming language Trionary to v0.3.1 to support input using a keyword: inpt

Current problem:
Assignments only accept numeric literals, causing this error:

"Error: Expected number in assignment"

---

GOAL:

Extend assignment syntax to support:

a = arg0      (CLI input variable)
a = inpt      (interactive input)

---

CURRENT DESIGN:

- assignment → IDENT = NUMBER
- Symbol table exists (sym_get, sym_set)
- Pipeline system already working
- CLI arguments are passed as arg0, arg1, ...

---

REQUIRED CHANGES:

1. Parser Update:
   Modify assignment rule to:

   assignment → IDENT = (NUMBER | IDENT | INPT)

   - NUMBER → numeric literal
   - IDENT → variable (including arg0, arg1)
   - INPT  → special input keyword

2. AST Update:
   Assignment node must store:
   - type: number | variable | input
   - value (if number)
   - variable name (if IDENT)
   - flag for input (if INPT)

3. Execution Update:
   When executing assignment:

   if NUMBER → use literal
   if IDENT  → resolve from symbol table
   if INPT   → read from stdin (scanf)

---

CONSTRAINTS:

- DO NOT break existing syntax
- DO NOT add complex expression parsing
- Keep input numeric only
- Keep implementation minimal
- Maintain performance

---

EXPECTED BEHAVIOR:

Example 1 (CLI input):

a = arg0
b = arg1
a + b -> emt

Command:
./tri run file.tri 10 20

Output:
30

---

Example 2 (interactive input):

a = inpt
b = inpt
a + b -> emt

User enters:
10
20

Output:
30

---

OUTPUT REQUIRED:

1. Updated parser logic for assignment
2. Updated AST structure (minimal change)
3. Updated execution logic (input handling)
4. Any necessary token additions (INPT)
5. Minimal code snippets only

---

Now provide the exact minimal implementation changes.
