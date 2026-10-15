# Fault Codes Reference

Every fault in CortexActuate maps to a bit in the `fault_bitmask` field of telemetry records.

## Fault Bitmask Bits

| Bit | Code | Name | Condition | Deadline | State Transition |
|-----|------|------|-----------|----------|------------------|
| 0x01 | 1 | HEARTBEAT_TIMEOUT | No valid command CAN frame for >100ms | 100ms | ACTIVE→DEGRADED |
| 0x02 | 2 | SENSOR_OUT_OF_RANGE | Feedback outside [-100, 100] range | 1 loop (~1ms) | ACTIVE→DEGRADED |
| 0x04 | 4 | SENSOR_RATE_EXCEEDED | Feedback change rate >50 units/sec | 1 loop (~1ms) | ACTIVE→DEGRADED |
| 0x08 | 8 | REDUNDANT_DISAGREEMENT | Secondary sensor disagrees with primary | (Reserved for future use) | ACTIVE→DEGRADED |
| 0x10 | 16 | BAD_CRC | Command frame CRC check failed | 1 frame (~10ms) | ACTIVE→DEGRADED |
| 0x20 | 32 | BAD_SEQUENCE | Command sequence counter discontinuous | 1 frame (~10ms) | ACTIVE→DEGRADED |
| 0x40 | 64 | OUTPUT_SATURATED | Output at limit for ≥10 consecutive loops | ~10ms | ACTIVE→DEGRADED |
| 0x80 | 128 | WATCHDOG_FAULT | Loop did not complete within timeout | 1 loop (~1ms) | ACTIVE→SAFE_STATE |

## State Transitions

```
INIT
  ↓ (test complete)
SELF_TEST
  ↓ (initialization OK)
ACTIVE
  ↓ (any fault raised)
DEGRADED
  ↓ (watchdog or cumulative faults)
SAFE_STATE (latched, requires power cycle)
```

Once in SAFE_STATE:
- Output ramps to zero over 100ms
- CAN status frames show FAULTED
- Watchdog remains armed (forces reboot if loop hangs)
- Cannot transition back to ACTIVE without power cycle

## Fault Reaction Examples

### Heartbeat Timeout
1. Command frame received at t=1000ms, heartbeat timer reset
2. No command frame until t=1200ms
3. At t=1100ms, check finds 100ms silence, raises HEARTBEAT_TIMEOUT
4. State becomes DEGRADED
5. Output ramps to zero
6. CAN status frames report fault

### Bad CRC
1. Frame arrives with incorrect CRC value
2. CRC check in SafetyMonitor fails
3. Frame is rejected, BAD_SEQUENCE check is skipped for this frame
4. State becomes DEGRADED immediately
5. Next valid frame (if CRC correct) may be processed

### Output Saturation
1. PID output at ±50 units (max limit) for 10 consecutive loops
2. Saturation counter increments to 10
3. OUTPUT_SATURATED fault is raised
4. State becomes DEGRADED (does not force SAFE_STATE)
5. Can be cleared by reducing setpoint

## Safety Properties

- **Fail-safe**: All faults lead to zero output (safe state)
- **Latched**: SAFE_STATE cannot be cleared except by power cycle (Watchdog reset)
- **Independent**: Each fault has own deadline and checker
- **Deterministic**: Same input sequence produces same fault timeline (SIL verified)
