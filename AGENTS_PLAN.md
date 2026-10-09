# Freedom Voting Machine — AGENTS Plan

This document is the operational plan for AI agents working on the FVM.
Read it before writing any code.

## 1. What the FVM Is

The Freedom Voting Machine (FVM) is an all-contiguous, paper-trail voting
machine built on the ASCII Crabs stack machine. It is designed to be
**fully auditable**: every vote leaves a physical paper record, an
anonymous public ledger entry, and a blockchain hash. The voter can
verify their vote on the chain without revealing their identity.

### The Flow

```
Voter arrives with State-issued ID
    |
    v
[1] State scans ID into the blockchain cue (FIFO of pending voters)
    |
    v
[2] Voter makes their paper vote (pen and paper, or CrabsBed hardware)
    |
    v
[3] Machine anonymizes: pops token from cue, generates untraceable
    ballot token via one-way function. Original ID is discarded.
    |
    v
[4] Paper roll unrolls. Machine prints ONLY the voter's line to the
    public anon ledger. Other lines are already on the paper.
    |
    v
[5] Voter inspects their line. Privacy mask: the roll position shows
    only the voter's line. They cannot see other votes.
    |
    v
[6] Paper roll re-rolls. The paper is sealed in a privacy container.
    |
    v
[7] Vote is submitted to the blockchain (hash-chained, append-only).
    |
    v
[8] Triple count: in-memory tally, paper-text tally, blockchain tally.
    All three must AGREE. If not, the election is flagged.
    |
    v
[9] AI-assisted ledger verification (fraud detection, pattern analysis).
    |
    v
[10] Voter can look up their ballot on the chain using their anonymous
     token (the "future app" feature). The token is the only link
     between the voter and their vote — it cannot be reversed to
     identity.
```

### The Three Counts (Triple Count)

| Count | Source | Independence |
|-------|--------|--------------|
| 1 | In-memory `FvmTripleCount.in_memory[]` | Direct tally from ballot struct |
| 2 | Paper text `FvmLedgerTally()` | Re-parses the TEXT lines, not the struct |
| 3 | Blockchain `FvmChainTally` | Side-tally updated at add-time (weakest; QA: re-derive from stored block bytes) |

All three must agree. If any disagree, the result is `DISAGREE` and the
election is flagged for manual audit.

### Anonymity Model

- The voter's ID is hashed at scan time. The raw ID never enters the system.
- The anonymous ballot token is a one-way mix of the ID hash + machine
  secret. The ID hash is discarded after token generation.
- The public ledger shows the token, the choice, the time, and the
  precinct. It does NOT show the voter's identity.
- The blockchain stores the anonymous ballot. The voter uses their token
  (kept on the paper stub) to find their vote.
- **Known weakness (SLOP):** the one-way mix is FNV-1a, not real crypto.
  QA: replace with a proper blinding scheme or one-time pad.

## 2. Architecture

### Layers

```
+-----------------------------------------------+
|  FVMMain.cpp (single TU, happy path)          |
+-----------------------------------------------+
|  FVMVoter.h   -- ID scan, blockchain cue      |
|  FVMBallot.h  -- ballot struct, cast, checksum|
|  FVMLedger.h  -- paper ledger (text lines)    |
|  FVMPrinter.h -- paper roll, unroll/roll, mask|
|  FVMBlock.h   -- hash-chained blockchain      |
|  FVMCount.h   -- triple count, AI verify (stub)|
|  FVMAudit.h   -- Script-protocol byte-ring    |
|  FVMLookup.h  -- token-to-chain lookup        |
+-----------------------------------------------+
|  FVMTypes.h   -- local Crabs-type stand-ins   |
|  FVMHash.h    -- FNV-1a (slop, not crypto)    |
|  FVMStrings.h -- minimal CHA string helpers   |
|  FVMOut.h     -- console output (<cstdio>)    |
+-----------------------------------------------+
```

### All-Contiguous Design

