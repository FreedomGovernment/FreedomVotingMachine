// Copyright AStarship <https://astarship.net>.
//
// FVMTypes.h -- FVM-specific status codes.
//
// The fixed-width ASCII-Crabs types (CHA/CHB/IUD/ISC/IUC/IUA/BOL/ISN/ERC/...)
// are NO LONGER locally redefined. They come from the real ASCIICrabs core
// via _Config.h (see the include chain below). This file keeps only what is
// FVM-specific: the status/error codes the triple-count and I/O paths return.
//
// Build: the real core is resolved through the -I include path, with the
// core root BEFORE the _Seams dir (that ordering avoids the `namespace _`
// shadowing `std` when COut.hxx pulls <iostream> inside namespace _):
//   g++ -std=gnu++23 -I <ASCIICrabs> -I <ASCIICrabs>/_Seams FVMMain.cpp
// The angle-bracket form <_Config.h> is used (not quoted "_Config.h") so the
// include resolves to the CORE's _Config.h on the -I path, NOT the stale
// _Config.h/_ConfigHeader.h copies that historically lived in Code/FVM/.
#pragma once
#ifndef FVM_TYPES_DECL
#define FVM_TYPES_DECL

// The real ASCII Crabs core type system + platform config.
// Pulls CHA, CHB, CHC, CHN, CHS, ISA, IUA, ISB, IUB, BOL, ISC, IUC, ISD,
// IUD, ISN, ISW, IUN, IUW, ERC, DTW, DTA, DTB, TMC, TMD, NILP, and the
// namespace _ enums (POD type codes, error codes, ACPUCacheLineSize, ...).
#include <_Config.h>

namespace _ {

// FVM-specific status codes.
// The core defines Success = 0 and AError* (positive) error codes; the FVM
// keeps its own signed (negative-on-error) convention for its audit trail.
// FvmStatusOk maps to the core's Success so the two agree on the success path.
enum FvmErrors {
  FvmStatusOk = 0,        //< Success (== core ::_::Success).
  FvmErrGeneric = -1,     //< Generic failure.
  FvmErrFull = -2,        //< Ledger or block buffer full.
  FvmErrBadInput = -3,    //< Malformed ballot or ledger line.
  FvmErrMismatch = -4,    //< Triple-count disagreement.
};

}  //< namespace _

#endif  //< FVM_TYPES_DECL
