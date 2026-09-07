#!/usr/bin/env python3
"""Offline decoder for the reverse-engineered Elero UNI CODE64 payload."""

import argparse

DECODE = [0x0A,0x03,0x01,0x0C,0x0D,0x07,0x0F,0x06,
          0x00,0x08,0x0B,0x0E,0x09,0x02,0x05,0x04]


def sub_magic(d: int, r20: int) -> int:
    lo = (d - r20) & 0x0F
    hi = ((d & 0xF0) - (r20 & 0xF0)) & 0xF0
    return hi | lo


def decode(code64: int):
    msg = [(code64 >> (56 - 8*i)) & 0xFF for i in range(8)]
    msg = [(DECODE[x >> 4] << 4) | DECODE[x & 0x0F] for x in msg]

    r20 = 0xFE
    for i in range(2):
        msg[i] = sub_magic(msg[i], r20)
        r20 = (r20 - 0x22) & 0xFF

    key_hi, key_lo = msg[0], msg[1]
    for i in range(2, 8):
        msg[i] ^= key_lo if (i & 1) else key_hi

    r20 = 0xBA
    for i in range(2, 8):
        msg[i] = sub_magic(msg[i], r20)
        r20 = (r20 - 0x22) & 0xFF

    return msg


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("code64", help="16 hex digits, with or without 0x")
    args = ap.parse_args()
    value = int(args.code64, 16)
    msg = decode(value)
    print("plaintext:", " ".join(f"{x:02X}" for x in msg))
    print(f"key:       {msg[0]:02X}{msg[1]:02X}")
    print(f"type:      {msg[2]:02X}")
    print(f"command:   {msg[3]:02X}")
    print(f"flags:     {msg[4]:02X}")
    print(f"sender-id: {msg[5]:02X}:{msg[6]:02X}:{msg[7]:02X}")


if __name__ == "__main__":
    main()
