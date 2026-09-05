#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Andre Fiedler
#
# SPDX-License-Identifier: AGPL-3.0-or-later

"""Wrap a TI ARM CGT Intel HEX image in an RDX Manager update container.

The fixed 62,110-byte envelope is validated against a pinned compatibility
template. OpenRDX stores an explicit format marker and the SHA-256 digest of
the complete TI boot region in that envelope. The manifest marks whether
mode-05 activation is direct or must be completed through the ROM loader.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


RUNTIME_BASE = 0x08000000
FIRMWARE_PAYLOAD_LENGTH = 0xF100
CONTAINER_LENGTH = 0xF29E
BOOT_REGION_OFFSET = 0x018C
BOOT_REGION_END = 0xF29A
CUSTOM_AUTH_OFFSET = 0x0108
CUSTOM_AUTH_LENGTH = 0x0080
CUSTOM_AUTH_MAGIC = b"OPENRDX1"
METADATA_CHECK_OFFSET = 0x0188
METADATA_CHECK_END = 0x018C
COMPATIBILITY_TEMPLATE_SHA256 = (
    "73d528801aefc032d3a53637b035f65d72809151b2f127c6b2050e9bacc76f3b"
)
RDX_CHECK_POLYNOMIAL = 0x04C11DB7


def sha256(data: bytes) -> str:
    """Return the lowercase SHA-256 digest for *data*."""

    return hashlib.sha256(data).hexdigest()


def rdx_rolling_check(data: bytes, initial: int) -> int:
    """Calculate the RDX Manager envelope's polynomial metadata check."""

    value = initial
    for byte in data:
        working = ((byte ^ (value >> 24)) << 24) & 0xFFFFFFFF
        for _ in range(8):
            working = (working << 1) & 0xFFFFFFFF
            if working & 0x80000000:
                working ^= RDX_CHECK_POLYNOMIAL
        value = working ^ ((value << 8) & 0xFFFFFFFF)
    return value


def parse_intel_hex(source: Path) -> dict[int, int]:
    """Parse the Intel HEX record types emitted by TI ARM CGT armhex."""

    memory: dict[int, int] = {}
    upper_address = 0
    eof_seen = False
    for line_number, raw_line in enumerate(
        source.read_text(encoding="ascii").splitlines(), start=1
    ):
        line = raw_line.strip()
        if not line.startswith(":"):
            raise ValueError(f"line {line_number}: missing Intel HEX colon")
        try:
            record = bytes.fromhex(line[1:])
        except ValueError as exc:
            raise ValueError(f"line {line_number}: invalid hexadecimal data") from exc
        if len(record) < 5 or len(record) != record[0] + 5:
            raise ValueError(f"line {line_number}: invalid Intel HEX record length")
        if sum(record) & 0xFF:
            raise ValueError(f"line {line_number}: invalid Intel HEX checksum")

        byte_count = record[0]
        address = (record[1] << 8) | record[2]
        record_type = record[3]
        payload = record[4 : 4 + byte_count]
        if record_type == 0x00:
            absolute = upper_address + address
            for index, value in enumerate(payload):
                location = absolute + index
                if location in memory and memory[location] != value:
                    raise ValueError(
                        f"line {line_number}: conflicting byte at 0x{location:08X}"
                    )
                memory[location] = value
        elif record_type == 0x01:
            if byte_count != 0:
                raise ValueError(f"line {line_number}: malformed end record")
            eof_seen = True
            break
        elif record_type == 0x04:
            if byte_count != 2 or address != 0:
                raise ValueError(
                    f"line {line_number}: malformed extended linear address"
                )
            upper_address = int.from_bytes(payload, "big") << 16
        else:
            raise ValueError(
                f"line {line_number}: unsupported Intel HEX record type {record_type}"
            )
    if not eof_seen:
        raise ValueError("Intel HEX input has no end-of-file record")
    if not memory:
        raise ValueError("Intel HEX input contains no data records")
    return memory


def build_runtime_payload(memory: dict[int, int]) -> tuple[bytes, int]:
    """Build the fixed-size TI firmware payload and return its used length."""

    first_address = min(memory)
    last_address = max(memory)
    if first_address != RUNTIME_BASE:
        raise ValueError(
            f"firmware begins at 0x{first_address:08X}; expected 0x{RUNTIME_BASE:08X}"
        )
    used_length = last_address - RUNTIME_BASE + 1
    if used_length > FIRMWARE_PAYLOAD_LENGTH:
        raise ValueError(
            f"firmware uses 0x{used_length:X} bytes; maximum is "
            f"0x{FIRMWARE_PAYLOAD_LENGTH:X}"
        )
    payload = bytearray([0xFF] * FIRMWARE_PAYLOAD_LENGTH)
    for address, value in memory.items():
        if address < RUNTIME_BASE or address >= RUNTIME_BASE + len(payload):
            raise ValueError(f"firmware byte 0x{address:08X} is outside the image")
        payload[address - RUNTIME_BASE] = value
    return bytes(payload), used_length


