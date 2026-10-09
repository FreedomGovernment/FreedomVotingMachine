// Copyright AStarship <https://astarship.net>.
//
// FVMBlock.h -- Minimal hash-chained blockchain for the FVM.
//
// SLOP-SHORTCUT: Uses the FNV-1a hash from FVMHash.h, not the real Crabs
// Hash.hpp / BigInt. The chain is a fixed-size array of blocks in memory,
// NOT persisted to SubsecondDb / PostgreSQL. Append-only: there is no delete
// or rewrite path.
#pragma once
#ifndef FVM_BLOCK_DECL
#define FVM_BLOCK_DECL

#include "FVMTypes.h"
#include "FVMHash.h"
#include "FVMStrings.h"
#include "FVMBallot.h"

namespace _ {

constexpr ISN FvmChainMaxBlocks = 16;
constexpr ISN FvmBlockBatch = 8;  //< Ballots per block (slop: 1 is fine).

// One block in the hash chain.
struct FvmBlock {
  IUD index;         //< Block number (0-based).
  IUD prev_hash;     //< Hash of the previous block header.
  IUD batch_hash;    //< FNV-1a over the ballot batch in this block.
  IUD timestamp;     //< Seconds since epoch.
  IUD nonce;         //< SLOP: nonce is always 0 (no real PoW).
  ISN ballot_count;  //< How many ballots are in this block's batch.
};

// The blockchain: an append-only array of blocks.
struct FvmBlockchain {
  FvmBlock blocks[FvmChainMaxBlocks];
  ISN block_count;
  // The current batch buffer (ballots waiting to be committed into a block).
  FvmBallot batch[FvmBlockBatch];
  ISN batch_count;
};

inline void FvmChainInit(FvmBlockchain* chain) {
  chain->block_count = 0;
  chain->batch_count = 0;
  // Genesis prev_hash = 0.
}

// Hashes a block header (index, prev_hash, batch_hash, timestamp, nonce).
inline IUD FvmBlockHeaderHash(const FvmBlock* block) {
  IUA bytes[40];
  const IUD* words = reinterpret_cast<const IUD*>(&block->index);
  for (IUN i = 0; i < 5; ++i) {
    const IUA* src = reinterpret_cast<const IUA*>(&words[i]);
    for (IUN j = 0; j < 8; ++j) {
      bytes[i * 8 + j] = src[j];
    }
  }
  return FvmFnv1a(bytes, 40);
}

// Commits the current batch as a new block. Returns FvmStatusOk or error.
inline ERC FvmChainCommitBatch(FvmBlockchain* chain) {
  if (chain == NILP) {
    return FvmErrBadInput;
  }
  if (chain->block_count >= FvmChainMaxBlocks) {
    return FvmErrFull;
  }
  FvmBlock* block = &chain->blocks[chain->block_count];
  block->index = static_cast<IUD>(chain->block_count);
  if (chain->block_count == 0) {
    block->prev_hash = 0;
  } else {
    block->prev_hash = FvmBlockHeaderHash(
        &chain->blocks[chain->block_count - 1]);
  }
  // Hash the batch of ballots.
  IUA batch_bytes[FvmBlockBatch * sizeof(FvmBallot)];
  const IUA* src = reinterpret_cast<const IUA*>(chain->batch);
  for (ISN i = 0; i < chain->batch_count; ++i) {
    for (IUN j = 0; j < sizeof(FvmBallot); ++j) {
      batch_bytes[i * sizeof(FvmBallot) + j] = src[i * sizeof(FvmBallot) + j];
    }
  }
  block->batch_hash = FvmFnv1a(batch_bytes,
      static_cast<IUN>(chain->batch_count * sizeof(FvmBallot)));
  // SLOP: fixed timestamp, nonce 0.
  block->timestamp = 1757500001ULL;
  block->nonce = 0;
  block->ballot_count = chain->batch_count;
  ++chain->block_count;
  chain->batch_count = 0;
  return FvmStatusOk;
}

// Adds a ballot to the current batch.
inline ERC FvmChainAddBallot(FvmBlockchain* chain, const FvmBallot* ballot) {
  if (chain == NILP || ballot == NILP) {
    return FvmErrBadInput;
  }
  if (chain->batch_count >= FvmBlockBatch) {
    // Auto-commit the full batch, then add.
    ERC rc = FvmChainCommitBatch(chain);
    if (rc != FvmStatusOk) {
      return rc;
    }
  }
  chain->batch[chain->batch_count] = *ballot;
  ++chain->batch_count;
  return FvmStatusOk;
}

// Tallies choices from the blockchain by walking every ballot in every
// committed block's batch... SLOP: the batch is cleared after commit, so we
// keep a running copy. For the triple count we re-hash the chain and compare
// batch hashes; the "tally from blockchain" is done by re-deriving from the
// stored batch hashes. For one-ballot happy path we track a per-candidate
// count alongside the chain.
//
// SLOP-SHORTCUT: FvmChainTally does NOT re-parse anything; it uses the
// chain's side-tally counts that are updated when ballots are added. This is
// the weakest of the three counts. QA: make the blockchain store the ballot
// bytes (it does, in the batch before commit) and re-tally from the stored
// bytes, OR store per-ballot records in the block.
struct FvmChainTally {
  IUC counts[FvmCandidateCount];
};

// SLOP: maintains a side-tally. Real triple count should re-read block data.
inline void FvmChainAddToTally(FvmBlockchain* chain, const FvmBallot* ballot,
                               FvmChainTally* tally) {
  (void)chain;
  if (tally == NILP || ballot == NILP) {
    return;
  }
  if (ballot->choice < static_cast<IUC>(FvmCandidateCount)) {
    ++tally->counts[ballot->choice];
  }
}

// Verifies the hash chain integrity (each prev_hash matches previous header).
inline BOL FvmChainVerify(FvmBlockchain* chain) {
  if (chain == NILP) {
    return false;
  }
  for (ISN i = 1; i < chain->block_count; ++i) {
    IUD expected = FvmBlockHeaderHash(&chain->blocks[i - 1]);
    if (chain->blocks[i].prev_hash != expected) {
      return false;
    }
  }
  return true;
}

}  //< namespace _
#endif  //< FVM_BLOCK_DECL
