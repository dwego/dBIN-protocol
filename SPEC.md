# dBIN/1 Specification (Draft)

This document defines the **dBIN/1** wire format.

## Bit order
- Fields are written **MSB-first** (most-significant bit first).
- Within each output byte, bits are filled from bit 7 down to bit 0.

## Frame layout (dBIN/1)
A dBIN message is:

```

[ HEADER (bit-packed) ][ padding-to-next-byte ][ MSG_BYTES (msg_len bytes) ]

```

Padding bits (if any) are zero.

## Header layout (dBIN/1)

Fields are written in the following order:

| Field     | Bits | Description |
|----------|-----:|-------------|
| magic    | 12   | Protocol identifier (fixed constant) |
| version  | 4    | Protocol version (v1 = 1) |
| type     | 3    | Message type (MSG/ACK/PING/...) |
| valid    | 1    | 1 if message is valid |
| is_room  | 1    | 1 if `route` is a room_id, 0 if `route` is a to_user_id |
| reserved | 3    | Must be 0 |
| user_id  | 20   | Sender ID (0..2^20-1) |
| route    | 20   | Destination (room_id or to_user_id) |
| msg_id   | 16   | Sequence ID used for ACK/RTT |
| msg_len  | 12   | Number of bytes in `msg_bytes` (0..4095) |

### Header size
Total header bits:  
`12 + 4 + 3 + 1 + 1 + 3 + 20 + 20 + 16 + 12 = 92 bits`

After writing the 92 header bits, the encoder aligns to the next byte boundary and writes `msg_len` bytes.

## Message types (`type`, 3 bits)
- 0: MSG
- 1: ACK
- 2: PING
- 3: PONG
(4..7 reserved)

### MSG (type=0)
- `msg_len` MUST be > 0 (may be 0 if you want to allow empty messages)
- payload is `msg_len` bytes of UTF-8 after byte alignment

### ACK (type=1)
- `msg_len` MUST be 0
- ACK confirms the `msg_id` from the header

## Constants (recommended)
- `magic`: `0xDB1` (12-bit value)
- `version`: `1`

## Validation rules
A decoder SHOULD reject frames if:
- `magic` does not match
- `version` is unsupported
- `reserved != 0`
- `msg_len > 4095`
