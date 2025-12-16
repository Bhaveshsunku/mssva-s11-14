## Software Security Lab — HPRCSE
Role: Security Researcher Investigation Target: dataproc-agent Context

You are working as a Security Researcher in the Software Security Lab, HPRCSE.

Your task is to conduct a design-aware security investigation of the provided system, similar to how internal security teams analyze production software.

This is not a CTF and not an exploitation exercise.

## Core Task
You must investigate the target system and identify exactly five security-relevant insights, referred to as Flags.

To do this, you are required to:

Understand internal design and assumptions

Use static and dynamic analysis

Add instrumentation where visibility is missing

Use memory analysis and fuzzing

Explain why each issue matters

### Required Flags (ALL are mandatory)
You must find and report all five flags below:

### Design Assumption Broken
An implicit design or implementation assumption that fails under realistic conditions.

### Telemetry Gap Identified
A failure or abnormal behavior that occurs without sufficient logging or visibility.

### Memory Ownership Violation
Incorrect handling of allocation, lifetime, or ownership of memory.

### Silent Failure Detected
A failure that does not crash, log, or alert but leads to incorrect behavior.

### Fuzzer-Only Bug Explained
A bug realistically exposed only through fuzzing or abnormal input patterns.

Missing any flag = submission not ranked.

Instrumentation Requirement (Mandatory)
```text
You must add instrumentation to observe internal behavior.

Allowed:

Logging

Assertions

Counters / metrics

Sanitizers (ASan, UBSan)

Lightweight tracing
```

Rules:
Instrumentation must reveal, not fix

All instrumentation must be documented

Vulnerabilities must remain present
```text
Submission Structure (Strict)

mssva-day4-<s11-14>/
├── README.md
├── investigation/
│   ├── system-overview.md
│   ├── findings.md
│   ├── telemetry-gaps.md
│   ├── fuzzing-notes.md
│   └── reflection.md
└── evidence/
    ├── static/
    ├── dynamic/
    └── fuzzing/
```
Findings Format (Exact)

Your findings.md must contain exactly five findings, one per flag.

Each finding must follow:
```text
Title:
Flag:
Location (file:function):
Instrumentation Used:
Trigger Condition:
Root Cause:
Impact:
Evidence:
```

### Submission Mechanism (Required)
To verify authenticity, your submission must include at least one of the following:

A sanitizer error signature (exact message)

A fuzzer crash input hash

A deterministic log line produced by your instrumentation

A reproducible assertion failure

This must be referenced in the Evidence section.

Evaluation Basis

Submissions are evaluated on:

Correct flag identification

Quality of instrumentation

Depth of root-cause analysis

Clarity of reporting

Not Allowed
Exploit development

Removing or fixing vulnerabilities

Tool output without explanation

Generic vulnerability descriptions

Submission Instructions — MSSVA Security Investigation
This bootcamp follows a research snapshot submission model. Participants must not push code back to the original repository.

## Submission Steps

1. **Fork the Repository** 
   - Go to the main repository on GitHub and click **Fork** to create a copy under your account. 
   - Your fork **must be public** so the automated verification workflow can access it.

2. **Clone Your Fork** 
```bash
git clone <your-fork-url>
cd <repository-name>
```
3.
```bash
git checkout -b <your-username>-submission
```
4.
```text
Prepare Your Submission Folder

Create a folder named submission-mssva/ in the root of the repository.

Place all your files inside this folder:

submission.md with your findings

Any instrumentation or analysis files

Do not modify core application logic outside submission/
```
5.
```bash
git add submission-mssva/
git commit -m "Submission by <your-username>"
```
6.
```bash
git tag submission-<your-username>
git push origin submission-<your-username>
```
## @Final Note
Security is not a black-box toolchain.

This exercise evaluates how well you:

Understand software internals

Add visibility

Reason about failure

Communicate security risk
