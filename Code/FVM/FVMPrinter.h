// Copyright AStarship <https://astarship.net>.
//
// FVMPrinter.h -- Old-school paper roll printer with privacy masking.
//
// The FVM uses an old-school dot-matrix printer with a huge paper roll.
// The public anon ledger is printed onto this roll. The privacy mechanism:
//
//   1. The roll unrolls (advances) to make room for a new line.
//   2. The machine prints ONLY the new voter's line — all other lines
//      are already on the paper. The voter sees only their own line
//      because the roll has advanced past the previous lines.
//   3. After the voter inspects their line, the roll re-rolls (retracts)
//      and the paper is sealed in a privacy container.
//
// This is "pen-and-paper" (hybrid) — the paper IS the record. The
// machine masks out other votes so the voter can only see one line
// (their own) at a time.
//
// SLOP-SHORTCUT: The "paper" is a CHA buffer in memory. The "roll
// position" is an ISN index. The "privacy container" is a BOL flag.
// QA: wire to a real CrabsBed dot-matrix driver.
#pragma once
#ifndef FVM_PRINTER_DECL
#define FVM_PRINTER_DECL

#include "FVMTypes.h"
#include "FVMStrings.h"

namespace _ {

// Max lines on the paper roll (slop: fixed).
constexpr ISN FvmRollMaxLines = 128;
// Bytes per printed line.
constexpr ISN FvmRollLineBytes = 192;

// The old-school paper roll.
struct FvmPaperRoll {
  CHA lines[FvmRollMaxLines][FvmRollLineBytes];
  ISN line_count;      //< Total lines printed so far.
  ISN roll_position;   //< Current visible line (what the voter sees).
  BOL in_container;    //< True when the paper is sealed in the privacy box.
  ISN lines_in_container;  //< Lines sealed in the container.
};

// Initializes the roll to empty.
inline void FvmRollInit(FvmPaperRoll* roll) {
  roll->line_count = 0;
  roll->roll_position = 0;
  roll->in_container = false;
  roll->lines_in_container = 0;
}

// Unrolls the paper: advances the roll to make room for a new line.
// Returns the line index where the new line will be printed.
inline ISN FvmRollUnroll(FvmPaperRoll* roll) {
  if (roll == NILP) {
    return -1;
  }
  if (roll->line_count >= FvmRollMaxLines) {
    return -1;  // Roll is full.
  }
  // Advance the roll: the new line goes at line_count.
  roll->roll_position = roll->line_count;
  return roll->roll_position;
}

// Prints a line onto the roll at the current position.
// Returns FvmStatusOk on success.
inline ERC FvmRollPrintLine(FvmPaperRoll* roll, const CHA* line, ISN line_len) {
  if (roll == NILP || line == NILP) {
    return FvmErrBadInput;
  }
  if (roll->line_count >= FvmRollMaxLines) {
    return FvmErrFull;
  }
  if (line_len >= FvmRollLineBytes) {
    line_len = FvmRollLineBytes - 1;
  }
  for (ISN i = 0; i < line_len; ++i) {
    roll->lines[roll->line_count][i] = line[i];
  }
  roll->lines[roll->line_count][line_len] = 0;
  ++roll->line_count;
  return FvmStatusOk;
}

// Masks out all lines EXCEPT the given line_index.
// This is the privacy mechanism: the voter can only see their own line.
// Returns FvmStatusOk on success.
inline ERC FvmRollMaskAllExcept(FvmPaperRoll* roll, ISN visible_line) {
  if (roll == NILP) {
    return FvmErrBadInput;
  }
  if (visible_line < 0 || visible_line >= roll->line_count) {
    return FvmErrBadInput;
  }
  // In the real FVM, this is done by the roll position: the paper
  // advances so only one line is in the "window". Here we simulate it
  // by setting roll_position to the visible line.
  roll->roll_position = visible_line;
  return FvmStatusOk;
}

// Returns the text of the line the voter can currently see.
inline const CHA* FvmRollVisibleLine(const FvmPaperRoll* roll) {
  if (roll == NILP) {
    return NILP;
  }
  if (roll->roll_position < 0 || roll->roll_position >= roll->line_count) {
    return NILP;
  }
  return roll->lines[roll->roll_position];
}

// Re-rolls the paper and seals it in the privacy container.
// After this, the paper is no longer accessible — it goes to the
// blockchain as the hash of its contents.
// Returns FvmStatusOk on success.
inline ERC FvmRollSealInContainer(FvmPaperRoll* roll) {
  if (roll == NILP) {
    return FvmErrBadInput;
  }
  roll->in_container = true;
  roll->lines_in_container = roll->line_count;
  return FvmStatusOk;
}

// Returns true if the paper is in the privacy container.
inline BOL FvmRollIsSealed(const FvmPaperRoll* roll) {
  return roll->in_container;
}

}  //< namespace _
#endif  //< FVM_PRINTER_DECL
