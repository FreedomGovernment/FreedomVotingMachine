// Copyright AStarship <https://astarship.net>.
//
// FVMBallot.h -- Ballot type, simulated CrabsBed input, and casting.
//
// SLOP-SHORTCUT: Simulated ballot input. Real hardware comes from CrabsBed
// (Button.h / Switch.h / RotaryKnob.h / Unicontroller.h). Here a ballot is
// produced by FvmSimulatedVoterPress() which returns a hardcoded selection.
#pragma once
#ifndef FVM_BALLOT_DECL
#define FVM_BALLOT_DECL

#include "FVMTypes.h"

namespace _ {

// SLOP: 4 candidates, fixed size. Real FVM should be configurable.
constexpr ISN FvmCandidateCount = 4;
constexpr ISN FvmTokenBytes = 32;  //< Semi-anonymous token field width.
constexpr ISN FvmBallotBytes = 128;  //< Whole ballot record width.

// One cast ballot. All-contiguous by design.
struct FvmBallot {
  IUD token;             //< Semi-anonymous hash token (not voter identity).
  IUC choice;            //< Selected candidate index (0-based).
  IUD cast_time;         //< Seconds since epoch (slop: fixed timestamp).
  IUD precinct;          //< Precinct ID (slop: always 1).
  IUD checksum;          //< FNV-1a over the other fields.
};

// SLOP: hardcoded "voter presses button for candidate 2" simulation.
inline IUC FvmSimulatedVoterPress() {
  return 2;
}

// Computes a semi-anonymous token from a (fake) voter identity + counter.
// SLOP: NOT real anonymization. This is FNV-1a(voter_secret ^ counter) so the
// same voter's ballots are linkable if the secret is known. QA: replace with
// a proper one-way function / blinding scheme.
inline IUD FvmMakeToken(IUD voter_secret, IUD counter) {
  IUD mix = (voter_secret * 0x9E3779B97F4A7C15ULL) ^ counter;
  // FNV-1a over the 8 bytes of mix.
  const IUA* bytes = reinterpret_cast<const IUA*>(&mix);
  IUD hash = 2166136261U;
  for (IUN i = 0; i < 8; ++i) {
    hash ^= bytes[i];
    hash *= 16777619U;
  }
  return hash;
}

// Casts a ballot. Fills the ballot struct from simulated input.
// Returns FvmStatusOk on success.
inline ERC FvmCastBallot(FvmBallot* ballot, IUD voter_secret, IUD ballot_number) {
  if (ballot == NILP) {
    return FvmErrBadInput;
  }
  ballot->token = FvmMakeToken(voter_secret, ballot_number);
  ballot->choice = FvmSimulatedVoterPress();
  if (ballot->choice >= static_cast<IUC>(FvmCandidateCount)) {
    return FvmErrBadInput;
  }
  // SLOP: fixed timestamp so runs are deterministic. QA: wire to real clock.
  ballot->cast_time = 1757500000ULL;
  ballot->precinct = 1;
  // Checksum over the preceding fields (token, choice, cast_time, precinct).
  IUD* words = reinterpret_cast<IUD*>(ballot);
  IUD sum = 0;
  for (IUN i = 0; i < 4; ++i) {
    sum ^= (words[i] * 0x2545F4914F6CDD1DULL);
  }
  ballot->checksum = sum;
  return FvmStatusOk;
}

// Verifies a ballot's internal checksum (tamper check).
inline BOL FvmBallotValid(const FvmBallot* ballot) {
  if (ballot == NILP) {
    return false;
  }
  IUD* words = reinterpret_cast<IUD*>(const_cast<FvmBallot*>(ballot));
  IUD sum = 0;
  for (IUN i = 0; i < 4; ++i) {
    sum ^= (words[i] * 0x2545F4914F6CDD1DULL);
  }
  return sum == ballot->checksum;
}

}  //< namespace _
#endif  //< FVM_BALLOT_DECL
