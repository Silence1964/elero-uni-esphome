# Elero UNI 868 MHz for ESPHome / ESP-IDF

Community documentation and a reference implementation for the **unidirectional
Elero 868 MHz protocol** using an ESP32 and CC1101.

The goal is to save other users from having to repeat the RF reverse-engineering
work required to create a virtual Elero UNI transmitter.

> **Status:** public-release draft. The protocol encoder and the ESP-IDF/CC1101
> TX path are based on a setup that successfully paired and controlled real
> Elero UNI receivers. The repository should still be treated as experimental
> until it has been tested on more hardware variants.

## What is already understood

- 868.300 MHz CC1101 configuration used by the tested installation
- synchronous serial 2-FSK TX
- 65 logical-bit UNI frame layout
- biphase / Manchester-like symbol mapping
- 16-bit rolling index and key formula
- 64-bit payload encoding and decoding
- command bytes for UP, STOP, DOWN and P/programming
- 4-bit parity nibble in the FLAGS byte
- press/release frame behavior
- safe rolling-index persistence before RF transmission
- successful creation and pairing of independent virtual sender identities

See [docs/PROTOCOL.md](docs/PROTOCOL.md) for the protocol details.

## Hardware used for validation

- ESP32 DevKit (classic ESP32 / ESP32-D0WD-V3)
- CC1101 868 MHz module
- ESPHome with the **ESP-IDF** framework

Example pinout:

| Signal | ESP32 |
|---|---:|
| SCK | GPIO18 |
| MISO | GPIO19 |
| MOSI | GPIO23 |
| CS | GPIO17 |
| GDO0 / serial data | GPIO16 |
| GDO2 / serial clock | GPIO27 |
| VCC | 3.3 V |
| GND | GND |

See [docs/HARDWARE.md](docs/HARDWARE.md).

## Critical rolling-code warning

After a virtual sender has been paired, **do not reset its sender identity or
rolling index**. This reference implementation reserves and persists the next
unused index *before* RF transmission. A crash can therefore skip indexes, but
must never reuse an index that may already have been transmitted.

Do not use `erase-flash`, factory reset, or a changed preference key on a paired
virtual sender unless you intentionally want to abandon that sender identity.

## Quick start

1. Read [docs/HARDWARE.md](docs/HARDWARE.md).
2. Copy the example YAML and config header from `examples/`.
3. Choose a **unique virtual sender identity** and preference key.
4. Flash with ESPHome / ESP-IDF.
5. Verify the boot log and next rolling index.
6. Follow [docs/PAIRING.md](docs/PAIRING.md).
7. Only after pairing, test UP / STOP / DOWN.

The example is intentionally explicit rather than magical: users should be able
to see exactly which sender identity and persistent key they are committing to.

## Scope and limitations

This repository is about **Elero UNI**, not the longer bidirectional packet
format used by Elero bidirectional devices / USB transceiver implementations.
The two protocol families share some cryptographic/scrambling ideas but are not
interchangeable.

The sender-address semantics are not fully reverse engineered. The tested
virtual senders used an AstroTec-like `type = 0x20` identity family with unique
3-byte IDs. Do not assume every arbitrary type/address combination is valid on
every receiver.

## Repository layout

- `components/elero_uni/elero_uni_protocol.h` - pure UNI CODE64 encoder/decoder
- `components/elero_uni/elero_uni_remote.h` - ESPHome/ESP-IDF + CC1101 reference TX
- `examples/` - single virtual transmitter example
- `docs/PROTOCOL.md` - reverse-engineered wire protocol
- `docs/PAIRING.md` - safe pairing procedure
- `tools/decode_code64.py` - offline CODE64 decoder
- `tests/test_vectors.py` - protocol regression tests

## License / attribution

Main project: MIT. See [LICENSE](LICENSE).

The nibble substitution / scrambling foundation was informed by the MIT-licensed
`QuadCorei8085/elero_protocol` project. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
