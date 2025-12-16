cd "C:\Users\hp\workspace\mssva-s11-14"; Write-Host "Checking structure:" -ForegroundColor Cyan; Get-ChildItem -Path "mssva-day4-s11-14" -Recurse | Select-Object FullName | ForEach-Object { $_.FullName.Replace("C:\Users\hp\workspace\mssva-s11-14\mssva-day4-s11-14\", "") }# System Overview: dataproc-agent

## Purpose

`dataproc-agent` is an internal data preprocessing service designed for batch ingestion of proprietary format data files.

## Architecture

### Components

```
┌─────────────────────────────────────────────────────┐
│                   main.c                            │
│  (Orchestration, record processing loop)            │
└──────────┬──────────────────────────────────────────┘
           │
           ├─► config.c      (Environment-based configuration)
           ├─► parser.c      (Header and record parsing)
           ├─► record.c      (Binary record deserialization)
           ├─► validate.c    (Record validation logic)
           ├─► stats.c       (Statistics tracking)
           ├─► telemetry.c   (Logging infrastructure)
           └─► utils.c       (Processing utilities)
```

### Data Flow

1. **Configuration Loading** (`config.c`)
   - Reads `DATAPROC_FAST` environment variable
   - Sets `FAST_MODE` flag if enabled

2. **Header Parsing** (`parser.c`)
   - Reads 5-byte header: version (1), record_count (2), flags (1)
   - Logs record count and flags
   - Returns header structure

3. **Record Parsing** (`record.c`)
   - Allocates array for `record_count` records
   - For each record:
     - Read type (1 byte)
     - Read length (2 bytes)
     - Allocate payload buffer
     - Read payload data

4. **Record Processing Loop** (`main.c`)
   - Iterates through parsed records
   - Validates each record
   - Processes valid records
   - Conditionally frees memory (i % 2 == 0)
   - Updates statistics

5. **Statistics Reporting** (`stats.c`)
   - Tracks processed records
   - Counts invalid records
   - Dumps final statistics

## File Format

```
┌──────────────────────────────────────┐
│ HEADER (5 bytes)                     │
├──────────────────────────────────────┤
│ version     : uint8_t  (1 byte)      │
│ record_count: uint16_t (2 bytes, LE) │
│ flags       : uint8_t  (1 byte)      │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│ RECORD #1                            │
├──────────────────────────────────────┤
│ type    : uint8_t  (1 byte)          │
│ length  : uint16_t (2 bytes, LE)     │
│ payload : uint8_t[length]            │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│ RECORD #2                            │
├──────────────────────────────────────┤
│ ...                                  │
└──────────────────────────────────────┘
```

## Design Assumptions (Identified)

### Implicit Assumptions

1. **Trusted Input**
   - Assumes input files are pre-validated
   - No runtime bounds checking against `MAX_RECORDS` (1024)
   - Header values accepted without verification

2. **Complete Reads**
   - Assumes fread() always succeeds completely
   - No verification of bytes actually read
   - Expects files to be well-formed and complete

3. **Memory Management**
   - Assumes caller knows ownership semantics
   - `process_record()` ownership unclear (comment: "ownership ambiguity remains")
   - Conditional free() suggests unresolved design conflict

4. **Validation Sufficiency**
   - Assumes `length != 0` is sufficient validation
   - No upper bound checks on payload sizes
   - No verification that file contains claimed bytes

5. **Error Visibility**
   - Assumes failures will be observable
   - No per-record validation logging
   - Silent failures expected to be acceptable

## Key Constants

- `MAX_RECORDS = 1024` (common.h) - Design limit for record count
- `FAST_MODE = 0x1` (common.h) - Runtime configuration flag

## Build Configuration

### Standard Build
```makefile
CC=clang
CFLAGS=-g -O0 -Wall -Iinclude
```

### Sanitizer Build
```makefile
asan: $(CC) $(CFLAGS) -fsanitize=address src/*.c -o dataproc-agent
```

## Security-Relevant Observations

### Missing Checks
1. No validation that `record_count <= MAX_RECORDS`
2. No verification of fread() return values (except first type read)
3. No upper bounds on `length` field
4. No file size verification before allocation

### Error Handling Gaps
1. Partial reads processed as valid data
2. Validation failures not logged with context
3. Break statements in parse loop don't propagate errors
4. Success/failure indistinguishable at caller level

### Memory Management Issues
1. Conditional free() based on index parity (i % 2 == 0)
2. Explicit comment about ownership ambiguity
3. No cleanup on early exits from parse loop

## Evolution Notes

README states: "Program evolved over time"

This explains:
- Inconsistent error handling patterns
- Mixed ownership semantics
- Legacy behavior checks (e.g., `if (out && out[0] == 'X')`)
- Partial sanitizer support

## Threat Model (Inferred)

Based on "internal service" description:
- **Trusted network environment** (explains lack of input validation)
- **Pre-processed inputs** (explains assumption of valid data)
- **Batch processing** (explains statistics over detailed logging)
- **No formal security review** (per README notes)
