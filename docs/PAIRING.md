# Pairing a virtual Elero UNI sender

## Before you start

- Keep the original learned transmitter available.
- Record the virtual sender ID, preference key and current rolling index.
- Do **not** reset NVS after the virtual sender has been successfully paired.
- Pair one receiver at a time.

## Procedure verified in the test installation

The receiver was put into learning mode with an already learned physical
transmitter. On a VarioTel 2 this was done on the correct individual channel by
holding **UP + DOWN + P** for about three seconds until the receiver started its
learning travel.

The virtual UNI sender was then taught during the learning travel:

1. Send **P** from the virtual sender.
2. When the receiver begins an upward learning movement, send **UP**.
3. When it begins a downward learning movement, send **DOWN**.
4. If the learning travel continues after the direction confirmations, STOP can
   be used to end the process.
5. Test normal UP / STOP / DOWN only after learning is complete.

The exact timing can be receiver/transmitter dependent. In the validated setup,
more than one attempt was sometimes needed. Do not respond by resetting the
rolling index; skipped indexes are safe, reused indexes are not.

## Why P matters

A plain normal-motion command was not sufficient to register a new virtual
sender. A UNI P/programming command is encoded in the same 65-bit UNI frame
family as normal commands; no separate bidirectional programming packet is
required for the tested UNI receiver.

## Removing a sender

Sender removal is receiver-specific and potentially destructive. This project
does not currently automate deletion. Use the official transmitter/receiver
instructions and make sure you are deleting only the intended sender/channel.
