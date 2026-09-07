# Changelog

All notable changes to this project will be documented in this file.

The project is currently in an experimental reverse-engineering phase. Until the complete transmitter implementation is published and independently validated, releases should be treated as previews rather than production-ready software.

## Unreleased

### In progress

- Final real-hardware validation of the ESP32 + CC1101 transmitter implementation.
- Regression testing of UP, STOP, DOWN and P/programming transmissions.
- Validation of rolling-index persistence across reboots, crashes and firmware updates.
- Multi-sender / multi-cover testing.
- Final ESPHome and Home Assistant integration examples.

### Intentionally not published yet

- Complete CC1101 transmitter implementation.
- Complete ESPHome integration header.
- Ready-to-flash ESPHome YAML configuration.
- Final multi-sender / multi-cover example.
- Final Home Assistant cover configuration.

These files will be published only after the remaining hardware tests have been completed successfully.

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

### Changed

- Repository positioning changed from a complete public-release draft to a **documentation / reverse-engineering preview**.
- Ready-to-flash ESPHome example configuration was removed until final integration testing is complete.
- README now clearly separates verified protocol findings from the still-under-test transmitter implementation.

### Known limitations

- This is not a complete specification for every Elero product family.
- Bidirectional Elero packet formats are outside the scope of this repository.
- Full TYPE and sender-ID allocation semantics are not yet known.
- Receiver rolling-window size and all resynchronization behavior are not yet fully characterized.
- The complete ESPHome/CC1101 transmitter implementation is not part of this preview release.
