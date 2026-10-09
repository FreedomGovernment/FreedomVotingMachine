// Copyright AStarship <https://astarship.net>.
//
// FVMAudit.h -- Script-protocol message passing for audit events.
//
// SLOP-SHORTCUT: This is an in-memory byte-buffer implementation of the
// BIn/BOut byte-ring protocol. It does NOT link the real ASCIICrabs BIn.h /
// BOut.h / BSeq.h. The message format is a simplified B-Sequence:
//   [1-byte event type][4-byte length][payload bytes]
// The "ring" is a fixed IUA buffer with a write cursor (BOut side) and a read
// cursor (BIn side). This proves the serialize -> ring -> deserialize path
// works, but it is NOT the real Script protocol.
#pragma once
#ifndef FVM_AUDIT_DECL
#define FVM_AUDIT_DECL

#include "FVMTypes.h"
#include "FVMStrings.h"

namespace _ {

// Audit event types (1-byte codes).
enum FvmAuditEvent {
  FvmEvtBallotCast = 1,    //< A ballot was cast.
  FvmEvtCountAgree = 2,    //< Triple-count agreement result.
  FvmEvtBlockAppended = 3, //< A block was appended to the chain.
  FvmEvtLedgerLine = 4,    //< A paper ledger line was written.
};

// The in-memory byte-ring (BOut write side + BIn read side).
// SLOP: single ring buffer, write cursor and read cursor, no wraparound
// handling (assumes the reader drains before the writer wraps).
struct FvmByteRing {
  IUA buffer[4096];
  ISN write_pos;  //< BOut side: where the next byte goes.
  ISN read_pos;   //< BIn side: where the next byte comes from.
};

inline void FvmRingInit(FvmByteRing* ring) {
  ring->write_pos = 0;
  ring->read_pos = 0;
}

// BOut side: writes a byte into the ring.
inline ERC FvmRingWriteByte(FvmByteRing* ring, IUA byte) {
  if (ring == NILP) {
    return FvmErrBadInput;
  }
  if (ring->write_pos >= static_cast<ISN>(sizeof(ring->buffer))) {
    return FvmErrFull;
  }
  ring->buffer[ring->write_pos] = byte;
  ++ring->write_pos;
  return FvmStatusOk;
}

// BOut side: writes a byte buffer.
inline ERC FvmRingWriteBytes(FvmByteRing* ring, const IUA* data, IUN length) {
  for (IUN i = 0; i < length; ++i) {
    ERC rc = FvmRingWriteByte(ring, data[i]);
    if (rc != FvmStatusOk) {
      return rc;
    }
  }
  return FvmStatusOk;
}

// BIn side: reads a byte from the ring.
inline ERC FvmRingReadByte(FvmByteRing* ring, IUA* out_byte) {
  if (ring == NILP || out_byte == NILP) {
    return FvmErrBadInput;
  }
  if (ring->read_pos >= ring->write_pos) {
    return FvmErrGeneric;  // Empty.
  }
  *out_byte = ring->buffer[ring->read_pos];
  ++ring->read_pos;
  return FvmStatusOk;
}

// Serializes an audit event into the ring (BOut side).
// Format: [1-byte type][4-byte little-endian length][payload].
inline ERC FvmAuditEmit(FvmByteRing* ring, IUA event_type,
                        const IUA* payload, IUN payload_len) {
  ERC rc = FvmRingWriteByte(ring, event_type);
  if (rc != FvmStatusOk) {
    return rc;
  }
  IUA len_bytes[4];
  len_bytes[0] = static_cast<IUA>(payload_len & 0xFF);
  len_bytes[1] = static_cast<IUA>((payload_len >> 8) & 0xFF);
  len_bytes[2] = static_cast<IUA>((payload_len >> 16) & 0xFF);
  len_bytes[3] = static_cast<IUA>((payload_len >> 24) & 0xFF);
  rc = FvmRingWriteBytes(ring, len_bytes, 4);
  if (rc != FvmStatusOk) {
    return rc;
  }
  if (payload_len > 0) {
    rc = FvmRingWriteBytes(ring, payload, payload_len);
    if (rc != FvmStatusOk) {
      return rc;
    }
  }
  return FvmStatusOk;
}

// Reads one audit event from the ring (BIn side).
// Returns the event type in *out_type and copies the payload into
// out_payload (up to out_payload_size bytes). Returns FvmStatusOk or error.
inline ERC FvmAuditRead(FvmByteRing* ring, IUA* out_type,
                        IUA* out_payload, IUN out_payload_size,
                        IUN* out_payload_len) {
  if (ring == NILP || out_type == NILP || out_payload_len == NILP) {
    return FvmErrBadInput;
  }
  IUA type_byte = 0;
  ERC rc = FvmRingReadByte(ring, &type_byte);
  if (rc != FvmStatusOk) {
    return rc;
  }
  IUA len_bytes[4];
  for (IUN i = 0; i < 4; ++i) {
    rc = FvmRingReadByte(ring, &len_bytes[i]);
    if (rc != FvmStatusOk) {
      return rc;
    }
  }
  IUN payload_len = static_cast<IUN>(len_bytes[0])
      | (static_cast<IUN>(len_bytes[1]) << 8)
      | (static_cast<IUN>(len_bytes[2]) << 16)
      | (static_cast<IUN>(len_bytes[3]) << 24);
  *out_type = type_byte;
  *out_payload_len = payload_len;
  if (payload_len > out_payload_size) {
    // SLOP: truncate and note it. Real protocol would error.
    payload_len = out_payload_size;
  }
  for (IUN i = 0; i < payload_len; ++i) {
    rc = FvmRingReadByte(ring, &out_payload[i]);
    if (rc != FvmStatusOk) {
      return rc;
    }
  }
  return FvmStatusOk;
}

}  //< namespace _
#endif  //< FVM_AUDIT_DECL
