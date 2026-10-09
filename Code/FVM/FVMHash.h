// Copyright AStarship <https://astarship.net>.
//
// FVMHash.h -- Slop-level FNV-1a hash for the FVM blockchain.
//
// SLOP-SHORTCUT: Uses FNV-1a (32-bit offset basis + prime) instead of the
// real Crabs Hash.hpp / BigInt. FNV-1a is collision-prone for real crypto;
// QA must replace with a proper hash before any real deployment. Also note
// the real core's Hash.hpp has a pre-existing compiler warning about a
// 128-bit integer constant (see QA finding #1).
#pragma once
#ifndef FVM_HASH_DECL
#define FVM_HASH_DECL

#include "FVMTypes.h"

namespace _ {

// FNV-1a constants (32-bit).
constexpr IUD FvmFNVOffsetBasis = 2166136261U;
constexpr IUD FvmFNVPrime = 16777619U;

// Hashes a byte buffer with FNV-1a. Returns the 32-bit hash.
inline IUD FvmFnv1a(const IUA* data, IUN length) {
  IUD hash = FvmFNVOffsetBasis;
  for (IUN i = 0; i < length; ++i) {
    hash ^= static_cast<IUD>(data[i]);
    hash *= FvmFNVPrime;
  }
  return hash;
}

// Convenience: hash a CHA C-string.
inline IUD FvmFnv1a(const CHA* text) {
  IUN length = 0;
  for (; text[length] != 0; ++length) {
    // count chars
  }
  return FvmFnv1a(reinterpret_cast<const IUA*>(text), length);
}

// Hashes a byte buffer into a fixed-size IUA digest buffer (little-endian
// 4-byte FNV-1a result, zero-padded to digest_bytes).
inline BOL FvmHashInto(const IUA* data, IUN length, IUA* digest, IUN
                       digest_bytes) {
  IUD hash = FvmFnv1a(data, length);
  for (IUN i = 0; i < digest_bytes; ++i) {
    if (i < 4) {
      digest[i] = static_cast<IUA>((hash >> (8 * i)) & 0xFF);
    } else {
      digest[i] = 0;
    }
  }
  return true;
}

}  //< namespace _
#endif  //< FVM_HASH_DECL
