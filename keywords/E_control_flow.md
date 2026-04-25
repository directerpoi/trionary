# E. Control Flow

> Supplements the existing pipeline flow with explicit branching and loop control.
> Branching keywords (`if`, `els`, `elif`) and loop-control keywords (`brk`, `nxt`) are
> defined in [A_core_language.md](A_core_language.md) (A01–A03, A07–A08).

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| E01 | `ext` | keyword | Exit the program with a status code | `ext 0` | v0.4 |
| E02 | `stp` | keyword | Stop the program immediately (exit 1) | `stp` | v0.4 |
| E03 | `yld` | keyword | Yield a value from a pipeline stage | `yld x * 2` | v1.0 |
