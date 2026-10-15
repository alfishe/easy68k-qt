# L2-03 — EASy68K Structured Control Flow: IF, WHILE, REPEAT, FOR, DBLOOP

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L2-02

## Goal

Use EASy68K's built-in structured control statements (`IF`/`ELSE`/`ENDI`, `WHILE`/`ENDW`, `FOR`/`ENDF`, `REPEAT`/`UNTIL`, `DBLOOP`/`UNLESS`) and inspect the branches they generate in `.L68`.

## New concepts

- `IF.L D0 <GT> #10 THEN.S ... ELSE.S ... ENDI` emits `CMP` + `Bcc` + `BRA` automatically.
- `FOR.L D1 = #1 TO #5 DO.S ... ENDF` emits an initialized loop that increments `D1` through 5.
- Enable `Expand Structured Statements` (`OPT SEX`) to see every generated `CMP` and branch in the `.L68` listing.

## Predict

Before running, write down your answers:

1. What value is in `D0` after `FOR.L D1 = #1 TO #5 DO.S` adds `D1` to `D0`?
2. Which branch of `IF.L D0 <EQ> #15 THEN.S` executes?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `.L68` in Edit68K-Qt or Source View in Sim68K-Qt to see the auto-generated labels (`_00000001`, etc.) and branch instructions.

## Explain

EASy68K's structured directives are zero-overhead syntactic sugar: the assembler expands them directly into standard `CMP`/`Bcc`/`DBcc` instructions with unique internal labels.

## Common mistakes

- Forgetting the angle brackets around relational operators (`<EQ>`, `<NE>`, `<LT>`, `<LE>`, `<GT>`, `<GE>`, `<HI>`, `<LS>`, `<CC>`, `<CS>`).

## Exercises

1. ●● Use `REPEAT ... UNTIL.L D0 <GE> #10 DO.S` starting from `D0 = 0` and adding `3` each iteration so `D0` finishes at `12`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-04 (`subroutines`).
