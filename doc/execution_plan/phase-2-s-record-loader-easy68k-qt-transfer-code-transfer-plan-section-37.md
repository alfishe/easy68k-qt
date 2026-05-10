## Phase 2: S-Record Loader — EASy68K-qt Transfer (Code Transfer Plan Section 3.7)

### Task 2.1 — Transfer and Extract SRecordLoader

Source: `EASy68K-qt/src/core/simulator/srecord_loader.cc`
Target: `include/easym68k/sim/srecord_loader.h` + `src/sim/srecord_loader.cc`

Steps:

1. Create new header with standalone class:
   ```cpp
   class SRecordLoader {
    public:
     struct LoadResult {
       bool success;
       uint32_t start_address;
       bool has_start_address;
       std::string error_message;
       int error_line;
     };

     LoadResult LoadFromFile(const std::string& filename, Memory& memory);
     LoadResult LoadFromStream(std::istream& stream, Memory& memory);
     LoadResult LoadFromLines(const std::vector<std::string>& lines, Memory& memory);
   };
   ```
2. Copy the hex parsing helpers (`HexCharToInt`, `ParseHexByte`, `ParseHexValue`)
3. Add checksum validation:
   - Accumulate all bytes (byte count + address bytes + data bytes + checksum byte)
   - Verify `accumulated & 0xFF == 0xFF` (one's complement property)
   - On failure, set `LoadResult.success = false` with line number and error message
4. Change from writing to `memory_` member to writing through `Memory&` parameter
5. Return structured `LoadResult` instead of `bool`
6. Add `LoadFromStream` overload
7. Run `clang-format`

**Quality Gate 2.1:**
- SRecordLoader compiles with no dependency on Cpu or Simulator classes
- `srecord_loader_test.cc` passes with valid S1/S2/S3/S7/S8/S9 records
- Checksum validation correctly rejects corrupted records
- Error messages include line numbers

---

### Task 2.2 — S-Record Loader Tests

Source: `EASy68K-qt/tests/simulator/srecord_test.cc` → Target: `tests/sim/srecord_loader_test.cc`

Steps:

1. Port existing tests
2. Add new tests:
   - `ChecksumValid` — known-good S-Record with correct checksum
   - `ChecksumInvalid` — same record with corrupted checksum byte → `success == false`
   - `S1Record` — 16-bit address data record
   - `S2Record` — 24-bit address data record
   - `S3Record` — 32-bit address data record
   - `S7StartAddress` — 32-bit start address
   - `S8StartAddress` — 24-bit start address
   - `S9StartAddress` — 16-bit start address
   - `EmptyFile` — returns error
   - `MissingSRecordHeader` — lines without 'S' prefix are skipped
   - `TruncatedRecord` — byte count exceeds available hex digits → error
   - `MultipleDataRecords` — sequential S1 records load to correct addresses
   - `LoadFromStream` — `std::istringstream` input works

**Quality Gate 2.2:** All 14 S-Record tests pass. No ASan errors.

---
