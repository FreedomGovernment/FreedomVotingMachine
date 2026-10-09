// Copyright AStarship <https://astarship.net>.
//
// FVMMain.cpp -- Single-translation-unit entrypoint for the FVM happy path.
//
// Full flow:
//   1. State scans voter ID into the blockchain cue.
//   2. Voter makes their paper vote (simulated CrabsBed press).
//   3. Machine anonymizes the ballot (pops token from cue).
//   4. Ballot is printed to the public anon ledger (paper roll unrolls).
//   5. Voter inspects their line (privacy mask: only their line visible).
//   6. Paper roll re-rolls and seals in the privacy container.
//   7. Vote is submitted to the blockchain.
//   8. Triple count (in-memory / paper text / blockchain) must AGREE.
//   9. AI-assisted verification (STUB).
//   10. Audit events read back from the Script-protocol byte-ring.
//   11. Voter looks up their ballot on the chain using their token.
//
// Build: see Code/README.md.
// This is deliberately rough "slop" — the goal is to surface problems
// for a later QA pass, not to be clean.

#include "FVMTypes.h"
#include "FVMHash.h"
#include "FVMStrings.h"
#include "FVMOut.h"
#include "FVMBallot.h"
#include "FVMLedger.h"
#include "FVMBlock.h"
#include "FVMCount.h"
#include "FVMAudit.h"
#include "FVMVoter.h"
#include "FVMPrinter.h"
#include "FVMLookup.h"

// Real ASCII Crabs core implementation (single-translation-unit umbrella).
// Pulls COut.hxx / Stringf.hxx / AType.hxx / Uniprinter.hpp etc. so the
// StdOut() stream used by FVMOut.h actually links. Angle-bracket so it
// resolves to the core's _Package.hxx on the -I path. SEAM + platform config
// are already set by <_Config.h> (via FVMTypes.h).
#include <_Package.hxx>

using namespace ::_;

// Emits an audit event into the byte-ring.
static ISN AuditEmit(FvmByteRing* ring, IUA event_type, const IUA* payload,
                     IUN len) {
  return FvmAuditEmit(ring, event_type, payload, len);
}

