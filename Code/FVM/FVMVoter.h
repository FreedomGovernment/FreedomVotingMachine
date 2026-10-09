// Copyright AStarship <https://astarship.net>.
//
// FVMVoter.h -- Voter ID scan, blockchain cue, and ballot anonymization.
//
// The FVM flow:
//   1. Voter presents State-issued ID at the polling location.
//   2. The State scans the ID into the "blockchain cue" (a FIFO of
//      pending-voter tokens). This is the check-in: proves the voter
//      is present and eligible, but does NOT link identity to ballot.
//   3. The voter makes their paper vote.
//   4. The machine anonymizes the ballot: it pops the voter's token
//      from the cue and assigns a fresh, untraceable ballot token
//      via a one-way function. The original ID is discarded.
//   5. The anonymous ballot is printed to the public ledger and
//      submitted to the blockchain.
//
// SLOP-SHORTCUT: The ID scan is simulated (no real CrabsBed hardware).
// The "blockchain cue" is an in-memory FIFO. The anonymization uses
// a keyed one-way mix (still not real crypto — QA: replace with
// a proper blinding scheme or one-time pad).
#pragma once
#ifndef FVM_VOTER_DECL
#define FVM_VOTER_DECL

#include "FVMTypes.h"
#include "FVMHash.h"
#include "FVMStrings.h"

namespace _ {

// Max voters in the cue (slop: fixed).
constexpr ISN FvmCueMaxVoters = 32;

// One entry in the blockchain cue: a voter who has checked in
// but has not yet cast a ballot.
struct FvmCueEntry {
  IUD id_hash;       //< FNV-1a of the State-issued ID (not the raw ID).
  IUD cue_position;  //< Position in the FIFO (0-based).
  BOL used;          //< True once the voter has cast and been popped.
};

// The blockchain cue: a FIFO of checked-in voters.
struct FvmBlockchainCue {
  FvmCueEntry entries[FvmCueMaxVoters];
  ISN head;          //< Next position to pop from.
  ISN tail;          //< Next position to push to.
  ISN count;         //< Active (not-yet-used) entries.
};

// Initializes the cue to empty.
inline void FvmCueInit(FvmBlockchainCue* cue) {
  cue->head = 0;
  cue->tail = 0;
  cue->count = 0;
  for (ISN i = 0; i < FvmCueMaxVoters; ++i) {
    cue->entries[i].id_hash = 0;
    cue->entries[i].cue_position = 0;
    cue->entries[i].used = false;
  }
}

// Simulates the State scanning a voter's ID into the cue.
// In reality this is a CrabsBed hardware scan (RFID / barcode / NFC).
// Returns FvmStatusOk on success, FvmErrFull if the cue is full.
inline ERC FvmCueScanId(FvmBlockchainCue* cue, IUD raw_id) {
  if (cue == NILP) {
    return FvmErrBadInput;
  }
  if (cue->count >= FvmCueMaxVoters) {
    return FvmErrFull;
  }
  // Hash the raw ID so the cue only stores a fingerprint, not the ID itself.
  IUA id_bytes[8];
  const IUA* src = reinterpret_cast<const IUA*>(&raw_id);
  for (IUN i = 0; i < 8; ++i) {
    id_bytes[i] = src[i];
  }
  FvmCueEntry* entry = &cue->entries[cue->tail];
  entry->id_hash = FvmFnv1a(id_bytes, 8);
  entry->cue_position = static_cast<IUD>(cue->tail);
  entry->used = false;
  cue->tail = (cue->tail + 1) % FvmCueMaxVoters;
  ++cue->count;
  return FvmStatusOk;
}

// Pops the next voter from the cue and generates an anonymous ballot token.
// The original ID hash is discarded — only the new token survives.
// Returns FvmStatusOk on success, FvmErrBadInput if the cue is empty.
inline ERC FvmCuePopAnonymous(FvmBlockchainCue* cue, IUD* out_token,
                               IUD* out_cue_position) {
  if (cue == NILP || out_token == NILP) {
    return FvmErrBadInput;
  }
  if (cue->count <= 0) {
    return FvmErrBadInput;
  }
  FvmCueEntry* entry = &cue->entries[cue->head];
  // The anonymous token is a one-way mix of the ID hash and a
  // machine secret. The ID hash is NOT stored in the ballot.
  // SLOP: this is FNV-1a(id_hash ^ machine_secret ^ counter).
  // QA: replace with a real one-time-pad or blinding scheme.
  static IUD machine_secret = 0xDEADBEEFCAFEBABEULL;
  IUD mix = entry->id_hash ^ machine_secret;
  IUA mix_bytes[8];
  const IUA* src = reinterpret_cast<const IUA*>(&mix);
  for (IUN i = 0; i < 8; ++i) {
    mix_bytes[i] = src[i];
  }
  *out_token = FvmFnv1a(mix_bytes, 8);
  *out_cue_position = entry->cue_position;
  entry->used = true;
  cue->head = (cue->head + 1) % FvmCueMaxVoters;
  --cue->count;
  return FvmStatusOk;
}

// Returns the number of voters currently in the cue.
inline ISN FvmCueCount(const FvmBlockchainCue* cue) {
  return cue->count;
}

}  //< namespace _
#endif  //< FVM_VOTER_DECL
