# EASy68K-Qt Examples

A guided path through 68000 assembly language and C/C++ on the 68000, from
your first instruction to cycle-optimised code. Every example runs in
EASy68K-Qt as-is and is checked automatically by the test suite.

* Strategy and curriculum: [`doc/examples/EDUCATION_STRATEGY.md`](../doc/examples/EDUCATION_STRATEGY.md)
* Spec format for `example.toml`: [`doc/examples/example-spec.md`](../doc/examples/example-spec.md)

## Tracks

| Directory | Track | Levels | Status |
|-----------|-------|--------|--------|
| [`core/`](core/README.md) | Core path: orientation, data, control flow, data structures | 0–3 | planned (E2–E3) |
| [`systems/`](systems/README.md) | Track S: I/O, devices, exceptions, interrupts, protection | 4 | planned (E4) |
| [`c/`](c/README.md) | Track C: C and C++ on the 68000 | 4 | planned (E5–E6) |
| [`optimization/`](optimization/README.md) | Track O: naive vs optimised pairs with cycle budgets | 4 | planned (E7) |
| [`projects/`](projects/README.md) | Capstone projects | 4 | planned (E8) |

Start with `core/L0-01-hello-simhalt` and follow the `requires` links in each
example's README.

## Running an example

1. In **Edit68K**, open `main.x68` and assemble it (F9).
2. In **Sim68K**, open the generated `.S68` and Run, Step or Trace.
3. Follow the README's *Predict → Run & observe → Explain* sections.

Headless (CI or the command line), once E1 lands:

```bash
easy68k-run examples/core/L0-01-hello-simhalt   # runs and checks example.toml
```

## Layout of an example

```
<track>/<ID>-<name>/
  README.md      goal, prerequisites, concepts, predict/run/observe/explain, exercises
  main.x68       the program (main.c / main.cpp in Track C)
  example.toml   expected results and cycle budget
  exercises/     starter files + <name>.toml specs
  solutions/     reference solutions (verified by CI)
```

Copy [`_template/`](_template/) to start a new example.

## License

All files under `examples/` are dedicated to the public domain under
CC0 1.0 Universal — see [`LICENSE`](LICENSE). Use them however you like.
The EASy68K-Qt applications themselves are licensed under GPL-2.0-or-later (see the
repository's root `LICENSE`).
