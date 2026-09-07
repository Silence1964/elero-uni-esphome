# Elero UNI 868 MHz for ESPHome / ESP-IDF

> **AI-assisted reverse engineering**
>
> This project was created through reverse engineering with the assistance of artificial intelligence (AI). AI was used extensively to analyze captured radio transmissions, compare bit patterns, identify protocol structures, formulate hypotheses, and assist in developing and reviewing the decoder and transmitter implementation.
>
> The reverse-engineered results were not accepted solely on the basis of AI-generated analysis. Findings were validated through repeated captures and practical tests with real hardware. The goal of publishing this project is to document the results openly and save others from having to repeat the same reverse-engineering work from scratch.

Open-source documentation and a working ESPHome / ESP-IDF reference transmitter for the **unidirectional Elero UNI 868 MHz protocol** using an ESP32 and CC1101.

> **Status: working hardware-validated implementation.**
>
> The transmitter has been validated end-to-end with real Elero UNI receivers: pairing a newly created virtual sender, UP, STOP, DOWN, repeated commands, independent sender identities and persistent rolling indexes all work in the tested installation.
>
> This is still a community reverse-engineering project, not an official Elero implementation. Hardware families that were not tested may behave differently.

## Features

- ESP32 + CC1101 transmitter at approximately **868.300 MHz**
- synchronous serial **2-FSK** transmission
- reverse-engineered 65-logical-bit UNI frame format
- CODE64 encoder and decoder
- UP, STOP, DOWN and P/programming commands
- press/release telegram generation
- validated 3/3 repeats for normal commands and 8/3 for P/programming
- multiple independent virtual sender slots
- 16-bit rolling-index persistence in ESP32 NVS
- **pre-TX reservation:** PRESS and RELEASE indexes are committed before the first RF bit is transmitted
- ESPHome example exposing buttons and diagnostics to Home Assistant
- offline CODE64 decoder and regression tests

## Hardware

Validated with:

- classic ESP32 DevKit / ESP32-D0WD-V3
- 868 MHz CC1101 module
- ESPHome **2026.8.x**
- ESP-IDF framework

### Wiring

| CC1101 | ESP32 | Purpose |
|---|---:|---|
| SCK | GPIO18 | SPI clock |
| SO / MISO | GPIO19 | SPI MISO / ready handshake |
| SI / MOSI | GPIO23 | SPI MOSI |
| CSN | GPIO17 | chip select |
| GDO0 | GPIO16 | synchronous serial DATA |
| GDO2 | GPIO27 | synchronous serial CLOCK |
| VCC | 3.3 V | supply |
| GND | GND | ground |

Use an 868 MHz CC1101 board and antenna. Do **not** power the CC1101 from 5 V.

See [docs/HARDWARE.md](docs/HARDWARE.md) for details.

## Quick start

1. Copy `components/elero_uni/` next to your ESPHome YAML.
2. Start from [`examples/elero_uni_single.yaml`](examples/elero_uni_single.yaml).
3. Before pairing, change the example three-byte sender identity to your own unique value.
4. Keep a unique NVS key for every virtual sender.
5. Flash the ESP32 and verify that `Elero UNI radio ready` is ON.
6. Check the logged / exposed next rolling index.
7. Follow [docs/PAIRING.md](docs/PAIRING.md).
8. After pairing, test UP, STOP and DOWN.

The reference header initializes SPI2 itself. Do not add a separate ESPHome `spi:` block for the same bus in the simple example.

## Example sender configuration

The example registers one sender during `on_boot`:

```cpp
elero_uni_remote::configure_remote(
    0,                 // slot
    "Blind 1",         // display/log name
    "blind1_next",     // unique NVS key
    1,                 // initial NEXT index, used only on first creation
    0x20,              // tested AstroTec-like TYPE
    0x70, 0x55, 0xC0  // example sender ID - CHANGE BEFORE PAIRING
);
```

Up to eight sender slots are supported by the reference implementation. Each sender needs its own NVS key and sender identity.

## Critical rolling-index warning

**Never reset or reuse the rolling index of a paired virtual sender.**

A physical action uses two consecutive indexes:

```text
PRESS   = N
RELEASE = N + 1
NEXT    = N + 2
```

The implementation writes and commits `NEXT` to NVS **before** RF transmission starts. If power is lost during transmission, an index may be skipped. That is intentionally safer than reusing an index that the receiver may already have seen.

Do not use `erase-flash`, delete the NVS namespace, change an existing sender's NVS key, or manually move its rolling index backwards unless you intentionally want to abandon/re-pair that virtual sender.

## Pairing

The tested learning sequence is documented in [docs/PAIRING.md](docs/PAIRING.md).

In the validated setup, a virtual sender was learned by putting the receiver into learning mode using an already paired physical transmitter and then sending P/programming followed by the required direction confirmations.

## Protocol documentation

Detailed reverse-engineering notes are in [docs/PROTOCOL.md](docs/PROTOCOL.md), including:

- RF configuration and CC1101 register values
- 65-bit logical frame layout
- biphase mapping (`0 -> 01`, `1 -> 10`)
- rolling-key formula
- plaintext byte layout
- command bytes
- FLAGS parity nibble
- CODE64 scrambling / substitution algorithm
- observed AstroTec-like and VarioTel UNI behavior

## Repository layout

- `components/elero_uni/elero_uni_protocol.h` - pure CODE64 encoder/decoder
- `components/elero_uni/elero_uni_remote.h` - working ESP32 + CC1101 transmitter
- `examples/elero_uni_single.yaml` - ESPHome / Home Assistant example
- `docs/PROTOCOL.md` - reverse-engineered wire protocol
- `docs/HARDWARE.md` - tested hardware setup
- `docs/PAIRING.md` - pairing procedure and safety notes
- `tools/decode_code64.py` - offline CODE64 decoder
- `tests/test_vectors.py` - protocol regression tests

## Scope and limitations

This repository covers the **unidirectional Elero UNI** protocol tested here. It does not implement the longer bidirectional Elero packet format used by other product families / USB transceiver implementations.

The tested virtual senders use an AstroTec-like `TYPE = 0x20` identity family. The full manufacturer allocation rules for TYPE and sender IDs are not known, so arbitrary combinations should not be assumed valid on every receiver.

Receiver rolling-window size and every possible resynchronization behavior are also not fully characterized. Contributions with clean captures from additional hardware are welcome.

## Contributions

Especially useful contributions are:

- independent validation on additional Elero UNI receivers
- clean RF captures from other UNI transmitter models
- TYPE / sender-ID observations
- rolling-window and resynchronization tests
- additional CODE64 regression vectors

Please distinguish directly observed behavior from hypotheses and state which hardware was used.

## License / attribution

Main project: MIT. See [LICENSE](LICENSE).

The nibble substitution / scrambling foundation was informed by the MIT-licensed `QuadCorei8085/elero_protocol` project. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
