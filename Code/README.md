# Freedom Voting Machine — Code (Slop Pass)

This is a **deliberately rough, end-to-end FVM** in C++ that compiles and runs
a full happy path: ID scan → anonymize → print to paper roll → privacy mask →
seal in container → blockchain → triple count → AI verify → audit → lookup.
The goal of this pass is to **surface problems for a later QA pass**, not to
be clean. "Slop" is the point.

## What it does (happy path, 10 steps)

1. **ID scan** — State scans voter ID into the blockchain cue (FIFO).
2. **Anonymize** — Machine pops the token from the cue, generates an
   untraceable ballot token via one-way function. Original ID discarded.
3. **Paper vote** — Voter makes their paper vote (simulated CrabsBed press).
4. **Unroll + print** — Paper roll unrolls, machine prints ONLY the voter's
   line to the public anon ledger.
5. **Privacy mask** — Voter inspects their line. Roll position shows only
   their line; other votes are masked out.
6. **Seal** — Paper roll re-rolls and seals in the privacy container.
7. **Blockchain** — Vote submitted to the hash-chained, append-only chain.
8. **Triple count** — In-memory, paper-text, and blockchain tallies must
   all **AGREE** or the result is flagged **DISAGREE**.
9. **AI verify** — AI-assisted ledger verification (STUB).
10. **Lookup** — Voter looks up their ballot on the chain using their
    anonymous token (the "future app" feature).

## Layout

```
Code/
  README.md          <-- you are here
  FVM/
    FVMTypes.h       Local Crabs-type stand-ins (IUD, ISN, CHA, ...) + status codes.
    FVMHash.h        FNV-1a hash (slop, not real crypto).
    FVMStrings.h     Minimal CHA string helpers (no stdlib <string>/<cstring>).
    FVMOut.h         Console output via <cstdio> (the one stdlib I/O touch).
    FVMBallot.h      Ballot struct, simulated input, cast + checksum.
    FVMLedger.h      Paper ledger (text lines) + text parser for triple count.
    FVMBlock.h       Hash-chained blockchain + side-tally.
    FVMCount.h       Triple-count consensus + AI-verify STUB.
    FVMAudit.h       In-memory byte-ring audit message passing.
    FVMVoter.h       ID scan, blockchain cue, anonymization. (NEW)
    FVMPrinter.h     Paper roll, unroll/roll, privacy masking. (NEW)
    FVMLookup.h      Token-to-chain lookup (future app). (NEW)
    FVMMain.cpp      Single-TU entrypoint, runs the 10-step happy path.
    FVMDisagreeTest.cpp  Sanity test: proves triple count reports DISAGREE.
```

## Build

Single translation unit, plain g++. No CMake. The FVM links the **real
ASCIICrabs core** (the all-contiguous type system + `COut` console stream).
The core is resolved through the `-I` include path — the core root comes
**before** the `_Seams` dir (that ordering avoids the `namespace _` shadowing
`std` when `COut.hxx` pulls `<iostream>` inside `namespace _`):

```
cd /home/astarcale/FreedomGovernment/FreedomVotingMachine/Code
CORE=~/AStarStarship/ASCIICrabs
g++ -std=gnu++23 -g -o fvm -I"$CORE" -I"$CORE/_Seams" FVM/FVMMain.cpp
```

Optional sanity test (proves DISAGREE is detected):

```
g++ -std=gnu++23 -g -o fvm_disagree_test -I"$CORE" -I"$CORE/_Seams" FVM/FVMDisagreeTest.cpp
```

Note: `-std=gnu++23` (not `-std=c++23`) matches the core's own CMake build.
The FVM's `FVMMain.cpp` / `FVMDisagreeTest.cpp` include `<_Config.h>` (via
`FVMTypes.h`) for the real types and `<_Package.hxx>` for the core
implementation; `FVMOut.h` uses the real `StdOut()`/`COut` stream.

## Run

```
./fvm
```

Expected: prints the full 10-step happy-path trace ending in
`=== FVM Happy Path COMPLETE ===` with exit code 0 and the triple count
reporting `AGREE`.

## Real Crabs core now linked (migration complete)

As of this pass the FVM **links the real ASCIICrabs core**, not local
typedef stand-ins. What changed vs. the earlier slop pass:

- **Types are the core's.** `FVMTypes.h` no longer re-defines
  `CHA/IUD/ISC/...`; it `#include <_Config.h>` and keeps only FVM-specific
  status codes (`FvmErrMismatch`, etc.). All FVM structs (`FvmBallot`,
  `FvmBlock`, `FvmPaperLedger`, ...) now use the core's real type system.
- **Console output is the core's `COut`.** `FVMOut.h` no longer uses
  `<cstdio>`; it emits through `StdOut() << ...` / `.NL()` / `.Hex()`.