def build_ti_boot_region(payload: bytes) -> bytes:
    """Build descriptor-zero plus type-2 TUSB9261 boot records."""

    if len(payload) != FIRMWARE_PAYLOAD_LENGTH:
        raise ValueError("runtime payload must be exactly 0xF100 bytes")
    descriptor_record = b"\x01" + struct.pack("<I", 0) + b"\x00"
    firmware_record = (
        b"\x02"
        + struct.pack("<I", len(payload))
        + payload
        + bytes([sum(payload) & 0xFF])
    )
    boot_region = b"\x60\x92" + descriptor_record + firmware_record
    if len(boot_region) != BOOT_REGION_END - BOOT_REGION_OFFSET:
        raise AssertionError("internal TUSB9261 boot-region length mismatch")
    return boot_region


def build_container(template: bytes, payload: bytes) -> bytes:
    """Build an integrity-marked OpenRDX container from the pinned template."""

    if len(template) != CONTAINER_LENGTH:
        raise ValueError(
            f"template length is {len(template)}; expected {CONTAINER_LENGTH}"
        )
    if sha256(template) != COMPATIBILITY_TEMPLATE_SHA256:
        raise ValueError("template does not match the pinned compatibility image")

    result = bytearray(template)
    boot_region = build_ti_boot_region(payload)
    result[BOOT_REGION_OFFSET:BOOT_REGION_END] = boot_region
    boot_check = rdx_rolling_check(boot_region, 0)
    result[BOOT_REGION_END:CONTAINER_LENGTH] = struct.pack(">I", boot_check)
    result[CUSTOM_AUTH_OFFSET:CUSTOM_AUTH_OFFSET + CUSTOM_AUTH_LENGTH] = (
        bytes(CUSTOM_AUTH_LENGTH)
    )
    result[CUSTOM_AUTH_OFFSET:CUSTOM_AUTH_OFFSET + len(CUSTOM_AUTH_MAGIC)] = (
        CUSTOM_AUTH_MAGIC
    )
    digest_offset = CUSTOM_AUTH_OFFSET + len(CUSTOM_AUTH_MAGIC)
    result[digest_offset:digest_offset + 32] = hashlib.sha256(boot_region).digest()
    metadata_check = rdx_rolling_check(
        result[:METADATA_CHECK_OFFSET], 0xFFFFFFFF
    )
    result[METADATA_CHECK_OFFSET:METADATA_CHECK_END] = struct.pack(
        ">I", metadata_check
    )
    return bytes(result)


def main() -> None:
    """Command-line entry point."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("firmware_hex", type=Path)
    parser.add_argument("template", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()

    memory = parse_intel_hex(args.firmware_hex)
    payload, used_length = build_runtime_payload(memory)
    container = build_container(args.template.read_bytes(), payload)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(container)

    manifest = {
        "container": str(args.output.resolve()),
        "container_length": len(container),
        "container_sha256": sha256(container),
        "firmware_hex": str(args.firmware_hex.resolve()),
        "runtime_base": f"0x{RUNTIME_BASE:08X}",
        "runtime_used_length": used_length,
        "runtime_padded_length": len(payload),
        "runtime_sha256": sha256(payload),
        "boot_region_sha256": sha256(
            container[BOOT_REGION_OFFSET:BOOT_REGION_END]
        ),
        "authentication_scheme": "openrdx-sha256-v1",
        "requires_openrdx_receiver": True,
        "installation_requires_rom_loader": True,
        "template_sha256": COMPATIBILITY_TEMPLATE_SHA256,
    }
    flashburner_hex = args.firmware_hex.with_name(
        f"{args.firmware_hex.stem}_flash{args.firmware_hex.suffix}"
    )
    if flashburner_hex.is_file():
        manifest["flashburner_hex"] = str(flashburner_hex.resolve())
        manifest["flashburner_hex_sha256"] = sha256(
            flashburner_hex.read_bytes()
        )
    if args.manifest is not None:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    print(json.dumps(manifest, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
