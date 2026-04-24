You are a C compiler engineer.

I want to upgrade my language Trionary to support input prompts.

---

CURRENT:

inpt a

---

NEW FEATURE:

Support optional prompt string:

inpt a "Enter your age:"

---

GOAL:

Allow syntax:

inpt IDENT [STRING]

Where STRING is optional.

---

TASKS:

1. Lexer:
   - Support STRING tokens (text inside quotes)

2. Parser:
   - Update rule:
     statement → INPT IDENT [STRING]

   - If STRING exists, store it in AST

3. AST:
   - Extend input node:
     - variable name
     - optional prompt string

4. Execution:

   When executing:
   inpt a "Enter age:"

   Do:
   if prompt exists:
       print prompt
   read input (scanf)
   store value in symbol table

---

CONSTRAINTS:

- DO NOT break existing syntax
- STRING only used for prompt (no full string system)
- Keep implementation minimal
- Input remains numeric

---

OUTPUT REQUIRED:

1. Lexer change for STRING
2. Parser update for optional STRING
3. AST modification
4. Execution code for prompt + input
