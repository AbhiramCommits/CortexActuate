/* Node State Machine — 5 states with explicit transitions */
#pragma once

#include <cstdint>

enum class NodeState : uint8_t {
    INIT = 0,
    SELF_TEST = 1,
    ACTIVE = 2,
    DEGRADED = 3,
    SAFE_STATE = 4,
};

enum class FaultCode : uint32_t {
    HEARTBEAT_TIMEOUT = (1u << 0),
    SENSOR_OUT_OF_RANGE = (1u << 1),
    SENSOR_RATE_EXCEEDED = (1u << 2),
    REDUNDANT_DISAGREEMENT = (1u << 3),
    BAD_CRC = (1u << 4),
    BAD_SEQUENCE = (1u << 5),
    OUTPUT_SATURATED = (1u << 6),
    WATCHDOG_FAULT = (1u << 7),
};

struct NodeStateMachineState {
    NodeState state;
    uint32_t fault_bitmask;
    uint64_t fault_enter_time_us;
    uint64_t last_valid_command_time_us;
};

class NodeStateMachine {
private:
    NodeStateMachineState state_data_{
        .state = NodeState::INIT,
        .fault_bitmask = 0,
        .fault_enter_time_us = 0,
        .last_valid_command_time_us = 0,
    };

public:
    NodeState state() const { return state_data_.state; }
    uint32_t fault_bitmask() const { return state_data_.fault_bitmask; }
    uint64_t fault_enter_time() const { return state_data_.fault_enter_time_us; }
    void set_last_valid_command_time(uint64_t t) { state_data_.last_valid_command_time_us = t; }

    // Transition: INIT -> SELF_TEST
    void enter_self_test() {
        if (state_data_.state == NodeState::INIT) {
            state_data_.state = NodeState::SELF_TEST;
            state_data_.fault_bitmask = 0;
        }
    }

    // Transition: SELF_TEST -> ACTIVE
    void enter_active() {
        if (state_data_.state == NodeState::SELF_TEST) {
            state_data_.state = NodeState::ACTIVE;
            state_data_.fault_bitmask = 0;
        }
    }

    // Raise a fault and transition if needed
    void raise_fault(FaultCode code, uint64_t now_us) {
        uint32_t code_val = static_cast<uint32_t>(code);
        if (!(state_data_.fault_bitmask & code_val)) {
            state_data_.fault_bitmask |= code_val;
            state_data_.fault_enter_time_us = now_us;

            // Transition rules
            if (state_data_.state == NodeState::ACTIVE ||
                state_data_.state == NodeState::DEGRADED) {
                state_data_.state = NodeState::DEGRADED;
            }
        }
    }

    // Transition: DEGRADED -> SAFE_STATE (one-way)
    void enter_safe_state(uint64_t now_us) {
        if (state_data_.state != NodeState::SAFE_STATE) {
            state_data_.state = NodeState::SAFE_STATE;
            state_data_.fault_enter_time_us = now_us;
        }
    }

    // Check if in safe state (latched)
    bool is_safe_state() const { return state_data_.state == NodeState::SAFE_STATE; }
    bool is_active() const { return state_data_.state == NodeState::ACTIVE; }
};
