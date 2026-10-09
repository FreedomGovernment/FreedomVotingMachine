// Copyright AStarship <https://astarship.net>.
//
// FVMDisagreeTest.cpp -- Quick sanity check that the triple count reports
// DISAGREE when one of the three sources is corrupted. SLOP test: not part of
// the deliverable, just proof the check is real.

#include "FVMTypes.h"
#include "FVMHash.h"
#include "FVMStrings.h"
#include "FVMOut.h"
#include "FVMBallot.h"
#include "FVMLedger.h"
#include "FVMBlock.h"
#include "FVMCount.h"
#include "FVMAudit.h"

// Real ASCII Crabs core implementation (single-translation-unit umbrella).
// Angle-bracket so it resolves to the core's _Package.hxx on the -I path.
#include <_Package.hxx>

using namespace ::_;

ISN main(ISN, CHA**) {
  FvmPaperLedger ledger;
  FvmLedgerInit(&ledger);
  FvmBlockchain chain;
  FvmChainInit(&chain);
  FvmChainTally chain_tally;
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    chain_tally.counts[c] = 0;
  }
  FvmBallot ballot;
  if (FvmCastBallot(&ballot, 0xC0FFEE1234567890ULL, 1) != FvmStatusOk) {
    return -1;
  }
  FvmLedgerAppend(&ledger, &ballot);
  FvmChainAddBallot(&chain, &ballot);
  FvmChainAddToTally(&chain, &ballot, &chain_tally);
  FvmChainCommitBatch(&chain);

  // Corrupt the chain side-tally: pretend the blockchain counted candidate 3.
  chain_tally.counts[ballot.choice] = 0;
  chain_tally.counts[3] = 1;

  FvmTripleCount triple;
  ERC rc = FvmTripleCountRun(&ballot, 1, &ledger, &chain, &chain_tally,
                             &triple);
  FvmPrint("expected DISAGREE, got: ");
  FvmPrint(triple.agree ? "AGREE" : "DISAGREE");
  FvmPrintln("");
  FvmPrint("rc = ");
  FvmPrintU64(static_cast<IUD>(rc));
  FvmPrintln("");
  return (rc == FvmErrMismatch && !triple.agree) ? 0 : -1;
}
