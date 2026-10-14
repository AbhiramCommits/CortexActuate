/* Telemetry Format — shared between firmware and Python parser */
#pragma once

#include <cstdint>
#include <cstring>

#pragma pack(push, 1)

struct TelemetryRecord {
    uint32_t loop_count;
    uint64_t timestamp_us;
    uint32_t loop_start_jitter_us;  // Deviation from nominal 1000us
    uint32_t execution_time_us;
    int16_t setpoint_fixed;  // Fixed-point: setpoint * 256
    int16_t feedback_fixed;  // Fixed-point: feedback * 256
    int16_t output_fixed;    // Fixed-point: output * 256
    uint8_t state;           // NodeStateMachine state
    uint32_t fault_bitmask;  // Fault codes
    uint16_t stack_hwm;      // Stack high-water mark (bytes remaining)
};

#pragma pack(pop)

// Compile-time checks (with pack(1))
static_assert(sizeof(TelemetryRecord) == 33, "TelemetryRecord size must be 33 bytes");
static_assert(offsetof(TelemetryRecord, loop_count) == 0, "loop_count offset");
static_assert(offsetof(TelemetryRecord, timestamp_us) == 4, "timestamp_us offset");
static_assert(offsetof(TelemetryRecord, loop_start_jitter_us) == 12, "loop_start_jitter_us offset");
static_assert(offsetof(TelemetryRecord, execution_time_us) == 16, "execution_time_us offset");
static_assert(offsetof(TelemetryRecord, setpoint_fixed) == 20, "setpoint_fixed offset");
static_assert(offsetof(TelemetryRecord, feedback_fixed) == 22, "feedback_fixed offset");
static_assert(offsetof(TelemetryRecord, output_fixed) == 24, "output_fixed offset");
static_assert(offsetof(TelemetryRecord, state) == 26, "state offset");
static_assert(offsetof(TelemetryRecord, fault_bitmask) == 27, "fault_bitmask offset");
static_assert(offsetof(TelemetryRecord, stack_hwm) == 31, "stack_hwm offset");

// Python struct format string: "<I Q I I h h h B I H"
// I=uint32, Q=uint64, h=int16, B=uint8, H=uint16
