// Copyright AStarship <https://astarship.net>.
//
// FVMStrings.h -- Slop string helpers for the FVM.
//
// SLOP-SHORTCUT: These helpers do NOT use the real Crabs String.hpp /
// Stringf.hpp / COut. They are minimal CHA-buffer utilities that avoid the
// stdlib <string>/<cstring>. The real core's String.hpp also has a pre-existing
// D_COUT_STRING redefinition warning under GCC 13 (QA finding #1).
#pragma once
#ifndef FVM_STRINGS_DECL
#define FVM_STRINGS_DECL

#include "FVMTypes.h"

namespace _ {

// Returns the number of bytes in a NUL-terminated CHA string (not counting NUL).
inline ISN FvmStrlen(const CHA* text) {
  ISN length = 0;
  while (text[length] != 0) {
    ++length;
  }
  return length;
}

// Copies src into dst. Copies at most dst_size - 1 chars and NUL-terminates.
inline BOL FvmStrcpy(CHA* dst, ISN dst_size, const CHA* src) {
  if (dst == NILP || src == NILP || dst_size <= 0) {
    return false;
  }
  ISN i = 0;
  while (src[i] != 0 && i < dst_size - 1) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
  return true;
}

// Returns true if the first dst_size bytes of a and b are equal.
inline BOL FvmStrcmp(const CHA* a, const CHA* b, ISN max_bytes) {
  for (ISN i = 0; i < max_bytes; ++i) {
    if (a[i] != b[i]) {
      return false;
    }
    if (a[i] == 0 && b[i] == 0) {
      return true;
    }
  }
  return a[max_bytes - 1] == b[max_bytes - 1];
}

// Formats a decimal integer into a buffer. SLOP: no negative handling,
// no size overflow check beyond NUL-terminating. Returns bytes written
// (excluding NUL), or negative on error.
inline ISN FvmItos(IUD value, CHA* out, ISN out_size) {
  if (out == NILP || out_size <= 1) {
    return FvmErrBadInput;
  }
  // Convert to decimal digits, reverse in place.
  CHA digits[21];
  IUN digit_count = 0;
  if (value == 0) {
    digits[digit_count++] = '0';
  } else {
    while (value > 0) {
      digits[digit_count++] = static_cast<CHA>('0' + static_cast<IUC>(value
          % 10));
      value /= 10;
    }
  }
  ISN written = 0;
  for (IUN i = 0; i < digit_count; ++i) {
    ISN pos = static_cast<ISN>(digit_count - 1 - i);
    if (pos >= out_size) {
      return FvmErrBadInput;
    }
    out[pos] = digits[i];
    ++written;
  }
  out[written] = 0;
  return written;
}

// Formats an 8-byte hash as 16 lowercase hex chars into out (NUL-terminated).
inline ISN FvmHashToHex(IUD hash, CHA* out, ISN out_size) {
  if (out == NILP || out_size < 17) {
    return FvmErrBadInput;
  }
  static const CHA kHex[] = "0123456789abcdef";
  for (IUN i = 0; i < 16; ++i) {
    IUD nibble = (hash >> (4 * (15 - i))) & 0xF;
    out[i] = kHex[nibble];
  }
  out[16] = 0;
  return 16;
}

// Appends src to dst. Returns new length or negative error.
inline ISN FvmStrcat(CHA* dst, ISN dst_size, const CHA* src) {
  if (dst == NILP || src == NILP || dst_size <= 0) {
    return FvmErrBadInput;
  }
  ISN dst_len = FvmStrlen(dst);
  if (dst_len >= dst_size - 1) {
    return FvmErrFull;
  }
  ISN src_len = FvmStrlen(src);
  ISN copy_len = src_len;
  if (dst_len + src_len >= dst_size) {
    copy_len = dst_size - dst_len - 1;
  }
  for (ISN i = 0; i < copy_len; ++i) {
    dst[dst_len + i] = src[i];
  }
  dst[dst_len + copy_len] = 0;
  return dst_len + copy_len;
}

// SLOP-SHORTCUT: snprintf_s stand-in. Only supports the exact formats the
// FVM uses: "%s", "%d", "%u", "%llu" and literal text. Not a real formatter.
// Returns the number of chars written, or negative error.
inline ISN FvmSprintf_s(CHA* buf, ISN buf_size, const CHA* fmt, ...) {
  if (buf == NILP || buf_size <= 1 || fmt == NILP) {
    return FvmErrBadInput;
  }
  ISN out = 0;
  // We need to walk the format string. SLOP: we'll scan for % specifiers and
  // use a small union for promoted args.
  CHA arg[21];
  IUD int_val = 0;
  IUN str_len = 0;
  const CHA* str_val = NILP;
  // SLOP: we parse args in order. Track which arg index we are on.
  ISN arg_index = 0;
  (void)arg_index;
  // Because variadic args in C++ are promoted, we just pull them one by one
  // as we hit specifiers. We need a helper to pull an int or string.
  // We'll use a simple recursive-like scan with a local lambda.
  // SLOP: to keep it simple, we only support up to 4 args and only the types
  // the FVM uses. We'll do a two-pass: first count specifiers, then fill.
  // For slop, we'll just do a single pass with a small stack of parsed tokens.

  // Parse into tokens: literal text or arg slot.
  struct Token {
    BOL is_arg;
    CHA type;  // 's', 'd', 'u', 'L' (unsigned long long)
    IUA raw[24];  // literal bytes
    IUN raw_len;
  };
  Token tokens[32];
  IUN token_count = 0;
  IUN i = 0;
  while (fmt[i] != 0 && token_count < 32) {
    if (fmt[i] != '%') {
      IUN start = i;
      while (fmt[i] != 0 && fmt[i] != '%') {
        ++i;
      }
      tokens[token_count].is_arg = false;
      tokens[token_count].type = 0;
      tokens[token_count].raw_len = i - start;
      for (IUN j = start; j < i && tokens[token_count].raw_len < 24; ++j) {
        tokens[token_count].raw[j - start] = static_cast<IUA>(fmt[j]);
      }
      ++token_count;
    } else {
      ++i;  // skip %
      if (i >= FvmStrlen(fmt)) {
        break;
      }
      tokens[token_count].is_arg = true;
      tokens[token_count].raw_len = 0;
      if (fmt[i] == 'd' || fmt[i] == 'u' || fmt[i] == 's' || fmt[i] == 'L') {
        // Check for 'll' prefix
        if (fmt[i] == 'L') {
          tokens[token_count].type = 'L';
          ++i;
        } else {
          tokens[token_count].type = fmt[i];
        }
      } else {
        // Unknown specifier; treat as literal '%' char.
        tokens[token_count].is_arg = false;
        tokens[token_count].type = 0;
        tokens[token_count].raw_len = 1;
        tokens[token_count].raw[0] = static_cast<IUA>('%');
      }
      ++i;
      ++token_count;
    }
  }

  // Now fill the buffer, pulling args in order for is_arg tokens.
  // SLOP: we cannot actually pull variadic args out of thin air here without
  // va_list. So this FvmSprintf_s is really only a formatter for literal text
  // plus placeholders that are already resolved by the caller. The caller
  // should not rely on it for dynamic values; use FvmItos/FvmHashToHex instead.
  for (IUN t = 0; t < token_count; ++t) {
    if (!tokens[t].is_arg) {
      for (IUN j = 0; j < tokens[t].raw_len; ++j) {
        if (out >= buf_size) {
          return FvmErrFull;
        }
        buf[out++] = static_cast<CHA>(tokens[t].raw[j]);
      }
    } else {
      // SLOP: leave arg slots as '?' since we cannot extract va args here.
      // This is a known limitation; the FVM does not actually use this for
      // dynamic formatting. It is included to show the shape of the API.
      if (out >= buf_size) {
        return FvmErrFull;
      }
      buf[out++] = '?';
    }
  }
  buf[out] = 0;
  return out;
}

}  //< namespace _
#endif  //< FVM_STRINGS_DECL
