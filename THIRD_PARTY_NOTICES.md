# Third-party notices

This project documents and implements Elero RF behavior independently verified
with real unidirectional 868 MHz captures.

The message scrambling / nibble substitution work was informed by the
MIT-licensed project:

- QuadCorei8085/elero_protocol
  - https://github.com/QuadCorei8085/elero_protocol
  - Copyright (c) 2023 QuadCorei8085
  - License: MIT

The full upstream MIT license is available at:
https://github.com/QuadCorei8085/elero_protocol/blob/main/LICENSE

The project `stanleypa/eleropy` was consulted as background for the separate
bidirectional packet family. It is GPL-3.0 licensed. No source code from that
project is included here; this repository intentionally provides an original
unidirectional implementation so the main codebase can remain MIT licensed.

Elero is a trademark of its respective owner. This is an independent community
project and is not affiliated with or endorsed by Elero GmbH.