Every FVM data structure is a fixed-size C struct with no pointers to
heap-allocated data. The paper ledger, blockchain, audit ring, and
paper roll are all contiguous arrays. This matches the Crabs
"all-contiguous" design principle and makes the memory layout
deterministic and auditable.

### SLOP-SHORTCUT Convention

Every deliberate shortcut is marked with a `SLOP-SHORTCUT:` comment in
the code. These are the raw material for the QA pass. The full list is
in `Code/README.md` (numbered 1-20).

## 3. Build and Test

### Build

```bash
cd ~/FreedomGovernment/FreedomVotingMachine/Code
g++ -g -o fvm FVM/FVMMain.cpp
```

### Run

```bash
./fvm
```

Expected: full 10-step happy path, triple count **AGREE**, exit 0.

### DISAGREE Test

```bash
g++ -g -o fvm_disagree_test FVM/FVMDisagreeTest.cpp
./fvm_disagree_test
```

Expected: corrupts the chain side-tally, triple count reports
**DISAGREE** (rc = FvmErrMismatch = -4).

### Real Crabs Core (ASCIICrabs)

The FVM currently uses local typedef stand-ins (`FVMTypes.h`) because
the real ASCII Crabs core was not building on this host. As of
2026-09-11, the core now compiles and runs on Linux (GCC 13, C++23).
The migration path:

1. Replace `#include "FVMTypes.h"` with `#include <_Config.h>` +
   `namespace _`.
2. Replace `FVMAudit.h` byte-ring with real `BIn`/`BOut`/`BSeq`.
3. Replace `FVMOut.h` `<cstdio>` with real Crabs `COut`/`SSPrinter`.
4. Replace `FVMHash.h` FNV-1a with real Crabs `Hash.hpp`/`BigInt`.
5. Wire real CrabsBed hardware for ID scan and ballot input.
6. Wire real SubsecondDb for ledger/chain persistence.

## 4. Agent Rules

### What to Do

- Add new features as new `.h` files in `Code/FVM/`.
- Mark every deliberate shortcut with `SLOP-SHORTCUT:`.
- Keep all data structures contiguous (no `new`, no `std::vector`).
- Use ASCII Crabs types (`CHA`, `IUD`, `ISN`, `BOL`, etc.) not std types.
- Add a test for every new behavior.
- Build and run before reporting completion.

### What NOT to Do

- Do NOT introduce the C++ standard library (`<string>`, `<vector>`,
  `<iostream>`, etc.) into FVM code.
- Do NOT modify the real ASCII Crabs core from this repo. The core is
  at `~/AStarStarship/ASCIICrabs/` and is owned by a different agent.
- Do NOT commit or push unless explicitly asked.
- Do NOT remove `SLOP-SHORTCUT` markers until the QA pass is done.
- Do NOT change the triple-count logic without updating all three
  count sources.

### Seam Tests

When adding functionality, add a test case to the seam tree. The
Crabs convention: tests are in `_Seams/` and selected by the `SEAM`
macro. For the FVM, tests are standalone `.cpp` files that compile
independently (like `FVMDisagreeTest.cpp`).

## 5. Documentation Map

| File | Purpose |
|------|---------|
| `README.md` | Project overview, stack, status, license |
| `AGENTS_PLAN.md` | **This file.** Agent operational plan |
| `Contributing.md` | Volunteer opportunities, how to contribute |
| `LiveStream.md` | Live coding session info |
| `Code/README.md` | Code layout, build, run, SLOP-SHORTCUT list |
| `Firmware/README.md` | Firmware notes (empty — greenfield) |
| `Hardware/` | KiCAD PCB files |

## 6. Known Limitations (from SLOP-SHORTCUT list)

