# Hardware

## Tested platform

The reference transmitter was validated with a classic ESP32 DevKit and a CC1101 module intended for the 868 MHz band, using ESPHome with the ESP-IDF framework.

### Wiring

| CC1101 | ESP32 | Notes |
|---|---:|---|
| SCK | GPIO18 | SPI clock |
| SO / MISO | GPIO19 | SPI MISO + CC1101 ready handshake |
| SI / MOSI | GPIO23 | SPI MOSI |
| CSN | GPIO17 | manual chip select |
| GDO0 | GPIO16 | synchronous serial DATA |
| GDO2 | GPIO27 | synchronous serial CLOCK |
| VCC | 3.3 V | do not use 5 V |
| GND | GND | common ground |

The CC1101 uses 3.3 V supply and logic.

## RF notes

The validated transmitter setup uses approximately **868.300 MHz** and synchronous serial **2-FSK**.

A generic CC1101 board may have matching components or an antenna optimized for a different frequency band. Use a module and antenna suitable for 868 MHz.

During TX, GDO2 supplies the CC1101 synchronous modem clock and the ESP32 drives the next serial DATA bit on GDO0. The implementation uses a GPIO interrupt on GDO2 rather than software delay timing.

## SPI ownership

`components/elero_uni/elero_uni_remote.h` initializes `SPI2_HOST` directly with:

- SCK GPIO18
- MISO GPIO19
- MOSI GPIO23
- CS GPIO17

The simple ESPHome example therefore intentionally has **no separate `spi:` block**. Do not configure another device on the same SPI2 pins without adapting the code to share the bus safely.

## ESPHome framework

Use ESPHome with ESP-IDF:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
```

The implementation uses ESP-IDF GPIO, SPI, FreeRTOS and NVS APIs and does not depend on Arduino-only APIs.

## CC1101 sanity check

At startup the reference implementation reads CC1101 PARTNUM/VERSION and verifies the programmed register set. `Elero UNI radio ready` should only become true when the chip, protocol self-test, GPIO ISR and radio configuration all initialize successfully.
