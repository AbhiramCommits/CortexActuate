#!/usr/bin/env python3
"""Parse and analyze telemetry binary logs"""

import struct
import sys
from collections import defaultdict

def parse_telemetry(filename):
    """Read binary telemetry records (33 bytes each)"""
    records = []
    format_str = "<I Q I I h h h B I H"  # see telemetry_format.hpp
    record_size = 33

    try:
        with open(filename, "rb") as f:
            while True:
                data = f.read(record_size)
                if len(data) < record_size:
                    break
                rec = struct.unpack(format_str, data)
                records.append({
                    'loop_count': rec[0],
                    'timestamp_us': rec[1],
                    'loop_start_jitter_us': rec[2],
                    'execution_time_us': rec[3],
                    'setpoint_fixed': rec[4],
                    'feedback_fixed': rec[5],
                    'output_fixed': rec[6],
                    'state': rec[7],
                    'fault_bitmask': rec[8],
                    'stack_hwm': rec[9],
                })
    except Exception as e:
        print(f"Error reading {filename}: {e}", file=sys.stderr)
        return []

    return records

def state_name(state_byte):
    """Convert state byte to name"""
    states = {0: "INIT", 1: "SELF_TEST", 2: "ACTIVE", 3: "DEGRADED", 4: "SAFE_STATE"}
    return states.get(state_byte, f"UNKNOWN({state_byte})")

def fault_names(fault_bitmask):
    """Convert fault bitmask to list of fault names"""
    faults = {
        0x01: "HEARTBEAT_TIMEOUT",
        0x02: "SENSOR_OUT_OF_RANGE",
        0x04: "SENSOR_RATE_EXCEEDED",
        0x08: "REDUNDANT_DISAGREEMENT",
        0x10: "BAD_CRC",
        0x20: "BAD_SEQUENCE",
        0x40: "OUTPUT_SATURATED",
        0x80: "WATCHDOG_FAULT",
    }
    names = []
    for bit, name in faults.items():
        if fault_bitmask & bit:
            names.append(name)
    return names or ["NONE"]

def analyze(records):
    """Analyze telemetry records"""
    if not records:
        print("No records found")
        return

    print(f"Total records: {len(records)}")
    print(f"Time span: {records[0]['timestamp_us']/1e6:.2f}s to {records[-1]['timestamp_us']/1e6:.2f}s")

    # Loop period statistics
    periods = []
    for i in range(1, len(records)):
        period = records[i]['timestamp_us'] - records[i-1]['timestamp_us']
        periods.append(period)

    if periods:
        periods.sort()
        p50 = periods[len(periods)//2]
        p99 = periods[int(len(periods)*0.99)]
        p999 = periods[int(len(periods)*0.999)]
        mean = sum(periods) / len(periods)

        print(f"\nLoop Period (µs):")
        print(f"  Mean: {mean:.1f}")
        print(f"  Median (p50): {p50:.1f}")
        print(f"  p99: {p99:.1f}")
        print(f"  p99.9: {p999:.1f}")
        print(f"  Min: {min(periods):.1f}, Max: {max(periods):.1f}")

    # Jitter (deviation from nominal 1000 µs)
    jitter = [abs(p - 1000) for p in periods]
    if jitter:
        print(f"\nLoop Jitter (µs, deviation from 1000 µs):")
        print(f"  Max: {max(jitter):.1f}")
        print(f"  p99: {sorted(jitter)[int(len(jitter)*0.99)]:.1f}")

    # Execution time
    exec_times = [r['execution_time_us'] for r in records if r['execution_time_us'] > 0]
    if exec_times:
        print(f"\nExecution Time (µs):")
        print(f"  Mean: {sum(exec_times)/len(exec_times):.1f}")
        print(f"  Max: {max(exec_times):.1f}")

    # State transitions
    state_counts = defaultdict(int)
    for r in records:
        state_counts[r['state']] += 1
    print(f"\nState Distribution:")
    for state in sorted(state_counts.keys()):
        print(f"  {state_name(state)}: {state_counts[state]} records")

    # Faults
    faults_seen = defaultdict(int)
    for r in records:
        for fault in fault_names(r['fault_bitmask']):
            faults_seen[fault] += 1
    if any(fault != "NONE" for fault in faults_seen.keys()):
        print(f"\nFaults Detected:")
        for fault, count in sorted(faults_seen.items()):
            if fault != "NONE":
                print(f"  {fault}: {count} records")

    # Stack high-water mark
    stack_hwm = [r['stack_hwm'] for r in records]
    print(f"\nStack High-Water Mark (bytes free):")
    print(f"  Min: {min(stack_hwm)}")
    print(f"  Max: {max(stack_hwm)}")

    # First fault time
    for i, r in enumerate(records):
        if r['fault_bitmask'] != 0:
            print(f"\nFirst Fault at record {i}: {fault_names(r['fault_bitmask'])[0]} (t={r['timestamp_us']/1e6:.3f}s)")
            break

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: telemetry_report.py <telemetry.bin>")
        sys.exit(1)

    records = parse_telemetry(sys.argv[1])
    analyze(records)
