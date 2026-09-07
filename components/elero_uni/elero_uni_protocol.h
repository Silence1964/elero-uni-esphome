#pragma once

#include <cstdint>

namespace elero_uni {

static constexpr uint8_t CMD_STOP = 0x10;
static constexpr uint8_t CMD_UP   = 0x20;
static constexpr uint8_t CMD_DOWN = 0x40;
static constexpr uint8_t CMD_P    = 0x80;

static constexpr uint8_t ENCODE_NIBBLE[16] = {
    0x08, 0x02, 0x0D, 0x01, 0x0F, 0x0E, 0x07, 0x05,
    0x09, 0x0C, 0x00, 0x0A, 0x03, 0x04, 0x0B, 0x06};

static constexpr uint8_t DECODE_NIBBLE[16] = {
    0x0A, 0x03, 0x01, 0x0C, 0x0D, 0x07, 0x0F, 0x06,
    0x00, 0x08, 0x0B, 0x0E, 0x09, 0x02, 0x05, 0x04};

struct Plaintext {
  uint8_t key_hi;
  uint8_t key_lo;
  uint8_t type;
  uint8_t command;
  uint8_t flags;
  uint8_t id0;
  uint8_t id1;
  uint8_t id2;
};

struct EncodedFrame {
  uint64_t code64;
  uint16_t key;
  uint8_t flags;
};

inline uint8_t parity8(uint8_t v) {
  v ^= v >> 4;
  v ^= v >> 2;
  v ^= v >> 1;
  return v & 1U;
}

inline uint16_t key_from_index(uint16_t index) {
  return static_cast<uint16_t>(
      0U - static_cast<uint16_t>(index * 0x708FU));
}

inline uint8_t flags_for(uint16_t key, uint8_t type, uint8_t command,
                         uint8_t id0, uint8_t id1, uint8_t id2) {
  const uint8_t key_hi = static_cast<uint8_t>(key >> 8);
  const uint8_t key_lo = static_cast<uint8_t>(key & 0xFF);
  const uint8_t pn =
      static_cast<uint8_t>((parity8(key_hi) ^ parity8(key_lo)) << 3) |
      static_cast<uint8_t>((parity8(type) ^ parity8(command)) << 2) |
      static_cast<uint8_t>((parity8(0x07) ^ parity8(id0)) << 1) |
      static_cast<uint8_t>((parity8(id1) ^ parity8(id2)) << 0);
  return static_cast<uint8_t>((pn << 4) | 0x07);
}

inline EncodedFrame encode_code64(uint16_t index, uint8_t command,
                                  bool release, uint8_t type,
                                  uint8_t id0, uint8_t id1, uint8_t id2) {
  const uint16_t key = key_from_index(index);
  const uint8_t effective_command = release ? 0x00 : command;
  const uint8_t flags =
      flags_for(key, type, effective_command, id0, id1, id2);

  uint8_t msg[8] = {
      static_cast<uint8_t>(key >> 8),
      static_cast<uint8_t>(key & 0xFF),
      type,
      effective_command,
      flags,
      id0,
      id1,
      id2,
  };

  const uint8_t xor0 = msg[0];
  const uint8_t xor1 = msg[1];

  uint8_t r20 = 0xFE;
  for (uint8_t i = 0; i < 8; i++) {
    const uint8_t d = msg[i];
    const uint8_t ln = static_cast<uint8_t>((d + r20) & 0x0F);
    const uint8_t hn = static_cast<uint8_t>(
        ((d & 0xF0) + (r20 & 0xF0)) & 0xF0);
    msg[i] = static_cast<uint8_t>(hn | ln);
    r20 = static_cast<uint8_t>(r20 - 0x22);
  }

  for (uint8_t i = 2; i < 8; i += 2) {
    msg[i] ^= xor0;
    msg[i + 1] ^= xor1;
  }

  for (uint8_t i = 0; i < 8; i++) {
    msg[i] = static_cast<uint8_t>(
        (ENCODE_NIBBLE[msg[i] >> 4] << 4) |
        ENCODE_NIBBLE[msg[i] & 0x0F]);
  }

  uint64_t code64 = 0;
  for (uint8_t i = 0; i < 8; i++)
    code64 = (code64 << 8) | msg[i];

  return {code64, key, flags};
}

inline uint8_t sub_nibble_magic(uint8_t d, uint8_t r20) {
  const uint8_t ln = static_cast<uint8_t>((d - r20) & 0x0F);
  const uint8_t hn = static_cast<uint8_t>(
      ((d & 0xF0) - (r20 & 0xF0)) & 0xF0);
  return static_cast<uint8_t>(hn | ln);
}

inline Plaintext decode_code64(uint64_t code64) {
  uint8_t msg[8]{};
  for (uint8_t i = 0; i < 8; i++)
    msg[i] = static_cast<uint8_t>((code64 >> (56U - i * 8U)) & 0xFFU);

  for (uint8_t i = 0; i < 8; i++) {
    msg[i] = static_cast<uint8_t>(
        (DECODE_NIBBLE[msg[i] >> 4] << 4) |
        DECODE_NIBBLE[msg[i] & 0x0F]);
  }

  uint8_t r20 = 0xFE;
  for (uint8_t i = 0; i < 2; i++) {
    msg[i] = sub_nibble_magic(msg[i], r20);
    r20 = static_cast<uint8_t>(r20 - 0x22);
  }

  const uint8_t key_hi = msg[0];
  const uint8_t key_lo = msg[1];

  for (uint8_t i = 2; i < 8; i++)
    msg[i] ^= (i & 1U) ? key_lo : key_hi;

  r20 = 0xBA;
  for (uint8_t i = 2; i < 8; i++) {
    msg[i] = sub_nibble_magic(msg[i], r20);
    r20 = static_cast<uint8_t>(r20 - 0x22);
  }

  return {msg[0], msg[1], msg[2], msg[3],
          msg[4], msg[5], msg[6], msg[7]};
}

}  // namespace elero_uni
