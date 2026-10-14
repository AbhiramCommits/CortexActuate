/* Safety Monitor — independent fault checks */
#pragma once

#include "node_state_machine.hpp"
#include <cstdint>
#include <cmath>

class SafetyMonitor {
private:
    static constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 100;
    static constexpr float SENSOR_RANGE_MIN = -100.0f;
    static constexpr float SENSOR_RANGE_MAX = 100.0f;
    static constexpr float MAX_RATE_OF_CHANGE = 50.0f;  // units/second
    static constexpr uint32_t SATURATION_PERSIST_COUNT = 10;
    static constexpr uint8_t VALID_CRC_SEED = 0xFF;

    uint64_t last_heartbeat_us_ = 0;
    float last_feedback_ = 0.0f;
    uint64_t last_feedback_time_us_ = 0;
    uint32_t saturation_count_ = 0;
    uint8_t last_sequence_ = 0;

    // CRC-8 checksum
    uint8_t crc8(const uint8_t* data, size_t len) const {
        uint8_t crc = VALID_CRC_SEED;
        for (size_t i = 0; i < len; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x07;
                } else {
                    crc <<= 1;
                }
            }
        }
        return crc;
    }

public:
    void update_heartbeat(uint64_t now_us) {
        last_heartbeat_us_ = now_us;
    }

    bool check_heartbeat_timeout(uint64_t now_us, NodeStateMachine& fsm) {
        if (fsm.is_safe_state()) return true;  // Already faulted

        uint64_t elapsed_us = now_us - last_heartbeat_us_;
        uint32_t elapsed_ms = elapsed_us / 1000;

        if (elapsed_ms > HEARTBEAT_TIMEOUT_MS) {
            fsm.raise_fault(FaultCode::HEARTBEAT_TIMEOUT, now_us);
            return false;
        }
        return true;
    }

    bool check_sensor_range(float feedback, uint64_t now_us, NodeStateMachine& fsm) {
        if (fsm.is_safe_state()) return true;

        if (feedback < SENSOR_RANGE_MIN || feedback > SENSOR_RANGE_MAX) {
            fsm.raise_fault(FaultCode::SENSOR_OUT_OF_RANGE, now_us);
            return false;
        }
        return true;
    }

    bool check_sensor_rate_of_change(float feedback, uint64_t now_us,
                                     NodeStateMachine& fsm) {
        if (fsm.is_safe_state()) return true;

        if (last_feedback_time_us_ == 0) {
            last_feedback_ = feedback;
            last_feedback_time_us_ = now_us;
            return true;
        }

        uint64_t delta_us = now_us - last_feedback_time_us_;
        if (delta_us < 100) return true;  // Skip if too soon

        float delta_feedback = feedback - last_feedback_;
        float delta_seconds = delta_us / 1e6f;
        float rate = std::abs(delta_feedback / delta_seconds);

        last_feedback_ = feedback;
        last_feedback_time_us_ = now_us;

        if (rate > MAX_RATE_OF_CHANGE) {
            fsm.raise_fault(FaultCode::SENSOR_RATE_EXCEEDED, now_us);
            return false;
        }
        return true;
    }

    bool check_crc(const uint8_t* frame, size_t len, uint8_t expected_crc,
                   uint64_t now_us, NodeStateMachine& fsm) {
        if (fsm.is_safe_state()) return true;

        uint8_t computed_crc = crc8(frame, len);
        if (computed_crc != expected_crc) {
            fsm.raise_fault(FaultCode::BAD_CRC, now_us);
            return false;
        }
        return true;
    }

    bool check_sequence(uint8_t seq, uint64_t now_us, NodeStateMachine& fsm) {
        if (fsm.is_safe_state()) return true;

        // Simple: expect seq to be last_seq + 1 (with wrap)
        uint8_t expected = (last_sequence_ + 1) & 0xFF;
        if (seq != expected) {
            fsm.raise_fault(FaultCode::BAD_SEQUENCE, now_us);
            return false;
        }
        last_sequence_ = seq;
        return true;
    }

    void check_output_saturation(float output, float max_output,
                                 uint64_t now_us, NodeStateMachine& fsm) {
        if (fsm.is_safe_state()) return;

        if (std::abs(output) >= max_output * 0.99f) {
            saturation_count_++;
            if (saturation_count_ >= SATURATION_PERSIST_COUNT) {
                fsm.raise_fault(FaultCode::OUTPUT_SATURATED, now_us);
            }
        } else {
            saturation_count_ = 0;
        }
    }

    void check_watchdog_kick(bool kicked, uint64_t now_us, NodeStateMachine& fsm) {
        if (!kicked && !fsm.is_safe_state()) {
            fsm.raise_fault(FaultCode::WATCHDOG_FAULT, now_us);
        }
    }
};
