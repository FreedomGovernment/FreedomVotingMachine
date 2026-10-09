// Copyright AStarship <https://astarship.net>.
//
// FVMCount.h -- The triple-count consensus check.
//
// The core auditability property: each ballot is counted THREE independent
// ways and all three must agree:
//   (1) in-memory tally (direct from the FvmBallot struct)
//   (2) paper-ledger tally (parsed from the ledger TEXT)
//   (3) blockchain tally (from the chain's stored batch / side-tally)
//
// SLOP-SHORTCUT: Count (3) uses a side-tally that is updated when ballots are
// added to the chain, rather than re-deriving from the stored block bytes.
// See FVMBlock.h for details. Counts (1) and (2) are genuinely independent.
#pragma once
#ifndef FVM_COUNT_DECL
#define FVM_COUNT_DECL

#include "FVMTypes.h"
#include "FVMStrings.h"
#include "FVMBallot.h"
#include "FVMLedger.h"
#include "FVMBlock.h"

namespace _ {

// The result of the triple-count.
struct FvmTripleCount {
  IUC in_memory[FvmCandidateCount];   //< Count (1).
  IUC paper[FvmCandidateCount];       //< Count (2).
  IUC blockchain[FvmCandidateCount];  //< Count (3).
  BOL agree;                          //< true if all three match.
};

// Runs the triple count. `ballots`/`ballot_count` is the in-memory source,
// `ledger` is the paper source, `chain`+`chain_tally` is the blockchain
// source. Returns FvmStatusOk or FvmErrMismatch.
inline ERC FvmTripleCountRun(const FvmBallot* ballots, ISN ballot_count,
                             const FvmPaperLedger* ledger,
                             const FvmBlockchain* chain,
                             const FvmChainTally* chain_tally,
                             FvmTripleCount* result) {
  if (result == NILP) {
    return FvmErrBadInput;
  }
  // Count (1): in-memory.
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    result->in_memory[c] = 0;
    result->paper[c] = 0;
    result->blockchain[c] = 0;
  }
  for (ISN i = 0; i < ballot_count; ++i) {
    if (ballots[i].choice < static_cast<IUC>(FvmCandidateCount)) {
      ++result->in_memory[ballots[i].choice];
    }
  }
  // Count (2): paper ledger text.
  ERC rc = FvmLedgerTally(ledger, result->paper);
  if (rc != FvmStatusOk) {
    return rc;
  }
  // Count (3): blockchain side-tally.
  if (chain_tally != NILP) {
    for (ISN c = 0; c < FvmCandidateCount; ++c) {
      result->blockchain[c] = chain_tally->counts[c];
    }
  }
  (void)chain;
  // Compare all three.
  result->agree = true;
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    if (result->in_memory[c] != result->paper[c] ||
        result->in_memory[c] != result->blockchain[c]) {
      result->agree = false;
    }
  }
  return result->agree ? FvmStatusOk : FvmErrMismatch;
}

// AI-assisted ledger verification / fraud detection.
// SLOP-STUB: Always returns verified=true. This is a placeholder for the
// real AI verification pass. QA: replace with a real model or rule engine.
struct FvmAiVerdict {
  BOL verified;
  CHA reason[64];
};

inline ERC FvmAiVerifyLedger(const FvmPaperLedger* ledger,
                             const FvmTripleCount* triple,
                             FvmAiVerdict* verdict) {
  // TODO(AI): Implement real AI-assisted ledger verification and fraud
  // detection. For this slop pass, we just return "verified" if the triple
  // count agreed and the ledger is non-empty.
  (void)ledger;
  if (verdict == NILP || triple == NILP) {
    return FvmErrBadInput;
  }
  verdict->verified = triple->agree;
  if (triple->agree) {
    FvmStrcpy(verdict->reason, sizeof(verdict->reason),
              "triple-count agreed (stub)");
  } else {
    FvmStrcpy(verdict->reason, sizeof(verdict->reason),
              "triple-count DISAGREED (stub)");
  }
  return FvmStatusOk;
}

}  //< namespace _
#endif  //< FVM_COUNT_DECL
