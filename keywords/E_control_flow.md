# E. Control Flow

> Supplements the existing pipeline flow with explicit branching and loop control.

| # | Name | Type | Description | Example Syntax | Priority |
|---|------|------|-------------|----------------|----------|
| E01 | `if` | keyword | Conditional branch (see A01) | `if x > 0` | v0.4 |
| E02 | `els` | keyword | Else branch (see A02) | `els` | v0.4 |
| E03 | `elif` | keyword | Else-if branch (see A03) | `elif x == 0` | v0.5 |
| E04 | `brk` | keyword | Break out of nearest loop (see A07) | `brk` | v0.4 |
| E05 | `nxt` | keyword | Continue to next iteration (see A08) | `nxt` | v0.4 |
| E06 | `ext` | keyword | Exit the program with a status code | `ext 0` | v0.4 |
| E07 | `stp` | keyword | Stop the program immediately (exit 1) | `stp` | v0.4 |
| E08 | `yld` | keyword | Yield a value from a pipeline stage | `yld x * 2` | v1.0 |
