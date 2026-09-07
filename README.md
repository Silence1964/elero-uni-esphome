# Elero UNI 868 MHz for ESPHome / ESP-IDF

> **AI-assisted reverse engineering**
>
> This project was created through reverse engineering with the assistance of artificial intelligence (AI). AI was used extensively to analyze captured radio transmissions, compare bit patterns, identify protocol structures, formulate hypotheses, and assist in developing and reviewing the decoder and transmitter implementation.
>
> The reverse-engineered results were not accepted solely on the basis of AI-generated analysis. Findings were validated through repeated captures and practical tests with real hardware. The goal of publishing this project is to document the results openly and save others from having to repeat the same reverse-engineering work from scratch.

Community documentation and reference material for the **unidirectional Elero 868 MHz protocol** using an ESP32 and CC1101.

The goal is to document the reverse-engineered UNI protocol openly and to make the verified findings reproducible for other users and developers.

> **Current release status:** documentation / reverse-engineering preview.
>
> The protocol findings published here are based on direct RF captures and successful tests with real Elero UNI receivers. The complete ESPHome transmitter implementation and ready-to-flash YAML configuration are **not published yet**, because final integration and regression tests are still in progress.
>
> Treat the project as experimental until the implementation has been validated on additional hardware and the first implementation release is explicitly tagged.

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
- safe rolling-index persistence requirements
- successful creation and pairing of independent virtual sender identities during development

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

A paired virtual sender must never accidentally reuse a rolling index that may already have been transmitted.

The implementation currently being validated reserves and persists the next unused rolling index before RF transmission. A crash may therefore skip indexes, but must not cause reuse of an already transmitted index.

Do not experiment with arbitrary rolling-index resets on a paired receiver unless you understand the consequences and are prepared to re-pair or abandon that virtual sender identity.

## Protocol documentation

The currently published material is intended for people who want to inspect, verify, or extend the reverse-engineering work:

- [docs/PROTOCOL.md](docs/PROTOCOL.md) - reverse-engineered UNI wire protocol
- [docs/HARDWARE.md](docs/HARDWARE.md) - tested ESP32 / CC1101 hardware setup
- [docs/PAIRING.md](docs/PAIRING.md) - pairing observations and safety notes
- `components/elero_uni/elero_uni_protocol.h` - protocol encoder/decoder reference code
- `tools/decode_code64.py` - offline CODE64 decoder
- `tests/test_vectors.py` - protocol regression vectors

## Implementation status

The following parts are intentionally **not part of this preview release yet**:

- complete CC1101 transmitter implementation
- complete ESPHome integration header
- ready-to-flash YAML configuration
- final multi-sender / multi-cover example
- final Home Assistant cover configuration

These files will be published after the remaining real-hardware tests are complete.

This is intentional: the repository should not encourage users to flash an implementation that is still being actively validated.

## Scope and limitations

This repository is about **Elero UNI**, not the longer bidirectional packet format used by Elero bidirectional devices / USB transceiver implementations.

The two protocol families share some cryptographic/scrambling ideas but are not interchangeable.

The sender-address semantics are not fully reverse engineered. The tested virtual senders used an AstroTec-like `type = 0x20` identity family with unique 3-byte IDs. Do not assume every arbitrary type/address combination is valid on every receiver.

The currently documented behavior was validated against a limited number of real devices. Additional clean RF captures and independent hardware tests are welcome.

## Repository layout

- `components/elero_uni/elero_uni_protocol.h` - UNI CODE64 encoder/decoder reference
- `docs/PROTOCOL.md` - reverse-engineered wire protocol
- `docs/HARDWARE.md` - tested hardware and RF setup
- `docs/PAIRING.md` - pairing observations and warnings
- `tools/decode_code64.py` - offline CODE64 decoder
- `tests/test_vectors.py` - protocol regression tests
- `CHANGELOG.md` - project history
- `THIRD_PARTY_NOTICES.md` - third-party attribution

## Contributions

Independent verification is especially valuable. Useful contributions include:

- clean RF captures from additional Elero UNI transmitters
- captures from additional receiver families
- confirmation or correction of TYPE / sender-ID behavior
- rolling-window and resynchronization observations
- independently reproduced CODE64 test vectors

Please clearly state which hardware was used and distinguish observed facts from hypotheses.

## License / attribution

Main project: MIT. See [LICENSE](LICENSE).

The nibble substitution / scrambling foundation was informed by the MIT-licensed `QuadCorei8085/elero_protocol` project. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
