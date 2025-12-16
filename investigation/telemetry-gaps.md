# Telemetry Gaps Analysis

## Overview

This document identifies logging and observability gaps in the dataproc-agent system that prevent effective monitoring, debugging, and security incident response.

## Gap #1: Validation Failure Context Loss

### Current Behavior
```c
// main.c lines 37-39
if (!validate_record(&records[i]))
    continue;  // Silent skip, no logging
```

### Missing Information
- **Record index** that failed validation
- **Validation failure reason** (which check failed)
- **Record metadata** (type, length) for debugging
- **Timestamp** of failure
- **Cumulative context** (how many failures in this batch)

### Impact
- Cannot trace rejected records back to source data
- Impossible to identify systematic data quality issues
- No audit trail for compliance
- Debugging requires code modification and redeployment

### Recommended Instrumentation
```c
if (!validate_record(&records[i])) {
    sec_warn("record_validation_failed", 
             "index=%u type=%u length=%u", 
             i, records[i].type, records[i].length);
    continue;
}
```

---

## Gap #2: Partial Read Detection

### Current Behavior
```c
// record.c lines 17, 23 - no return value checking
fread(&records[i].length, 2, 1, fp);  // Unchecked
fread(records[i].payload, 1, records[i].length, fp);  // Unchecked
```

### Missing Information
- **Bytes requested** vs **bytes actually read**
- **EOF or error** condition
- **Which record** experienced partial read
- **File position** at failure

### Impact
- Partial reads accepted as complete data
- Uninitialized memory used without detection
- No differentiation between:
  - Truncated file (storage error)
  - Corrupted file (bit errors)
  - Malformed file (attacker crafted)

### Recommended Instrumentation
```c
size_t actual = fread(&records[i].length, 2, 1, fp);
if (actual != 1) {
    sec_error("partial_length_read",
              "record=%u expected=1 actual=%zu pos=%ld",
              i, actual, ftell(fp));
    break;
}
```

---

## Gap #3: Memory Allocation Lifecycle

### Current Behavior
```c
// main.c lines 40-42
char *out = process_record(&records[i], cfg.flags);
if (i % 2 == 0)
    free(out);  // No logging
```

### Missing Information
- **Which allocations** are freed vs leaked
- **Pointer values** for leak tracking
- **Allocation size** per record
- **Cumulative memory usage** tracking
- **Leak detection** without external tools

### Impact
- Memory leaks invisible until OOM
- Cannot correlate leaks to specific input patterns
- No early warning before crash
- Requires external leak detector (ASan, Valgrind)

### Recommended Instrumentation
```c
char *out = process_record(&records[i], cfg.flags);
sec_trace("allocation", "record=%u ptr=%p", i, (void*)out);

if (i % 2 == 0) {
    sec_trace("free", "record=%u ptr=%p", i, (void*)out);
    free(out);
} else {
    sec_warn("leak", "record=%u ptr=%p (odd index)", i, (void*)out);
}
```

---

## Gap #4: Bounds Violation Detection

### Current Behavior
```c
// parser.c - no validation against MAX_RECORDS
header_t parse_header(FILE *fp) {
    // ... reads record_count directly
    return hdr;  // No bounds check logged
}
```

### Missing Information
- **When design limits are exceeded**
- **By how much** (1025 vs 65535 very different)
- **Frequency** of violations
- **Correlation** with other anomalies

### Impact
- Design assumptions violated silently
- Capacity planning based on wrong assumptions
- No alerts when system used outside design parameters

### Recommended Instrumentation
```c
sec_log("record_count", hdr.record_count);
if (hdr.record_count > MAX_RECORDS) {
    sec_warn("bounds_violation",
             "record_count=%u exceeds MAX_RECORDS=%u",
             hdr.record_count, MAX_RECORDS);
}
```

---

## Gap #5: Configuration Visibility

### Current Behavior
```c
// config.c - only logs when FAST_MODE enabled
if (env && env[0] == '1') {
    cfg.flags |= FAST_MODE;
    sec_info("FAST_MODE enabled");  // Good!
}
// But no logging when FAST_MODE NOT enabled
```

