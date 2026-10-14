#!/usr/bin/env python3
"""Generate CAN traces for SIL testing"""

import csv
import sys

def make_frame(setpoint, seq):
    """Create a CAN frame payload"""
    cmd_type = 0x64
    reserved = 0x00
    setpoint_int = min(int(setpoint * 2), 200)
    crc_data = bytes([cmd_type, reserved, setpoint_int, seq & 0xFF])
    crc = 0xFF
    for b in crc_data:
        crc ^= b
    return f"{cmd_type:02x}{reserved:02x}{setpoint_int:02x}{seq:02x}{crc:02x}000000"

def gen_nominal_drive():
    """30-second normal command stream at 100 Hz"""
    traces = []
    for i in range(3000):
        t_us = i * 10000  # 100 Hz
        setpoint = min(i / 60.0, 50.0)  # Ramp over 60 frames
        seq = (i + 1) % 256  # Start seq at 1, not 0
        payload = make_frame(setpoint, seq)
        traces.append((t_us, 0x100, 8, payload))
    return traces

def gen_heartbeat_dropout():
    """Commands stop at 15 seconds"""
    traces = []
    for i in range(1500):  # 15 seconds of valid commands
        t_us = i * 10000
        setpoint = min(i / 60.0, 50.0)
        seq = (i + 1) % 256
        payload = make_frame(setpoint, seq)
        traces.append((t_us, 0x100, 8, payload))
    # Then silence — watchdog should fire around 15.1s
    return traces

def gen_sensor_glitch():
    """Normal commands (sensor glitch is detected on receiver side)"""
    traces = []
    for i in range(2000):
        t_us = i * 10000
        setpoint = min(i / 60.0, 50.0)
        seq = (i + 1) % 256
        payload = make_frame(setpoint, seq)
        traces.append((t_us, 0x100, 8, payload))
    return traces

def gen_corrupt_crc():
    """CRC corruption at 10 seconds (frame 1000)"""
    traces = []
    for i in range(2000):
        t_us = i * 10000
        setpoint = min(i / 60.0, 50.0)
        setpoint_int = min(int(setpoint * 2), 200)
        seq = (i + 1) % 256
        # Corrupt CRC at frame 1000
        if i == 1000:
            payload = f"64{0x00:02x}{setpoint_int:02x}{seq:02x}FF000000"  # Bad CRC
        else:
            payload = make_frame(setpoint, seq)
        traces.append((t_us, 0x100, 8, payload))
    return traces

def write_trace(filename, traces):
    """Write traces to CSV"""
    import os
    os.makedirs("sil/traces", exist_ok=True)
    with open(f"sil/traces/{filename}", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["timestamp_us", "id", "dlc", "payload_hex"])
        for t_us, can_id, dlc, payload in traces:
            w.writerow([t_us, hex(can_id), dlc, payload])

if __name__ == "__main__":
    write_trace("nominal_drive.csv", gen_nominal_drive())
    write_trace("heartbeat_dropout.csv", gen_heartbeat_dropout())
    write_trace("sensor_glitch.csv", gen_sensor_glitch())
    write_trace("corrupt_crc.csv", gen_corrupt_crc())
    print("Generated 4 traces in sil/traces/")
