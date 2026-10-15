# S-02 — File I/O: Create, Write, Seek, Read, Close, and Delete

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L3-03`

## Goal

Use Trap #15 file tasks (50–57) to create a binary file, write data, rewind the file pointer, read data back into memory, close, and delete.

## New Concepts

- Task 51 (`OpenFile`) and Task 52 (`NewFile`) return a File ID (`FID`) in `D1.L`
- Task 54 (`WriteFile`) writes `D2.L` bytes from `(A1)` to `FID` in `D1.L`
- Task 55 (`PositionFile`) seeks `FID` in `D1.L` to byte offset `D2.L`
- Task 53 (`ReadFile`) reads up to `D2.L` bytes into `(A1)` and returns actual bytes read in `D2.L`
- Task 56 (`CloseFile`) and Task 57 (`DeleteFile`)

## Predict (Before You Run)

After writing the 4 bytes `'68K!'` (`$36384B21`) to a new file, seeking to offset 0, and reading 4 bytes into `$2000`, what longword is in `D3`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-02-file-io`.
- Inspect `D0.W` after each Trap #15 file call (`0` = `F_SUCCESS`).
- Inspect memory at `$2000` (`36 38 4B 21`) and `D3` (`$36384B21`).

## Explain

Writing advances the file position to offset 4. Task 55 (`PositionFile` with `D2 = 0`) rewinds the stream to the beginning so task 53 can read back the 4 bytes.

## Common Mistakes

- Forgetting to save the `FID` returned in `D1.L` by task 51/52.
- Reading immediately after writing without seeking back to offset 0 (which hits EOF).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Create `ex_tmp.bin` (task 52), write the longword `$CAFEBABE` from `DATA`, seek to 0, read it back into `$2000`, close (task 56), delete `ex_tmp.bin` (task 57), and load `($2000)` into `D0`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-12-memory-map` for how file reads interact with ROM and read-only memory ranges.
