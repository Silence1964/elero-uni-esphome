#!/usr/bin/env python3
"""Small dependency-free regression test for the documented UNI codec."""

ENCODE = [0x08,0x02,0x0D,0x01,0x0F,0x0E,0x07,0x05,
          0x09,0x0C,0x00,0x0A,0x03,0x04,0x0B,0x06]
DECODE = [0x0A,0x03,0x01,0x0C,0x0D,0x07,0x0F,0x06,
          0x00,0x08,0x0B,0x0E,0x09,0x02,0x05,0x04]


def parity(v):
    return v.bit_count() & 1


def key(index):
    return (-(index * 0x708F)) & 0xFFFF


def flags(k, typ, cmd, ids):
    kh, kl = k >> 8, k & 0xFF
    p = ((parity(kh) ^ parity(kl)) << 3 |
         (parity(typ) ^ parity(cmd)) << 2 |
         (parity(0x07) ^ parity(ids[0])) << 1 |
         (parity(ids[1]) ^ parity(ids[2])))
    return (p << 4) | 0x07


def encode(index, cmd, release, typ=0x20, ids=(0x70,0x55,0xC0)):
    k = key(index)
    ecmd = 0 if release else cmd
    msg = [k >> 8, k & 0xFF, typ, ecmd, flags(k, typ, ecmd, ids), *ids]
    x0, x1 = msg[0], msg[1]
    r = 0xFE
    for i, d in enumerate(msg):
        msg[i] = ((((d & 0xF0) + (r & 0xF0)) & 0xF0) | ((d + r) & 0x0F))
        r = (r - 0x22) & 0xFF
    for i in range(2, 8, 2):
        msg[i] ^= x0
        msg[i+1] ^= x1
    msg = [(ENCODE[x >> 4] << 4) | ENCODE[x & 0x0F] for x in msg]
    out = 0
    for b in msg:
        out = (out << 8) | b
    return out


def sub_magic(d, r):
    return (((d & 0xF0) - (r & 0xF0)) & 0xF0) | ((d-r) & 0x0F)


def decode(code):
    msg = [(code >> (56 - 8*i)) & 0xFF for i in range(8)]
    msg = [(DECODE[x >> 4] << 4) | DECODE[x & 0x0F] for x in msg]
    r = 0xFE
    for i in range(2):
        msg[i] = sub_magic(msg[i], r)
        r = (r - 0x22) & 0xFF
    kh, kl = msg[0], msg[1]
    for i in range(2,8):
        msg[i] ^= kl if i & 1 else kh
    r = 0xBA
    for i in range(2,8):
        msg[i] = sub_magic(msg[i], r)
        r = (r - 0x22) & 0xFF
    return msg


def check(index, cmd, release):
    code = encode(index, cmd, release)
    plain = decode(code)
    k = key(index)
    expected_cmd = 0 if release else cmd
    assert plain[:4] == [k >> 8, k & 0xFF, 0x20, expected_cmd]
    assert plain[5:] == [0x70,0x55,0xC0]
    assert plain[4] == flags(k, 0x20, expected_cmd, (0x70,0x55,0xC0))


for idx in (1, 2, 17, 255, 2047, 65535):
    for cmd in (0x10, 0x20, 0x40, 0x80):
        check(idx, cmd, False)
        check(idx, cmd, True)

print("Elero UNI protocol regression tests: OK")
