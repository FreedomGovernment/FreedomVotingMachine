// Copyright AStarship <https://astarship.net>.
//
// FVMLookup.h -- Voter token-to-blockchain lookup.
//
// After the vote is submitted to the blockchain, the voter can use
// their anonymous token (kept on the paper stub) to find their ballot
// on the public chain. This is the "future app" feature: the voter
// opens the app, enters their token, and the app scans the blockchain
// for the matching ballot.
//
// The token is the ONLY link between the voter and their ballot.
// There is no way to go from the token back to the voter's identity
// (the ID hash was discarded at anonymization time).
//
// SLOP-SHORTCUT: The lookup is a linear scan of the in-memory chain.
// QA: when the chain is in SubsecondDb, this becomes an indexed query.
#pragma once
#ifndef FVM_LOOKUP_DECL
#define FVM_LOOKUP_DECL

#include "FVMTypes.h"
#include "FVMBlock.h"
#include "FVMBallot.h"

namespace _ {

// The result of a token lookup.
struct FvmLookupResult {
  BOL found;          //< True if the token was found on the chain.
  IUD block_index;    //< Which block contains the ballot.
  IUD ballot_offset;  //< Position of the ballot within the block's batch.
  IUD choice;         //< The candidate index that was voted for.
  IUD cast_time;      //< When the ballot was cast.
};

// Looks up a ballot token in the blockchain.
// Returns FvmStatusOk if found (out_result is filled),
// or FvmErrBadInput if the token is not on the chain.
//
// SLOP: The chain stores ballots in its batch buffer before commit.
// After commit, the batch is cleared. So the lookup only works
// on the current (uncommitted) batch. For committed blocks, the
// lookup would need to re-read the block's stored ballot bytes.
// QA: store ballot bytes in the block for post-commit lookup.
inline ERC FvmLookupToken(const FvmBlockchain* chain, IUD token,
                           FvmLookupResult* out_result) {
  if (chain == NILP || out_result == NILP) {
    return FvmErrBadInput;
  }
  out_result->found = false;
  out_result->block_index = 0;
  out_result->ballot_offset = 0;
  out_result->choice = 0;
  out_result->cast_time = 0;

  // Search the current batch (uncommitted ballots).
  for (ISN i = 0; i < chain->batch_count; ++i) {
    if (chain->batch[i].token == token) {
      out_result->found = true;
      out_result->block_index = static_cast<IUD>(chain->block_count);
      out_result->ballot_offset = static_cast<IUD>(i);
      out_result->choice = chain->batch[i].choice;
      out_result->cast_time = chain->batch[i].cast_time;
      return FvmStatusOk;
    }
  }

  // Not found in the current batch. In a real system, we'd also
  // search committed blocks. SLOP: we can't because the batch is
  // cleared after commit. QA: store ballots in blocks.
  return FvmErrBadInput;
}

// Returns the total number of ballots on the chain (committed + batch).
inline IUD FvmChainBallotCount(const FvmBlockchain* chain) {
  if (chain == NILP) {
    return 0;
  }
  IUD total = chain->batch_count;
  for (ISN i = 0; i < chain->block_count; ++i) {
    total += chain->blocks[i].ballot_count;
  }
  return total;
}

}  //< namespace _
#endif  //< FVM_LOOKUP_DECL