1. No real Crabs core linked (local stand-ins).
2. Hash is FNV-1a, not real crypto.
3. Simulated ballot input (no CrabsBed hardware).
4. Token is NOT real anonymization (FNV-1a mix).
5. Paper ledger is in-memory, not persisted.
6. Blockchain is in-memory, not persisted.
7. Triple count #3 (blockchain) uses a side-tally, not re-derived.
8. AI verification is a STUB.
9. Audit ring is in-memory, not real BIn/BOut/BSeq.
10. Console output uses `<cstdio>`.
11. No real clock (fixed timestamps).
12. No input validation beyond `choice=` parsing.
13. No ring wraparound in audit ring.
14. No error-path integration tests.
15. Fixed sizes everywhere (not configurable).
16. Single translation unit, no modules.
17. `FVMStrings.h` `FvmSprintf_s` is a non-functional stub.
18. `FvmItos` has no negative-number handling.
19. `FvmHashToHex` has no compile-time size guarantee.
20. No real blockchain nonce (always 0, no PoW).

## 7. Sequencing

Per the 2026-09-10 decision in `README.md`:

1. **Vibe-code the FVM** with existing components. (DONE — this pass.)
2. **Inspect the slop** for critical problems. (QA pass — next.)
3. **Write the ASCII Crabs core update plan** from what the FVM reveals.
4. **Migrate the FVM** to the real Crabs core.
5. **Wire real hardware** (CrabsBed, SubsecondDb, dot-matrix printer).

## 8. 2026-09-30 Decisions (Lawsuit Pressure)

The McCollough v. PSU lawsuit has put ASCII Crabs directly in the public
record (PSU innovation grant, API link cited). The FVM is the product
that proves the Crabs Machine works. The following decisions were made
under the constraint of public scrutiny + a working demo for reporters
and the Judge:

### 8.1 Contiguous Stack Machine (Mandatory)

All voting machine state lives on the **contiguous Crabs stack machine**.
No discontiguous heap blocks, no `std::vector`, no `new`. Every data
structure is a fixed-size C struct or a Crabs type-value list. This is
the auditability requirement: a forensics team must be able to dump the
machine's memory and read every vote in sequence without following
pointers.

The heterogeneous list of type-value types (the Crabs `List` / `BSeq`)
is the primary storage mechanism. Each entry carries its own type tag,
so the list can hold mixed ballot data, ledger lines, block hashes, and
audit events in one contiguous buffer.

### 8.2 SHA256 — Deferred to Crabs Machine (Note Only)

SHA256 hashing at each stage is **NOT implemented in the FVM**. Instead,
the hashing responsibility belongs in the **Crabs Machine** itself — in
the ASCII Data Specification. The FVM uses the Crabs Machine's built-in
integrity mechanism (whatever that becomes) rather than rolling its own
hash.

**NOTE for the Crabs Machine / ASCII Data Spec team:**
The FVM requires tamper-evident hashing at each stage of the voting
pipeline (ballot cast, ledger line, block commit, count agreement).
SHA256 (or equivalent) should be a first-class operation in the Crabs
Machine, exposed as a type-value entry that the FVM can push onto the
contiguous stack. Do NOT add SHA256 to the FVM directly — add it to the
core so all Crabs applications benefit.

This note is the contract between the FVM and the Crabs Machine team.
Until the Crabs Machine ships its hash operation, the FVM uses the
FNV-1a SLOP-SHORTCUT as a placeholder.

### 8.3 License (Public-Facing)

- **Free for the State:** the machine is free to use for any state or
  jurisdiction running an election. No license fee, no per-voter cost.
- **Source-available, non-commercial:** the source code is public.
  Third parties may read, audit, and learn from it. Commercial
  redistribution is prohibited.
- **Only AStarship sells:** AStarship is the sole authorized commercial
  distributor. This keeps the revenue stream open and the staff well
  paid.
- **License name:** Kabuki Strong Source-available License (consent at
  the FreedomGovernment GitHub org).

### 8.4 Public Demo Requirements

For reporters and the Judge, the FVM must:
1. Compile on a stock Linux machine (GCC 13, no special deps).
2. Run the full 10-step happy path in under 2 seconds.
3. Print a clear, human-readable trace of each step.
4. End with `=== FVM Happy Path COMPLETE ===` and exit 0.
5. Have a README that a non-engineer can read to understand the flow.

The current `./fvm` binary satisfies all five. The README needs a
plain-English section for the non-technical audience (reporters, Judge,
jury).
