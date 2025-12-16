# Static Analysis Evidence

## MAX_RECORDS Definition

**File:** `include/common.h`  
**Line:** 9

```c
#define FAST_MODE 0x1
#define MAX_RECORDS 1024
```

**Analysis:**
- Hardcoded design limit: 1024 records maximum
- Used nowhere in runtime code
- Parser accepts uint16_t (0-65535) without checking against this limit

---

## Parser Missing Bounds Check

**File:** `src/parser.c`  
**Function:** `parse_header()`

```c
header_t parse_header(FILE *fp) {
    header_t hdr;

    fread(&hdr.version, 1, 1, fp);
    fread(&hdr.record_count, 2, 1, fp);  // ← Accepts any value 0-65535
    fread(&hdr.flags, 1, 1, fp);

    sec_log("record_count", hdr.record_count);
    sec_log("flags", hdr.flags);

    return hdr;  // ← No validation against MAX_RECORDS
}
```

**Search Command:**
```powershell
Get-Content src/parser.c | Select-String "MAX_RECORDS"
# Result: No matches
```

**Finding:** Parser never references MAX_RECORDS constant.

---

## Conditional Free Bug

**File:** `src/main.c`  
**Lines:** 40-42

```c
for (uint16_t i = 0; i < hdr.record_count; i++) {
    if (!validate_record(&records[i]))
        continue;

    char *out = process_record(&records[i], cfg.flags);
    if (out && out[0] == 'X') {
        // legacy behavior
    }

    if (i % 2 == 0)      // ← Only even indices
        free(out);        // ← Memory leak for odd indices

    stats_inc_records();
}
```

**Analysis:**
- `process_record()` returns allocated memory
- `free()` only called when `i % 2 == 0`
- Indices 0, 2, 4, 6, 8... freed
- Indices 1, 3, 5, 7, 9... leaked
- Comment on line 42: `// ownership ambiguity remains`

---

## Unchecked fread() Calls

**File:** `src/record.c`  
**Function:** `parse_records()`  
**Lines:** 13-24

```c
for (uint16_t i = 0; i < count; i++) {
    if (fread(&records[i].type, 1, 1, fp) != 1)  // ← CHECKED
        break;

    fread(&records[i].length, 2, 1, fp);         // ← UNCHECKED (line 17)

    records[i].payload = malloc(records[i].length);
    if (!records[i].payload)
        break;

    fread(records[i].payload, 1, records[i].length, fp);  // ← UNCHECKED (line 23)
}
```

**Analysis:**
- Line 13: Type read IS checked (good)
- Line 17: Length read NOT checked (if fails, uninitialized length used)
- Line 23: Payload read NOT checked (if partial, uninitialized memory in buffer)

**Grep Results:**
```powershell
Get-Content src/record.c | Select-String "fread" | Select-Object LineNumber,Line

# LineNumber Line
# ---------- ----
#         13 if (fread(&records[i].type, 1, 1, fp) != 1)
#         17 fread(&records[i].length, 2, 1, fp);
#         23 fread(records[i].payload, 1, records[i].length, fp);
```

---

## Missing Upper Bound Validation

**File:** `src/validate.c`  
**Function:** `validate_record()`  
**Lines:** 4-13

```c
int validate_record(record_t *rec) {
    if (!rec || !rec->payload)
        return 0;

    if (rec->length == 0) {           // ← Only checks for ZERO
        stats_inc_invalid();
        return 0;
    }

    return 1; // does NOT validate upper bounds  ← Comment admits issue
}
```

**Analysis:**
- Checks `length == 0` (lower bound)
- No check for `length > MAX_PAYLOAD_SIZE` (upper bound)
- Accepts 65535 (0xFFFF) as valid
- Comment explicitly states: "does NOT validate upper bounds"

---

## Silent Validation Failure

**File:** `src/main.c`  
**Lines:** 37-39

```c
for (uint16_t i = 0; i < hdr.record_count; i++) {
    if (!validate_record(&records[i]))
        continue;  // ← Silent skip, no logging of which record failed

    char *out = process_record(&records[i], cfg.flags);
    // ...
}
```

**Analysis:**
- `continue` skips record without logging
- No indication of:
  - Which record index failed
  - Why validation failed
  - Record metadata (type, length)
- Telemetry gap: cannot debug validation failures

---

## Code Statistics

```powershell
# Count lines of code
Get-ChildItem src/*.c | ForEach-Object {
    $lines = (Get-Content $_.FullName).Count
    Write-Host "$($_.Name): $lines lines"
}

# Results:
# config.c: 15 lines
# main.c: 53 lines
# memory.c: 14 lines
# parser.c: 15 lines
# record.c: 26 lines
# stats.c: 24 lines
# telemetry.c: 19 lines
# utils.c: 22 lines
# validate.c: 13 lines
```

**Total:** ~200 lines of code (small codebase, high bug density)

---

## Compiler Warnings

```powershell
gcc -g -O0 -Wall -Wextra -Iinclude src/*.c -o dataproc-agent.exe

# No warnings generated
# (Bugs are logic errors, not syntax/type errors)
```

**Analysis:** Modern compilers cannot detect these bugs:
- Design assumption violations (MAX_RECORDS)
- Ownership errors (conditional free)
- Missing return value checks (unchecked fread)
- Validation logic gaps (no upper bounds)

These require **semantic analysis**, not just compilation.

---

## Dependency Analysis

```bash
# No external dependencies
# Only stdlib: stdio.h, stdlib.h, string.h, stdint.h
```

**Finding:** Simple codebase, no complex dependencies, yet multiple security issues.

**Insight:** Complexity is not required for vulnerabilities. Even 200 lines can have critical bugs if assumptions are wrong.

---

## Summary of Static Findings

| Issue | File | Line(s) | Type |
|-------|------|---------|------|
| MAX_RECORDS not enforced | parser.c | 10 | Design assumption |
| Conditional free() | main.c | 41 | Memory ownership |
| Unchecked fread (length) | record.c | 17 | Silent failure |
| Unchecked fread (payload) | record.c | 23 | Silent failure |
| No upper bound check | validate.c | 8-13 | Fuzzer bug |
| Silent validation skip | main.c | 38 | Telemetry gap |

All findings confirmed through static code review before dynamic testing.
