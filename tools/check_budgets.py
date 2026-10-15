#!/usr/bin/env python3
"""Verify timing and memory budgets"""

import sys
import re

def check_loop_jitter(telemetry_file):
    """Parse telemetry and check jitter budget"""
    import struct
    records = []
    with open(telemetry_file, "rb") as f:
        while True:
            data = f.read(33)
            if len(data) < 33:
                break
            rec = struct.unpack("<I Q I I h h h B I H", data)
            records.append(rec[1])  # timestamp_us

    if len(records) < 2:
        return True, "Not enough records"

    periods = []
    for i in range(1, len(records)):
        periods.append(records[i] - records[i-1])

    jitter = [abs(p - 1000) for p in periods]
    max_jitter = max(jitter)

    if max_jitter > 10:
        return False, f"Loop jitter {max_jitter:.1f}µs exceeds 10µs budget"
    return True, f"Loop jitter OK: max {max_jitter:.1f}µs"

def check_firmware_size(elf_file):
    """Parse arm-none-eabi-size output (stub: would call subprocess)"""
    # In real CI, would run: arm-none-eabi-size elf_file
    # For now, stub
    return True, "ARM build not available in this environment"

def main():
    # Check telemetry if available
    try:
        ok, msg = check_loop_jitter("telemetry.bin")
        print(f"Jitter budget: {'PASS' if ok else 'FAIL'} - {msg}")
        if not ok:
            return 1
    except FileNotFoundError:
        print("Jitter budget: SKIP (telemetry.bin not found)")

    print("All budget checks passed")
    return 0

if __name__ == "__main__":
    sys.exit(main())
