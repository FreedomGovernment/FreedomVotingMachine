// Copyright AStarship <https://astarship.net>.
//
// FVMOut.h -- Console output via the real ASCII Crabs COut/StdOut().
//
// Previously this used the C runtime <cstdio> fprintf because the core did
// not yet build on this host. The real core now compiles and links on
// Linux (GCC 13, C++23), so the FVM emits its happy-path trace through the
// real COut stream:  StdOut() << item  /  StdOut().NL()  /  .Hex().
//
// Build: the implementation lives in COut.hxx (pulled in via _Package.hxx,
// the core's single-translation-unit impl umbrella). FVMMain.cpp includes
// _Package.hxx after _Config.h so these inline/defined symbols link.
#pragma once
#ifndef FVM_OUT_DECL
#define FVM_OUT_DECL

// FVMTypes.h provides the real core types (CHA/IUD/...) via _Config.h.
#include "FVMTypes.h"
// Declares the COut stream + StdOut() used below. The implementation is
// provided by _Package.hxx (included by the .cpp translation unit).
#include <COut.h>

namespace _ {

// Prints a CHA string to the console (no newline).
inline void FvmPrint(const CHA* text) {
  StdOut() << text;
}

// Prints a CHA string followed by a newline.
inline void FvmPrintln(const CHA* text) {
  StdOut() << text;
  StdOut().NL();
}

// Prints a decimal unsigned 64-bit value.
inline void FvmPrintU64(IUD value) {
  StdOut() << value;
}

// Prints a hex 64-bit value (16 lowercase digits).
inline void FvmPrintHex64(IUD value) {
  StdOut().Hex(value);
}

}  //< namespace _

#endif  //< FVM_OUT_DECL
