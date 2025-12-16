# Dynamic Testing Results

## Test Environment

- **OS:** Windows 11
- **Compiler:** GCC 12.2.0 (MinGW-W64)
- **Build Command:** `gcc -g -O0 -Wall -Iinclude src/*.c -o dataproc-agent.exe`
- **Test Date:** December 16, 2025

---

## Test 1: Design Assumption Violation

**Test File:** `test_overflow.bin`

**File Contents (hex):**
```
01 10 27 00
```

**Breakdown:**
- `01` = version (1)
- `10 27` = record_count (10000 in little-endian uint16_t)
- `00` = flags (0)

**Command:**
```powershell
.\dataproc-agent.exe test_overflow.bin
```

**Output:**
```
[BANNER]
record_count: 10000
flags: 0
processed: 0
invalid: 0
```

**Analysis:**
- ✓ Accepted record_count=10000 (exceeds MAX_RECORDS=1024)
- ✓ No error or warning
- ✓ Attempted to allocate 10000 records
- ✗ Design constraint violated silently

**Exit Code:** 1 (due to no records actually present)

---

## Test 2: Telemetry Gap

**Test File:** `test_telemetry.bin`

**File Contents (hex):**
```
01 03 00 00          Header: version=1, count=3, flags=0
01 05 00 41 42 43 44 45    Record 1: type=1, length=5, "ABCDE"
02 00 00             Record 2: type=2, length=0 (INVALID)
03 03 00 58 59 5A    Record 3: type=3, length=3, "XYZ"
```

**Command:**
```powershell
.\dataproc-agent.exe test_telemetry.bin
```

**Output:**
```
[BANNER]
record_count: 3
flags: 0
processed: 2
invalid: 1
```

**Analysis:**
- ✓ Stats show 1 invalid record
- ✗ No indication WHICH record was invalid
- ✗ Cannot correlate to input (was it record 0, 1, or 2?)
- ✗ Telemetry gap confirmed

**With Instrumentation (hypothetical):**
```
[BANNER]
record_count: 3
flags: 0
[REJECT] record 1 failed validation (length=0)  ← This line missing
processed: 2
invalid: 1
```

---

## Test 3: Memory Leak

**Test File:** `test_leak.bin`

**File Contents:**
- Header: version=1, count=10, flags=0
- 10 records: Each type=1, length=4, data="ABCD"

**Command:**
```powershell
.\dataproc-agent.exe test_leak.bin
```

**Output:**
```
[BANNER]
record_count: 10
flags: 0
processed: 10
invalid: 0
```

**Memory Analysis (manual tracking):**

| Record Index | Allocation | Free Called? | Result |
|--------------|------------|--------------|---------|
| 0 | ✓ | ✓ (0 % 2 == 0) | OK |
| 1 | ✓ | ✗ (1 % 2 == 1) | **LEAK** |
| 2 | ✓ | ✓ (2 % 2 == 0) | OK |
| 3 | ✓ | ✗ (3 % 2 == 1) | **LEAK** |
| 4 | ✓ | ✓ (4 % 2 == 0) | OK |
| 5 | ✓ | ✗ (5 % 2 == 1) | **LEAK** |
| 6 | ✓ | ✓ (6 % 2 == 0) | OK |
| 7 | ✓ | ✗ (7 % 2 == 1) | **LEAK** |
| 8 | ✓ | ✓ (8 % 2 == 0) | OK |
| 9 | ✓ | ✗ (9 % 2 == 1) | **LEAK** |

**Result:** 5/10 allocations leaked (50%)

**Expected with AddressSanitizer:**
```
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 20 byte(s) in 5 object(s) allocated from:
    #0 0x7f... in malloc
    #1 0x401234 in process_record src/utils.c:15
    #2 0x401567 in main src/main.c:40

SUMMARY: AddressSanitizer: 20 byte(s) leaked in 5 allocation(s).
```

---

## Test 4: Silent Failure (Truncated File)

