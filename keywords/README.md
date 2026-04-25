# Trionary Keywords & Functions Roadmap

> Language: Trionary v0.3.2 → v1.0  
> Total planned features: **110**  
> Current keywords: `lst` `whn` `trn` `sum` `emt` `fn` `end` `use` `inpt`  
> Current built-ins: `floor` `ceil` `abs` `sqrt` `pow` `print` `read_line`

---

## Quick Reference — All Names

### Existing (v0.3.2)

| Name | Kind | Module |
|------|------|--------|
| `lst` | keyword | core |
| `whn` | keyword | core |
| `trn` | keyword | core |
| `sum` | keyword | core |
| `emt` | keyword | core |
| `fn` | keyword | core |
| `end` | keyword | core |
| `use` | keyword | core |
| `inpt` | keyword | core |
| `floor` | function | math |
| `ceil` | function | math |
| `abs` | function | math |
| `sqrt` | function | math |
| `pow` | function | math |
| `print` | function | io |
| `read_line` | function | io |

---

### Planned Features by Category

| File | Category | Count |
|------|----------|-------|
| [A_core_language.md](A_core_language.md) | Core Language Features | 15 |
| [B_data_types.md](B_data_types.md) | Data Types | 12 |
| [C_standard_functions.md](C_standard_functions.md) | Standard Functions | 38 |
| [D_input_output.md](D_input_output.md) | Input / Output | 10 |
| [E_control_flow.md](E_control_flow.md) | Control Flow | 3 |
| [F_functional.md](F_functional.md) | Functional Features | 8 |
| [G_modules.md](G_modules.md) | Module System | 8 |
| [H_error_handling.md](H_error_handling.md) | Error Handling | 8 |
| [I_developer_experience.md](I_developer_experience.md) | Developer Experience | 8 |
| [J_performance.md](J_performance.md) | Performance / Runtime | 8 |
| **Total** | | **118** |

---

## Priority Roadmap

### v0.4 — Foundation (must-have)

> Goal: add the bare minimum to make Trionary practical for real programs.

**Keywords / operators:**
`if`, `els`, `for`, `whl`, `each`, `brk`, `nxt`, `ret`, `not`, `and`, `or`, `rpt`,
`say`, `prt`, `ask`, `ext`, `stp`, `asrt`, `dflt`, `dbg`, `log`, `tst`,
`imp`, `std`

**Types:**
`str`, `arr`, `bool`, `true`, `fls`

**Functions:**
`mod`, `round`, `min`, `max`, `srt`, `srtd`, `rev`, `cnt`, `avg`,
`push`, `pop`, `cat`, `len`, `sub`, `tostr`, `mpa`, `fil`, `typ`

---

### v0.5 — Power (selective additions)

> Goal: cover 90 % of real-world scripting needs while keeping the language clean.

**Keywords / operators:**
`elif`, `let`, `in`, `lmb`, `try`, `ctch`, `thr`, `err`, `ok`, `orr`,
`frm`, `as`, `exp`, `pkg`, `trc`, `doc`, `chk`, `frd`, `fwr`, `fap`, `fex`

**Types:**
`map`, `int`, `flt`, `pair`, `nil`

**Functions:**
`sin`, `cos`, `tan`, `ln`, `log10`, `epow`, `clmp`, `rnd`, `rndi`,
`unq`, `zip`, `fnd`, `idx`, `slc`, `upr`, `lwr`, `trm`, `spl`, `has`,
`rep`, `fmt`, `num`, `red`, `cmp`, `apl`, `tim`

---

### v1.0 — Extended (deliberate additions)

> Goal: production-ready features for larger programs; keep minimal users unaffected.

**Keywords:** `elif`, `yld`, `lzy`, `csh`, `par`, `byt`, `opt`, `once`, `part` (partial apply), `nsp`, `bnd` (C binding), `bpt`, `spwn`

**Types:** `tpl`, `set`

**Functions:** `flat`, `lsf` (list files), `csv`, `jrd`, `mem`

---

## Risk Analysis

| Risk | Severity | Mitigation |
|------|----------|-----------|
| String type breaks numeric-only assumption | Medium | Introduce `str` as a distinct type; disallow mixed arithmetic |
| `map` name clashes with the data type AND the functional operator | High | **Resolved**: functional map renamed to `mpa`; `map` is reserved for the dictionary type |
| `if`/`els` adds nesting depth; indentation becomes meaningful | Medium | Define nesting via `end` keyword (already in use for `fn`) |
| `lmb` syntax complexity | Medium | Restrict lambda to single-expression body for v0.5 |
| `par` / `spwn` introduce non-determinism | High | Defer to v1.0; require explicit `join` before reading results |
| Adding too many keywords dilutes the minimal identity | High | Apply "NEVER add" list below before each release |
| `try`/`ctch` nesting makes error flow hard to trace | Low | Limit try/catch to one level deep in v0.5 |

---

## Features to NEVER Add

> These would turn Trionary into a copy of Python/JavaScript and violate its minimal philosophy.

- `class` / `struct` / `object` — no OOP; use functions and maps instead
- `import *` — wildcard imports; always explicit
- `goto` / `jmp` / `lbl` — no unstructured jumps
- `switch` / `case` — use `if`/`elif`/`els` chains
- `async`/`await` — use `spwn`/`join` pipeline model instead
- `macro` / `#define` — no preprocessor
- Operator overloading — keep `+`, `-`, `*`, `/` numeric-only
- Implicit type coercion — all conversions must be explicit (`num`, `tostr`)
- Semicolons — Trionary is newline-delimited; no optional terminators
- Multiple return values via tuple unpacking — too implicit

---

## Final Recommendation

**Direction: Minimal-first, extended by stages.**

Trionary's identity is a *pipeline language* — readable, sequential, and numeric at its core.

1. **Ship v0.4** with the foundation keywords (`if`, `for`, `str`, `arr`) to make day-to-day scripting viable without changing the pipeline model.
2. **Evaluate v0.5** additions only after v0.4 is stable and community feedback is collected. Add only the features that cannot be expressed cleanly with existing pipelines.
3. **Gate v1.0** features behind a compiler flag or `use experimental` module to keep the default surface area small.
4. **Reject** any feature that requires more than two lines of Trionary to explain to a beginner.

> *"A language that fits in your head is more powerful than one that doesn't."*
