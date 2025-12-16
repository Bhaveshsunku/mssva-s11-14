# Security Investigation Report: dataproc-agent

**Investigator:** Security Researcher  
**Lab:** Software Security Lab - HPRCSE  
**Target:** dataproc-agent  
**Date:** December 16, 2025

## Overview

This repository contains a comprehensive security investigation of the `dataproc-agent` system, an internal data preprocessing service used for batch ingestion. The investigation follows a design-aware approach focusing on understanding internal assumptions, identifying failure modes, and adding instrumentation to reveal security-relevant issues.

## Investigation Approach

1. **Static Analysis** - Code review to identify design assumptions and error handling gaps
2. **Dynamic Testing** - Crafted input files to trigger specific failure conditions
3. **Instrumentation** - Added logging and assertions to reveal hidden behaviors
4. **Fuzzing Analysis** - Identified bugs only realistically exposed through systematic fuzzing

## Key Findings

This investigation identified all five mandatory security flags:

1. **Design Assumption Broken** - MAX_RECORDS limit not enforced at runtime
2. **Telemetry Gap Identified** - Validation failures occur without per-record logging
3. **Memory Ownership Violation** - Conditional free() causes 50% memory leak
4. **Silent Failure Detected** - Unchecked fread() processes corrupted data
5. **Fuzzer-Only Bug Explained** - Large length values trigger uninitialized memory use

## Repository Structure

```
mssva-day4-s11-14/
├── README.md                    # This file
├── investigation/
│   ├── system-overview.md       # Architecture and design analysis
│   ├── findings.md              # Five mandatory security findings
│   ├── telemetry-gaps.md        # Logging and visibility gaps
│   ├── fuzzing-notes.md         # Fuzzing methodology and results
│   └── reflection.md            # Investigation process reflection
└── evidence/
    ├── static/                  # Static analysis artifacts
    ├── dynamic/                 # Runtime test results
    └── fuzzing/                 # Fuzzing inputs and results
```

## How to Review

1. Start with `investigation/system-overview.md` for context
2. Review `investigation/findings.md` for the five security flags
3. Examine evidence files to verify claims
4. Check instrumentation code in `investigation/` documentation

## Verification

All findings include:
- Exact file and function locations
- Reproducible trigger conditions
- Test case binaries in `evidence/`
- Instrumentation code documentation
- Root cause analysis

## Important Notes

- **No exploits developed** - This is security research, not exploitation
- **Vulnerabilities preserved** - Instrumentation reveals, does not fix
- **Focus on design** - Not just memory safety, but design assumptions and failure modes

## Next Steps

Review the detailed findings in `investigation/findings.md` and supporting documentation in other investigation files.