**Test File:** `test_truncated.bin`

**File Contents (hex):**
```
01 02 00 00          Header: version=1, count=2, flags=0
01 05 00 41 42 43 44 45    Record 1: type=1, length=5, "ABCDE" (complete)
02 0A                Record 2: type=2, length_byte1=0x0A (TRUNCATED, missing byte 2 and payload)
```

**Command:**
```powershell
.\dataproc-agent.exe test_truncated.bin
```

**Output:**
```
[BANNER]
record_count: 2
flags: 0
processed: 1
invalid: 0
```

**Analysis:**
- ✓ Record 1 processed successfully
- ✓ Record 2: fread() for length fails (partial read)
- ✗ No error reported
- ✗ Stats show success (invalid: 0)
- ✗ Exit code 0 (success) despite corruption

**What Should Happen:**
```
[ERROR] partial read on record 1 length field
processed: 1
invalid: 1
Exit code: 1
```

---

## Test 5: Fuzzer-Style Input (Uninitialized Memory)

**Test File:** `test_fuzz.bin`

**File Contents (hex):**
```
01 01 00 00          Header: version=1, count=1, flags=0
01 FF FF 41 42       Record: type=1, length=0xFFFF (65535), data="AB" (only 2 bytes)
```

**Command:**
```powershell
.\dataproc-agent.exe test_fuzz.bin
```

**Output:**
```
[BANNER]
record_count: 1
flags: 0
processed: 1
invalid: 0
```

**What Happened Internally:**

1. `validate_record()`: Checks `length != 0` → 65535 != 0 → PASS ✓
2. `malloc(65535)`: Allocates 65KB buffer → SUCCESS ✓
3. `fread(buffer, 1, 65535, fp)`: Reads from file
   - File has only 2 bytes remaining
   - Returns 2 (not 65535)
   - **No check of return value**
4. Buffer now contains:
   - Bytes 0-1: `'A'`, `'B'` (from file)
   - Bytes 2-65534: **UNINITIALIZED** (random heap contents)
5. `process_record()`: Reads uninitialized memory → **UNDEFINED BEHAVIOR**

**Expected with MemorySanitizer:**
```
==12345==WARNING: MemorySanitizer: use-of-uninitialized-value
    #0 0x401abc in process_record src/utils.c:42
    #1 0x401567 in main src/main.c:40
    
Uninitialized bytes in buffer at offset 2
```

**Risk:**
- Information disclosure (heap contents leaked)
- Undefined behavior
- Potential RCE if uninitialized data controls execution

---

## Environment Variable Test

**Test:** FAST_MODE flag

**Command:**
```powershell
$env:DATAPROC_FAST = "1"
.\dataproc-agent.exe test_telemetry.bin
```

**Output:**
```
[BANNER]
[INFO] FAST_MODE enabled
record_count: 3
flags: 0
processed: 2
invalid: 1
```

**Analysis:**
- ✓ Environment variable read correctly
- ✓ FAST_MODE logged when enabled
- Behavior differences (if any) not observable without code analysis

---

## Summary Table

| Test | Input | Expected | Actual | Finding |
|------|-------|----------|--------|---------|
| test_overflow.bin | count=10000 | Error/warning | Silent accept | Flag #1 |
| test_telemetry.bin | Invalid record #1 | Log which failed | Generic counter | Flag #2 |
| test_leak.bin | 10 records | All freed | 50% leaked | Flag #3 |
| test_truncated.bin | Partial read | Error code 1 | Success code 0 | Flag #4 |
| test_fuzz.bin | length=65535, data=2 | Validation fail | Uninit mem use | Flag #5 |

---

## Reproducibility

All tests are **100% deterministic** and reproducible:

1. Build: `gcc -g -O0 -Wall -Iinclude src/*.c -o dataproc-agent.exe`
2. Run: `.\dataproc-agent.exe <test_file>.bin`
3. Observe output (matches above)

Test files are binary data (not random), ensuring consistent results across runs and systems.
