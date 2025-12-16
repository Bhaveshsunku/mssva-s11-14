# MSSVA Security Investigation - Submission Guide

**Lab:** Software Security Lab — HPRCSE  
**Role:** Security Researcher  
**Target:** dataproc-agent  

---

## Overview

Conduct a **design-aware security investigation** (not CTF, not exploitation).

**Goal:** Identify exactly **5 security-relevant insights** (Flags)

---

## Required Flags (ALL Mandatory)

| # | Flag | Description |
|---|------|-------------|
| 1 | **Design Assumption Broken** | Implicit assumption that fails under realistic conditions |
| 2 | **Telemetry Gap Identified** | Failure/behavior without sufficient logging or visibility |
| 3 | **Memory Ownership Violation** | Incorrect handling of allocation, lifetime, or ownership |
| 4 | **Silent Failure Detected** | Failure that doesn't crash/log/alert but causes incorrect behavior |
| 5 | **Fuzzer-Only Bug Explained** | Bug realistically exposed only through fuzzing/abnormal input |

⚠️ **Missing any flag = submission not ranked**

---

## Required Actions

✅ **Static Analysis** - Understand internal design and assumptions  
✅ **Dynamic Analysis** - Test with crafted inputs  
✅ **Instrumentation** - Add logging/assertions/counters (reveal, don't fix)  
✅ **Memory Analysis** - Track allocations, ownership, sanitizers  
✅ **Fuzzing** - Identify fuzzer-only bugs  

---

## Instrumentation Rules

**Allowed:**
- Logging (stderr/stdout)
- Assertions
- Counters/metrics
- Sanitizers (ASan, UBSan, MSan)
- Lightweight tracing

**Rules:**
- ✅ Instrumentation reveals, NOT fixes
- ✅ All instrumentation documented
- ✅ Vulnerabilities remain present

---

## Repository Structure (Strict)

```
mssva-day4-s11-14/
├── README.md
├── investigation/
│   ├── system-overview.md       # Architecture analysis
│   ├── findings.md              # 5 findings (THIS IS KEY)
│   ├── telemetry-gaps.md        # Logging analysis
│   ├── fuzzing-notes.md         # Fuzzing methodology
│   └── reflection.md            # Investigation process
└── evidence/
    ├── static/                  # Code analysis artifacts
    ├── dynamic/                 # Runtime test results
    └── fuzzing/                 # Fuzzer inputs/outputs
```

---

## Finding Format (Exact - Required for Each)

```markdown
## Finding N

**Title:** <Descriptive title>
**Flag:** <One of the 5 mandatory flags>
**Location (file:function):** <specific file and function>
**Instrumentation Used:** <what you added to observe>
**Trigger Condition:** <how to reproduce>
**Root Cause:** <why it happens>
**Impact:** <security implications>
**Evidence:** <concrete proof>
```

---

## Evidence Requirements (At Least 1)

Your submission MUST include at least one:

- ✅ Sanitizer error signature (exact message)
- ✅ Fuzzer crash input hash (SHA1/SHA256)
- ✅ Deterministic log line from your instrumentation
- ✅ Reproducible assertion failure

**This must be in the Evidence section of findings.md**

---

## Example Finding (Template)

```markdown
## Finding 1

**Title:** Design Assumption Broken  
**Flag:** Design Assumption Broken  
**Location (file:function):** record.c:parse_records()  
**Instrumentation Used:** Runtime logging to stderr  
**Trigger Condition:** Partial read during payload parsing  

**Root Cause:**  
The parser assumes fread() always reads requested bytes. This fails when 
input is truncated/corrupted. Parser allocates based on declared length 
but doesn't verify actual bytes read.

**Impact:**  
Uninitialized memory treated as valid payload data, leading to corrupted 
processing and potential out-of-bounds access.

**Evidence:**  
Deterministic log from instrumentation:
```
[FUZZER_BUG] Record 0: partial read (expected=10, got=5)
```
This proves the design assumption is broken - parser expected 10 bytes 
but got 5, yet continued without detecting mismatch.
```

---

## Evaluation Criteria

✅ Correct flag identification  
✅ Quality of instrumentation  
✅ Depth of root-cause analysis  
✅ Clarity of reporting  

---

## NOT Allowed

❌ Exploit development  
❌ Removing or fixing vulnerabilities  
❌ Tool output without explanation  
❌ Generic vulnerability descriptions  

---

## Submission Steps

1. **Fork** the repository (must be public)

2. **Complete investigation** in your fork:
   - Add instrumentation (don't modify core logic)
   - Identify all 5 flags
   - Document in `investigation/findings.md`

3. **Create submission tag:**
   ```bash
   git tag submission-mssva
   git push origin submission-mssva
   ```

4. **Only the tagged snapshot is evaluated**

---

## Quick Checklist

- [ ] All 5 flags identified and documented
- [ ] Each finding follows exact format
- [ ] Instrumentation added and documented
- [ ] Evidence includes at least one verification element
- [ ] Repository structure matches requirements
- [ ] Vulnerabilities preserved (not fixed)
- [ ] Git tag created: `submission-mssva`

---

## Key Principle

**Security is not a black-box toolchain.**

This evaluates:
- Understanding software internals
- Adding visibility
- Reasoning about failure
- Communicating security risk

---

**Remember:** Design-aware analysis, not exploitation. Reveal, don't fix.
