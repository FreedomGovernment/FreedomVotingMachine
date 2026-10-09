// Copyright AStarship <https://astarship.net>.
//
// FVMLedger.h -- Semi-anonymized paper ledger (one text line per ballot).
//
// SLOP-SHORTCUT: The paper ledger is a fixed-size array of CHA lines in
// memory. It is NOT written to a real SubsecondDb / PostgreSQL store. The
// Append line formats "token=... choice=... time=... precinct=..." and the
// triple-count parser re-reads this text, which is what makes count #2
// independent from the in-memory tally.
#pragma once
#ifndef FVM_LEDGER_DECL
#define FVM_LEDGER_DECL

#include "FVMTypes.h"
#include "FVMHash.h"
#include "FVMStrings.h"
#include "FVMBallot.h"

namespace _ {

constexpr ISN FvmLedgerMaxLines = 64;
constexpr ISN FvmLedgerLineBytes = 192;

// The paper ledger: a contiguous block of text lines.
struct FvmPaperLedger {
  CHA lines[FvmLedgerMaxLines][FvmLedgerLineBytes];
  ISN line_count;
  // The text representation of the whole ledger, used as the "paper" source
  // of truth for the triple count.
  CHA text[FvmLedgerMaxLines * FvmLedgerLineBytes];
  ISN text_length;
};

// Formats one ballot into a paper line:
//   "B0000000000000000 token=abcd... choice=2 time=1757500000 precinct=1"
inline ISN FvmBallotToLine(const FvmBallot* ballot, ISN line_index, CHA* line,
                           ISN line_size) {
  ISN pos = 0;
  // Line ID: "B" + 16 hex chars of the token.
  CHA token_hex[17];
  FvmHashToHex(ballot->token, token_hex, sizeof(token_hex));
  line[pos++] = 'B';
  for (ISN i = 0; i < 16; ++i) {
    line[pos++] = token_hex[i];
  }
  line[pos++] = ' ';
  line[pos++] = 't';
  line[pos++] = 'o';
  line[pos++] = 'k';
  line[pos++] = 'e';
  line[pos++] = 'n';
  line[pos++] = '=';
  for (ISN i = 0; i < 16; ++i) {
    if (pos >= line_size) {
      return FvmErrFull;
    }
    line[pos++] = token_hex[i];
  }
  line[pos++] = ' ';
  line[pos++] = 'c';
  line[pos++] = 'h';
  line[pos++] = 'o';
  line[pos++] = 'i';
  line[pos++] = 'c';
  line[pos++] = 'e';
  line[pos++] = '=';
  if (pos >= line_size) {
    return FvmErrFull;
  }
  line[pos++] = static_cast<CHA>('0' + static_cast<IUC>(ballot->choice));
  line[pos++] = ' ';
  line[pos++] = 't';
  line[pos++] = 'i';
  line[pos++] = 'm';
  line[pos++] = 'e';
  line[pos++] = '=';
  ISN digits = FvmItos(ballot->cast_time, line + pos, line_size - pos);
  if (digits < 0) {
    return FvmErrFull;
  }
  pos += digits;
  line[pos++] = ' ';
  line[pos++] = 'p';
  line[pos++] = 'r';
  line[pos++] = 'e';
  line[pos++] = 'c';
  line[pos++] = 'i';
  line[pos++] = 'n';
  line[pos++] = 'c';
  line[pos++] = 't';
  line[pos++] = '=';
  digits = FvmItos(ballot->precinct, line + pos, line_size - pos);
  if (digits < 0) {
    return FvmErrFull;
  }
  pos += digits;
  line[pos] = 0;
  (void)line_index;
  return pos;
}

// Appends a ballot to the paper ledger. Updates the text blob too.
inline ERC FvmLedgerAppend(FvmPaperLedger* ledger, const FvmBallot* ballot) {
  if (ledger == NILP || ballot == NILP) {
    return FvmErrBadInput;
  }
  if (ledger->line_count >= FvmLedgerMaxLines) {
    return FvmErrFull;
  }
  ISN pos = FvmBallotToLine(ballot, ledger->line_count,
                            ledger->lines[ledger->line_count],
                            FvmLedgerLineBytes);
  if (pos < 0) {
    return FvmErrFull;
  }
  // Append the same line to the text blob (with newline).
  if (ledger->text_length + pos + 1 >= static_cast<ISN>(
      sizeof(ledger->text))) {
    return FvmErrFull;
  }
  for (ISN i = 0; i < pos; ++i) {
    ledger->text[ledger->text_length + i] = ledger->lines[ledger->line_count][i];
  }
  ledger->text[ledger->text_length + pos] = '\n';
  ledger->text_length += pos + 1;
  ++ledger->line_count;
  return FvmStatusOk;
}

// Initializes the ledger to empty.
inline void FvmLedgerInit(FvmPaperLedger* ledger) {
  ledger->line_count = 0;
  ledger->text_length = 0;
  ledger->text[0] = 0;
}

// Parses the paper ledger text and tallies choices into counts[4].
// SLOP parser: scans for "choice=N" in each line. This is the independence
// property: it reads the TEXT, not the FvmBallot struct.
inline ERC FvmLedgerTally(const FvmPaperLedger* ledger, IUC* counts) {
  if (ledger == NILP || counts == NILP) {
    return FvmErrBadInput;
  }
  for (ISN c = 0; c < FvmCandidateCount; ++c) {
    counts[c] = 0;
  }
  // Walk the text line by line.
  ISN i = 0;
  while (i < ledger->text_length && ledger->text[i] != 0) {
    // Find "choice="
    if (ledger->text[i] == 'c' && i + 7 <= ledger->text_length) {
      const CHA* marker = "choice=";
      BOL match = true;
      for (ISN m = 0; m < 7; ++m) {
        if (ledger->text[i + m] != marker[m]) {
          match = false;
          break;
        }
      }
      if (match) {
        CHA digit = ledger->text[i + 7];
        if (digit >= '0' && digit <= '9') {
          IUC choice = static_cast<IUC>(digit - '0');
          if (choice < static_cast<IUC>(FvmCandidateCount)) {
            ++counts[choice];
          }
        }
        i += 7;
      }
    }
    ++i;
  }
  return FvmStatusOk;
}

}  //< namespace _
#endif  //< FVM_LEDGER_DECL
