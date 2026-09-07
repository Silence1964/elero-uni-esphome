# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

- Independent testing on additional Elero UNI hardware is welcome.
- Further characterization of TYPE / sender-ID allocation and receiver rolling windows remains open research.

## 0.2.0 - 2026-09-07

First hardware-validated transmitter release.

### Added

- Complete ESP32 / ESP-IDF + CC1101 Elero UNI transmitter implementation.
- Validated 868.300 MHz synchronous serial 2-FSK CC1101 configuration.
- Safe NVS-backed rolling-index handling for multiple virtual sender slots.
- Atomic pre-TX reservation of PRESS and RELEASE indexes before RF transmission.
- UP, STOP, DOWN and P/programming transmit helpers.
- 3 PRESS / 3 RELEASE repetitions for normal commands.
- 8 PRESS / 3 RELEASE repetitions for P/programming.
- ESPHome example with Home Assistant buttons and diagnostic entities.
- Runtime CC1101 presence/configuration checks.
- CODE64 self-test during radio setup.

### Validated

- Pairing of newly created independent virtual sender identities with real Elero UNI receivers.
- UP, STOP and DOWN control from the generated virtual senders.
- Repeated command sequences without losing rolling-index synchronization.
- Persistence of the next rolling index across firmware operation/reboots.
- Multi-sender operation in the completed installation.
- Final implementation integrated successfully into the production ESPHome bridge used for the hardware tests.

### Changed

- Repository status changed from protocol-preview to working implementation.
- README now contains installation and example configuration instructions.
- The previously withheld transmitter header and ready-to-use ESPHome example are now published.

### Safety

- A command consumes two rolling indexes: PRESS uses `N`, RELEASE uses `N+1`, and `N+2` is committed to NVS before transmission begins.
- Existing NVS values always take precedence over the configured initial seed.
- Documentation explicitly warns against flash erasure, rolling-index rollback, or reuse after pairing.

### Known limitations

- This remains an unofficial community reverse-engineering project.
- Only the unidirectional Elero UNI family tested by this project is in scope.
- Complete TYPE / sender-ID allocation semantics are not known.
- Receiver rolling-window size and every resynchronization edge case are not fully characterized.

## 0.1.0-protocol-preview - 2026-09-07

First public documentation preview of the reverse-engineered unidirectional Elero 868 MHz protocol.

### Added

- Initial public repository structure and MIT license.
- Transparent disclosure that artificial intelligence was used extensively as an assistant during the reverse-engineering process.
- Documentation of the approximately 868.300 MHz RF configuration used during validation.
- Documentation of synchronous serial 2-FSK transmission with the CC1101.
- Documentation of the 65-logical-bit Elero UNI frame structure.
- Documentation of the biphase / Manchester-like symbol mapping.
- Documentation of the 64-bit encoded payload (`CODE64`).
- Reverse-engineered 16-bit rolling-index to key formula.
- Reverse-engineered command bytes for UP, STOP, DOWN and P/programming.
- Reverse-engineered press/release frame behavior.
- Reverse-engineered four-bit parity nibble used in the FLAGS byte.
- Documentation of observed AstroTec-like and VarioTel UNI sender behavior.
- Hardware notes for the tested ESP32 DevKit + CC1101 setup.
- Pairing observations and rolling-index safety warnings.
- Standalone CODE64 encoder/decoder reference implementation.
- Offline CODE64 decoder tool.
- Protocol regression test vectors.
- Third-party attribution for prior MIT-licensed Elero protocol work that informed part of the scrambling analysis.

### Validated

- Protocol findings were derived from captured RF transmissions and repeatedly checked against real hardware.
- Generated UNI frames were accepted by real Elero UNI receivers during development.
- Independent virtual sender identities could be learned by tested receivers when sender data, FLAGS and rolling indexes were generated consistently.