- **Single-TU impl via `_Package.hxx`.** The `.cpp` entrypoints include
  `<_Package.hxx>` (the core's single-translation-unit umbrella) so `COut`
  and the AType/Stringf/Uniprinter chain actually link.
- **Stale config copies removed.** The FVM's own `_Config.h`,
  `_ConfigHeader.h`, `_ConfigFooter.h`, `_Release.h`, `_Debug.h`, `_Test.h`,
  `_Undef.h` (which pointed at a broken `../../../../` path and shadowed the
  real core) are deleted. The core is the single source of truth.

The audit machinery — the byte-ring (`FVMAudit.h`), FNV-1a hash
(`FVMHash.h`), and the in-memory ledger/chain — is still the FVM's own
slop. It is deliberately kept as FVM-specific audit logic (see the
shortcut list below); the core's `BIn`/`BOut`/`BSeq` are the deeper
Script-protocol sockets and are a later integration, not needed for the
happy path.

### Remaining shortcuts (raw material for QA)

These are the still-faked pieces that a QA pass should address. The
type-system and console-output migration (items the old "migration path"
listed) are now **done** — see the "Real Crabs core now linked" section
above. What remains:

- Replace `FVMAudit.h` byte-ring with the real `BIn`/`BOut`/`BSeq`
  Script-protocol sockets.
- Replace `FVMHash.h` FNV-1a with real Crabs `Hash.hpp`/`BigInt` (real
  preimage-resistant crypto).
- Wire real CrabsBed hardware for ID scan and ballot input.
- Wire real SubsecondDb for ledger/chain persistence.

## Candid shortcut / stub list (raw material for QA)

Numbered; each names file, function, what was faked, and why.

1. **~~No real Crabs core linked~~ — DONE.** `FVMTypes.h` now `#include
   <_Config.h>` for the real types and the `.cpp` entrypoints include
   `<_Package.hxx>` for the impl; `FVMOut.h` uses the real `COut`.
   Remaining: the audit byte-ring and FNV-1a hash are still FVM-local
   (see #2 and #9).

2. **Hash is FNV-1a, not real crypto.** `FVMHash.h` `FvmFnv1a`. Fine for a
   happy-path probe; collision-prone and not preimage-resistant. QA: replace
   with a real hash.

3. **Simulated ballot input.** `FVMBallot.h` `FvmSimulatedVoterPress()`
   returns a hardcoded candidate (2). No CrabsBed hardware. QA: wire real
   Button/Switch/RotaryKnob/Unicontroller.

4. **Token is NOT real anonymization.** `FVMVoter.h` `FvmCuePopAnonymous`
   = FNV-1a(id_hash ^ machine_secret). Linkable if the secret is known;
   no blinding/one-way property. QA: real anonymization.

5. **Paper ledger is in-memory, not persisted.** `FVMLedger.h`. No
   SubsecondDb / PostgreSQL write. QA: real SubsecondDb storage layer.

6. **Blockchain is in-memory, not persisted.** `FVMBlock.h`. No real chain
   store. `nonce` is always 0 (no real PoW). `timestamp` is fixed (not a
   real clock). QA: real persistence + real timestamp + real nonce policy.

7. **Triple count #3 (blockchain) is the weakest.** `FVMBlock.h`
   `FvmChainAddToTally` maintains a *side-tally* updated when ballots are
   added, rather than re-deriving the count from the stored block bytes.
   QA: make the blockchain re-derive its tally from stored block data.

8. **AI verification is a STUB.** `FVMCount.h` `FvmAiVerifyLedger` always
   returns `verified = triple->agree` with reason "(stub)". QA: real
   AI-assisted verification / fraud detection.

9. **Audit ring is an in-memory byte-buffer, not real BIn/BOut/BSeq.**
   `FVMAudit.h`. QA: replace with real `BIn`/`BOut`/`BSeq`.

10. **~~Console output uses `<cstdio>`~~ — DONE.** `FVMOut.h` now emits
    through the real Crabs `COut`/`StdOut()` stream.

11. **`FVMStrings.h` `FvmSprintf_s` is a non-functional formatter stub.**
    QA: either implement a real formatter or delete this function.

12. **`FvmItos` has no negative-number handling.** Slop: unsigned decimal
    only. Fine for this pass.

13. **Fixed sizes everywhere.** 4 candidates, 64 ledger lines, 16 blocks,
    8-ballot batch, 4096-byte audit ring, 32-voter cue, 128-line roll.
    QA: make configurable / dynamic.

14. **No real clock.** Fixed timestamps so runs are deterministic.
    QA: wire a real clock (Crabs `Clock`).

15. **No input validation of ledger text beyond `choice=`.** QA: robust
    parser + error reporting.

16. **No ring wraparound in the audit ring.** QA: real ring-buffer
    wraparound + backpressure.

17. **`FvmHashToHex` has no compile-time size guarantee.** QA: add a
    static_assert or a sized return.

18. **Single translation unit, no modules.** QA: decide the real
    module/linkage strategy.

19. **No error-path integration tests.** QA: build a real seam-test tree.

20. **Paper roll is in-memory, not a real dot-matrix printer.**
    `FVMPrinter.h`. The "roll" is a CHA buffer; the "roll position" is an
    ISN index; the "privacy container" is a BOL flag. QA: wire to a real
    CrabsBed dot-matrix driver.

21. **Voter lookup only works on uncommitted batches.** `FVMLookup.h`
    `FvmLookupToken` searches the current batch buffer. After commit, the
    batch is cleared, so committed ballots can't be looked up. QA: store
    ballot bytes in blocks for post-commit lookup.

## Verified behavior (this pass)

- `g++ -std=gnu++23 -g -o fvm -I"$CORE" -I"$CORE/_Seams" FVM/FVMMain.cpp`
  → compiles clean against the real core, exit 0.
- `./fvm` → full 10-step happy-path trace, triple count **AGREE**, 4 audit
  events read back, voter lookup succeeds, exit 0.
- `FVMDisagreeTest` → corrupts the chain side-tally, triple count correctly
  reports **DISAGREE** (rc = FvmErrMismatch = -4).
- Tamper check → flipping `ballot.choice` after cast makes
  `FvmBallotValid` return false.

## See also

- `../AGENTS_PLAN.md` — the operational plan for AI agents working on the FVM.
- `../README.md` — project overview, stack, status, license.