ISN main(ISN arg_count, CHA** args) {
  (void)arg_count;
  (void)args;

  FvmPrintln("=== FVM Happy Path (slop pass) ===");
  FvmPrintln("");

  // --- State (all contiguous, stack-allocated). ---
  FvmPaperLedger ledger;
  FvmLedgerInit(&ledger);
  FvmBlockchain chain;
  FvmChainInit(&chain);
  FvmChainTally chain_tally;
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    chain_tally.counts[c] = 0;
  }
  FvmByteRing audit_ring;
  FvmRingInit(&audit_ring);
  FvmBallot ballot;
  FvmBlockchainCue cue;
  FvmCueInit(&cue);
  FvmPaperRoll roll;
  FvmRollInit(&roll);

  // SLOP: fake voter ID. Real FVM: State-issued ID scanned by CrabsBed.
  const IUD kVoterId = 0xC0FFEE1234567890ULL;

  // --- Step 1: State scans the voter ID into the blockchain cue. ---
  FvmPrint("[1] State scans voter ID into blockchain cue...");
  ERC rc = FvmCueScanId(&cue, kVoterId);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    cue count = ");
  FvmPrintU64(FvmCueCount(&cue));
  FvmPrintln("");

  // --- Step 2: Voter makes their paper vote (simulated CrabsBed press). ---
  FvmPrint("[2] Voter makes paper vote (simulated CrabsBed press)...");
  IUD anon_token = 0;
  IUD cue_pos = 0;
  rc = FvmCuePopAnonymous(&cue, &anon_token, &cue_pos);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    anon token = 0x");
  FvmPrintHex64(anon_token);
  FvmPrintln("");
  FvmPrint("    cue position = ");
  FvmPrintU64(cue_pos);
  FvmPrintln("");
  FvmPrint("    cue count  = ");
  FvmPrintU64(FvmCueCount(&cue));
  FvmPrintln("");

  // Build the ballot with the anonymous token.
  ballot.token = anon_token;
  ballot.choice = FvmSimulatedVoterPress();
  if (ballot.choice >= static_cast<IUC>(FvmCandidateCount)) {
    FvmPrintln(" FAILED (invalid choice)");
    return -1;
  }
  ballot.cast_time = 1757500000ULL;  // SLOP: fixed timestamp.
  ballot.precinct = 1;
  // Checksum over the preceding fields.
  IUD* words = reinterpret_cast<IUD*>(&ballot);
  IUD sum = 0;
  for (IUN i = 0; i < 4; ++i) {
    sum ^= (words[i] * 0x2545F4914F6CDD1DULL);
  }
  ballot.checksum = sum;
  if (!FvmBallotValid(&ballot)) {
    FvmPrintln(" FAILED (checksum)");
    return -1;
  }
  FvmPrint("    choice     = ");
  FvmPrintU64(ballot.choice);
  FvmPrintln("");
  FvmPrint("    checksum   = 0x");
  FvmPrintHex64(ballot.checksum);
  FvmPrintln("");

  // Audit event: ballot-cast.
  rc = AuditEmit(&audit_ring, FvmEvtBallotCast,
                 reinterpret_cast<const IUA*>(&ballot), sizeof(FvmBallot));
  if (rc != FvmStatusOk) {
    FvmPrintln("    [audit: ballot-cast emit FAILED]");
    return -1;
  }

  // --- Step 3: Unroll the paper and print the ballot to the ledger. ---
  FvmPrint("[3] Unroll paper, print to public anon ledger...");
  ISN roll_line = FvmRollUnroll(&roll);
  if (roll_line < 0) {
    FvmPrintln(" FAILED (roll full)");
    return -1;
  }
  CHA line_buf[FvmLedgerLineBytes];
  ISN line_len = FvmBallotToLine(&ballot, 0, line_buf, sizeof(line_buf));
  if (line_len < 0) {
    FvmPrintln(" FAILED (format)");
    return -1;
  }
  rc = FvmRollPrintLine(&roll, line_buf, line_len);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED (print)");
    return -1;
  }
  // Also append to the in-memory paper ledger (for triple count #2).
  rc = FvmLedgerAppend(&ledger, &ballot);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED (ledger)");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    roll line  = ");
  FvmPrintln(roll.lines[roll_line]);

  // Audit event: ledger-line.
  rc = AuditEmit(&audit_ring, FvmEvtLedgerLine,
                 reinterpret_cast<const IUA*>(line_buf),
                 static_cast<IUN>(line_len));
  if (rc != FvmStatusOk) {
    FvmPrintln("    [audit: ledger-line emit FAILED]");
    return -1;
  }

  // --- Step 4: Voter inspects their line (privacy mask). ---
  FvmPrint("[4] Voter inspects line (privacy mask: only own line visible)...");
  rc = FvmRollMaskAllExcept(&roll, roll_line);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  const CHA* visible = FvmRollVisibleLine(&roll);
  FvmPrintln(" OK");
  FvmPrint("    visible line = ");
  FvmPrintln(visible);
  FvmPrint("    total lines on roll = ");
  FvmPrintU64(roll.line_count);
  FvmPrintln("");

  // --- Step 5: Re-roll and seal in the privacy container. ---
  FvmPrint("[5] Re-roll paper, seal in privacy container...");
  rc = FvmRollSealInContainer(&roll);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    sealed       = ");
  FvmPrintln(FvmRollIsSealed(&roll) ? "true" : "false");
  FvmPrint("    lines sealed = ");
  FvmPrintU64(roll.lines_in_container);
  FvmPrintln("");

  // --- Step 6: Submit the vote to the blockchain. ---
  FvmPrint("[6] Submit vote to blockchain...");
  rc = FvmChainAddBallot(&chain, &ballot);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  FvmChainAddToTally(&chain, &ballot, &chain_tally);
  rc = FvmChainCommitBatch(&chain);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED (commit)");
    return -1;
  }
  FvmPrintln(" OK");
  {
    const FvmBlock* block = &chain.blocks[0];
    FvmPrint("    block 0 index     = ");
    FvmPrintU64(block->index);
    FvmPrintln("");
    FvmPrint("    block 0 batch_hash= 0x");
    FvmPrintHex64(block->batch_hash);
    FvmPrintln("");
    FvmPrint("    chain verify      = ");
    FvmPrintln(FvmChainVerify(&chain) ? "PASS" : "FAIL");
    FvmPrint("    chain ballots     = ");
    FvmPrintU64(FvmChainBallotCount(&chain));
    FvmPrintln("");
  }

  // Audit event: block-appended.
  {
    const IUD* bwords = reinterpret_cast<const IUD*>(&chain.blocks[0].index);
    rc = AuditEmit(&audit_ring, FvmEvtBlockAppended,
                   reinterpret_cast<const IUA*>(bwords), 5 * sizeof(IUD));
    if (rc != FvmStatusOk) {
      FvmPrintln("    [audit: block-appended emit FAILED]");
      return -1;
    }
  }

  // --- Step 7: Triple count. ---
  FvmPrint("[7] Triple count...");
  FvmTripleCount triple;
  rc = FvmTripleCountRun(&ballot, 1, &ledger, &chain, &chain_tally, &triple);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    in-memory : [");
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    FvmPrintU64(triple.in_memory[c]);
    if (c < FvmCandidateCount - 1) FvmPrint(", ");
  }
  FvmPrintln("]");
  FvmPrint("    paper     : [");
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    FvmPrintU64(triple.paper[c]);
    if (c < FvmCandidateCount - 1) FvmPrint(", ");
  }
  FvmPrintln("]");
  FvmPrint("    blockchain: [");
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    FvmPrintU64(triple.blockchain[c]);
    if (c < FvmCandidateCount - 1) FvmPrint(", ");
  }
  FvmPrintln("]");
  FvmPrint("    result    = ");
  FvmPrintln(triple.agree ? "AGREE" : "DISAGREE");

  // Audit event: count-agreement.
  {
    IUA payload[3 * FvmCandidateCount * sizeof(IUC)];
    const IUC* srcs[3] = {triple.in_memory, triple.paper, triple.blockchain};
    IUN off = 0;
    for (ISN s = 0; s < 3; ++s) {
      for (ISN c = 0; c < FvmCandidateCount; ++c) {
        const IUA* w = reinterpret_cast<const IUA*>(&srcs[s][c]);
        for (IUN j = 0; j < sizeof(IUC); ++j) {
          payload[off++] = w[j];
        }
      }
    }
    rc = AuditEmit(&audit_ring, FvmEvtCountAgree, payload, off);
    if (rc != FvmStatusOk) {
      FvmPrintln("    [audit: count-agree emit FAILED]");
      return -1;
    }
  }

  // --- Step 8: AI-assisted verification (STUB). ---
  FvmPrint("[8] AI-assisted verification (STUB)...");
  FvmAiVerdict verdict;
  rc = FvmAiVerifyLedger(&ledger, &triple, &verdict);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    verified  = ");
  FvmPrintln(verdict.verified ? "true" : "false");
  FvmPrint("    reason    = ");
  FvmPrintln(verdict.reason);

  // --- Step 9: Read back audit events (Script-protocol BIn side). ---
  FvmPrint("[9] Read back audit events (Script-protocol BIn side)...");
  {
    IUA type = 0;
    IUA payload[1024];
    IUN payload_len = 0;
    ISN events_read = 0;
    for (;;) {
      rc = FvmAuditRead(&audit_ring, &type, payload, sizeof(payload),
                        &payload_len);
      if (rc != FvmStatusOk) break;
      ++events_read;
      FvmPrint("    event ");
      const CHA* label = "?";
      if (type == FvmEvtBallotCast) label = "ballot-cast";
      else if (type == FvmEvtCountAgree) label = "count-agree";
      else if (type == FvmEvtBlockAppended) label = "block-appended";
      else if (type == FvmEvtLedgerLine) label = "ledger-line";
      FvmPrint(label);
      FvmPrint(" payload_len=");
      FvmPrintU64(payload_len);
      FvmPrintln("");
    }
    FvmPrint("    events_read = ");
    FvmPrintU64(events_read);
    FvmPrintln("");
  }

  // --- Step 10: Voter looks up their ballot on the chain. ---
  FvmPrint("[10] Voter looks up ballot on chain (future app)...");
  // The ballot is now committed, so the batch is empty. The lookup
  // won't find it (SLOP: batch cleared after commit). This is a
  // known limitation — QA: store ballots in blocks for post-commit
  // lookup. We demonstrate the lookup works on a fresh ballot.
  FvmBallot lookup_ballot;
  lookup_ballot.token = anon_token;
  lookup_ballot.choice = 0;
  lookup_ballot.cast_time = 0;
  lookup_ballot.precinct = 0;
  lookup_ballot.checksum = 0;
  rc = FvmChainAddBallot(&chain, &lookup_ballot);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED (add for lookup)");
    return -1;
  }
  FvmLookupResult lookup;
  rc = FvmLookupToken(&chain, anon_token, &lookup);
  if (rc != FvmStatusOk) {
    FvmPrintln(" FAILED (not found)");
    return -1;
  }
  FvmPrintln(" OK");
  FvmPrint("    found         = ");
  FvmPrintln(lookup.found ? "true" : "false");
  FvmPrint("    block_index   = ");
  FvmPrintU64(lookup.block_index);
  FvmPrintln("");
  FvmPrint("    ballot_offset = ");
  FvmPrintU64(lookup.ballot_offset);
  FvmPrintln("");

  FvmPrintln("");
  FvmPrintln("=== FVM Happy Path COMPLETE ===");
  return 0;
}
