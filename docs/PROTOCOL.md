# Reverse-engineered Elero UNI protocol

This document describes the unidirectional 868 MHz frames validated by direct
RF capture and by generated transmissions accepted by real receivers.

## 1. RF layer

Validated CC1101 settings include:

- center frequency: about **868.300 MHz**
- modulation: **2-FSK**
- synchronous serial mode
- modem clock: about **2.4 kHz**
- GDO2: serial clock
- GDO0: serial data

The reference transmitter uses these important register values:

```text
FREQ2/FREQ1/FREQ0 = 21 65 6A
MDMCFG4            = 56
MDMCFG3            = 83
MDMCFG2            = 00
DEVIATN             = 43
IOCFG2              = 0B
IOCFG0              = 0C
PKTCTRL0            = 12
```

Treat the complete register set in the reference code as the authoritative
configuration for this project.

## 2. Logical UNI frame

A normal frame is:

```text
idle prefix + 65 logical bits
```

Validated idle prefix:

```text
00001111
```

The 65 logical bits are expanded to two physical serial bits each:

```text
logical 0 -> 01
logical 1 -> 10
```

Logical bit 0 is fixed to zero. Logical bits 1..64 are the 64-bit encoded
payload (`CODE64`), MSB first.

Therefore a frame contributes:

```text
8 + (65 * 2) = 138 serial clock bits
```

Normal button transmissions repeat the same press frame several times, followed
by a release frame using the next rolling index. The validated reference uses
3 press repeats and 3 release repeats for normal motion commands. P/programming
uses 8 press repeats and 3 release repeats.

## 3. Rolling key

Each frame consumes a full 16-bit rolling index:

```text
key = (0x0000 - (index * 0x708F)) & 0xFFFF
```

The two key bytes are:

```text
KEY_H = key >> 8
KEY_L = key & 0xFF
```

A physical button action consumes two consecutive indexes:

```text
PRESS   = index N
RELEASE = index N+1
next    = index N+2
```

The project persists `next` before the first RF bit of PRESS is transmitted.

## 4. Effective plaintext

Before scrambling, the 8-byte message is:

```text
[0] KEY_H
[1] KEY_L
[2] TYPE
[3] COMMAND
[4] FLAGS
[5] ID0
[6] ID1
[7] ID2
```

For a release frame, COMMAND is `0x00`.

### Commands validated in UNI captures

```text
STOP = 0x10
UP   = 0x20
DOWN = 0x40
P    = 0x80
RELEASE command = 0x00
```

A VarioTel UNI channel was also observed with a constant low command bit `0x08`
(e.g. `0x18`, `0x28`, `0x48`, `0x88`). This appears to be transmitter-family /
channel behavior and should not be blindly mixed with the AstroTec-like type
used by the reference virtual sender.

## 5. FLAGS byte

The low nibble is constant `0x7` in the captured UNI family used by this
reference implementation.

The upper nibble is four pair-parity bits:

```text
bit7 = parity(KEY_H) ^ parity(KEY_L)
bit6 = parity(TYPE)  ^ parity(COMMAND)
bit5 = parity(0x07)  ^ parity(ID0)
bit4 = parity(ID1)   ^ parity(ID2)
```

`parity(x)` means the parity (popcount modulo 2) of one byte.

Then:

```text
FLAGS = (parity_nibble << 4) | 0x07
```

This detail was critical: changing a sender ID without recomputing the complete
FLAGS nibble produces frames that look structurally plausible but are rejected
by the receiver.

## 6. Scrambling / CODE64 encoder

Start from the effective plaintext above.

### Step A - nibble-wise addition

Initialize:

```text
r20 = 0xFE
```

For each byte `d` from byte 0 to byte 7:

```text
low  = (d + r20) & 0x0F
high = ((d & 0xF0) + (r20 & 0xF0)) & 0xF0
byte = high | low
r20  = (r20 - 0x22) & 0xFF
```

### Step B - alternating XOR

XOR bytes 2..7 with the original key bytes:

```text
byte 2 ^= KEY_H
byte 3 ^= KEY_L
byte 4 ^= KEY_H
byte 5 ^= KEY_L
byte 6 ^= KEY_H
byte 7 ^= KEY_L
```

### Step C - nibble substitution

Encode every high and low nibble using:

```text
input : 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
output: 8  2  D  1  F  E  7  5  9  C  0  A  3  4  B  6
```

The resulting eight bytes are `CODE64`.

## 7. Decoder

Reverse the substitution table, undo the first two nibble additions to recover
KEY_H/KEY_L, undo the alternating XOR on bytes 2..7, then subtract the remaining
nibble offsets starting at `0xBA` from bytes 2..7.

The included `tools/decode_code64.py` performs this operation offline.

## 8. Sender identity observations

Two distinct transmitter families were captured:

```text
AstroTec-like:
TYPE = 0x20
three-byte sender identity
commands 10/20/40/80, release 00

VarioTel UNI:
TYPE changed with the selected channel in the tested transmitter
three-byte transmitter identity remained constant
rolling index was global across the tested channels
commands carried an additional low 0x08 bit
```

The exact manufacturer allocation/checking rules for TYPE and sender identity
are not fully known. The project successfully paired two independently created
AstroTec-like virtual senders by giving them unique IDs and correctly computing
FLAGS.

## 9. What is deliberately not claimed

- This is not a complete specification for all Elero products.
- This does not describe the longer bidirectional Elero packet format.
- The full semantics of TYPE and address allocation are not yet known.
- Receiver rolling-window size and all resynchronization behavior are not yet
  characterized.

Contributions with additional clean RF captures are welcome.
