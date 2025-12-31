# dBIN-protocol

dBIN is a small, deterministic **binary messaging protocol** designed for **high-frequency, low-overhead** real-time messaging (e.g. live chat / presence).

It uses a **bit-level packed header** to minimize per-message overhead and keep encoding/decoding cost predictable.

This repository provides:
- a reference specification [SPEC.md](SPEC.md)
- a C implementation (encoder/decoder + bit I/O)

## What is this (in one sentence)?
A custom **wire format** (bit layout) for sending messages over a socket, optimized for small messages.

## Goals
- Compact header (bit-packed)
- Deterministic layout (same on any language/platform)
- Simple to implement in C
- Good for benchmarking vs JSON for small messages

## Basic model
A message is sent as:

1) A **bit-packed header** containing metadata (who sent, where it goes, length, etc.)
2) The **message bytes** (UTF-8), after aligning to the next byte boundary

See [SPEC.md](SPEC.md) for the exact bit layout.