### Missing Information
- **Negative confirmation** (what mode are we in?)
- **Environment variable values** checked
- **Configuration source** (env var vs default vs file)
- **Configuration changes** between runs

### Impact
- Cannot confirm system is in expected mode
- Difficult to diagnose behavior differences between environments
- No audit trail of configuration used

### Recommended Instrumentation
```c
const char *env = getenv("DATAPROC_FAST");
sec_log("config_check", "DATAPROC_FAST=%s", env ? env : "<unset>");

cfg.flags = 0;
if (env && env[0] == '1') {
    cfg.flags |= FAST_MODE;
    sec_info("FAST_MODE enabled");
} else {
    sec_info("FAST_MODE disabled (using safe defaults)");
}
```

---

## Gap #6: Error Path Visibility

### Current Behavior
```c
// record.c - break on malloc failure, no context
if (!records[i].payload)
    break;  // Which record? Why failed?
```

### Missing Information
- **Record index** at failure
- **Requested size** that failed to allocate
- **System state** (available memory, process limit)
- **Previous successful allocations** in this batch

### Impact
- Cannot determine why allocation failed
- No record of how far parsing progressed
- Incomplete data processed without indication

### Recommended Instrumentation
```c
records[i].payload = malloc(records[i].length);
if (!records[i].payload) {
    sec_error("malloc_failed",
              "record=%u size=%u errno=%d",
              i, records[i].length, errno);
    break;
}
```

---

## Telemetry Architecture Recommendations

### Logging Levels

Implement hierarchical logging:

```c
typedef enum {
    LOG_TRACE,   // Fine-grained debugging (per-byte)
    LOG_DEBUG,   // Function entry/exit, allocations
    LOG_INFO,    // Normal operations, config
    LOG_WARN,    // Recoverable errors, anomalies
    LOG_ERROR,   // Failures, corruptions
    LOG_FATAL    // Unrecoverable errors
} log_level_t;
```

### Structured Logging

Use key=value format for machine parsing:

```c
// Current: sec_log("record_count", hdr.record_count);
// Better:  
sec_log(LOG_INFO, "component=parser event=header_parsed "
        "record_count=%u version=%u flags=0x%02x",
        hdr.record_count, hdr.version, hdr.flags);
```

### Context Propagation

Add request ID for correlation:

```c
// Generate at start
uuid_t request_id = generate_uuid();
sec_log(LOG_INFO, "request_id=%s event=processing_start file=%s",
        request_id, argv[1]);

// Include in all subsequent logs
sec_log(LOG_WARN, "request_id=%s event=validation_failed record=%u",
        request_id, i);
```

### Metrics Collection

Beyond logging, track:

```c
typedef struct {
    uint64_t records_processed;
    uint64_t records_invalid;
    uint64_t bytes_read;
    uint64_t malloc_calls;
    uint64_t free_calls;  // Should equal malloc_calls!
    uint64_t partial_reads;
    uint64_t bounds_violations;
} metrics_t;
```

---

## Priority Matrix

| Gap | Severity | Effort | Priority |
|-----|----------|--------|----------|
| Validation failure context | High | Low | P0 |
| Partial read detection | High | Low | P0 |
| Bounds violation detection | Medium | Low | P1 |
| Memory lifecycle tracking | Medium | Medium | P1 |
| Configuration visibility | Low | Low | P2 |
| Error path visibility | Medium | Low | P1 |

---

## Testing Telemetry Improvements

After instrumentation, verify:

1. **Coverage:** Every error path logs
2. **Context:** Logs include sufficient debug info
3. **Performance:** Logging overhead acceptable (<5%)
4. **Volume:** Log volume manageable in production
5. **Parsability:** Logs are machine-readable
6. **Actionability:** Logs enable root cause identification

## Conclusion

Current telemetry is **minimal and inconsistent**:
- ✓ Some events logged (config, header)
- ✗ Most failures silent
- ✗ No context in logs
- ✗ Cannot debug production issues
- ✗ No audit trail

Adding instrumentation per above recommendations would:
- Enable production debugging
- Provide security audit trail
- Detect anomalies early
- Support incident response
- **Reveal bugs without fixing them** (per lab requirements)
