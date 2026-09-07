# Hardware

## Tested platform

The reference implementation was validated with a classic ESP32 DevKit and a
CC1101 module intended for the 868 MHz band.

### Example wiring

| CC1101 | ESP32 example | Notes |
|---|---:|---|
| SCK | GPIO18 | SPI clock |
| SO / MISO | GPIO19 | SPI MISO + CC1101 ready handshake |
| SI / MOSI | GPIO23 | SPI MOSI |
| CSN | GPIO17 | manual chip select |
| GDO0 | GPIO16 | synchronous serial DATA |
| GDO2 | GPIO27 | synchronous serial CLOCK |
| VCC | 3.3 V | do not use 5 V |
| GND | GND | common ground |

The CC1101 runs from 3.3 V logic and supply.

## RF notes

The validated transmitter setup uses **868.300 MHz**. Antenna quality matters.
A module sold as a generic CC1101 board may be fitted with components optimized
for a different band, so use a board/antenna suitable for 868 MHz.

The current implementation configures the radio for synchronous serial 2-FSK.
GDO2 provides the modem clock; GDO0 is driven by the ESP32 with the next serial
bit on the opposite clock edge.

## ESPHome framework

Use ESPHome with:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
```

The implementation deliberately avoids Arduino-only APIs.
