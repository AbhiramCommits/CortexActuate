/* Control Logic Tests */
#include <gtest/gtest.h>
#include <pid_controller.hpp>
#include <node_state_machine.hpp>
#include <safety_monitor.hpp>
#include <cmath>

class ControllerTest : public ::testing::Test {
protected:
    PidController pid{1.0f, 0.1f, 0.05f, 100.0f, 50.0f, 100.0f};
};

TEST_F(ControllerTest, StepResponse) {
    float output = pid.update(10.0f, 0.0f, 0);
    EXPECT_GE(output, 0.0f);
}

TEST_F(ControllerTest, AntiWindup) {
    for (uint64_t t = 0; t < 10000000; t += 1000) {
        float out = pid.update(100.0f, 0.0f, t);
        EXPECT_LE(std::abs(out), 50.0f);
    }
}

TEST_F(ControllerTest, SlewRateLimiting) {
    pid.reset();
    float prev_out = 0.0f;
    for (uint64_t t = 0; t < 100000; t += 1000) {
        float out = pid.update(50.0f, 0.0f, t);
        float delta = out - prev_out;
        EXPECT_LE(std::abs(delta), 100.0f * 0.001f + 0.1f);  // 100 units/sec + tolerance
        prev_out = out;
    }
}

class StateMachineTest : public ::testing::Test {
protected:
    NodeStateMachine fsm;
};

TEST_F(StateMachineTest, InitState) {
    EXPECT_EQ(fsm.state(), NodeState::INIT);
    EXPECT_EQ(fsm.fault_bitmask(), 0u);
}

TEST_F(StateMachineTest, Transitions) {
    fsm.enter_self_test();
    EXPECT_EQ(fsm.state(), NodeState::SELF_TEST);

    fsm.enter_active();
    EXPECT_EQ(fsm.state(), NodeState::ACTIVE);
}

TEST_F(StateMachineTest, FaultRaises) {
    fsm.enter_self_test();
    fsm.enter_active();
    uint32_t code = static_cast<uint32_t>(FaultCode::HEARTBEAT_TIMEOUT);
    fsm.raise_fault(FaultCode::HEARTBEAT_TIMEOUT, 1000);
    EXPECT_EQ(fsm.state(), NodeState::DEGRADED);
    EXPECT_TRUE(fsm.fault_bitmask() & code);
}

TEST_F(StateMachineTest, SafeStateLatch) {
    fsm.enter_self_test();
    fsm.enter_active();
    fsm.raise_fault(FaultCode::HEARTBEAT_TIMEOUT, 1000);
    fsm.enter_safe_state(2000);
    EXPECT_TRUE(fsm.is_safe_state());

    // Try to raise another fault — should stay in SAFE_STATE
    fsm.raise_fault(FaultCode::SENSOR_OUT_OF_RANGE, 3000);
    EXPECT_TRUE(fsm.is_safe_state());
}

class SafetyMonitorTest : public ::testing::Test {
protected:
    SafetyMonitor monitor;
    NodeStateMachine fsm;

    void SetUp() override {
        fsm.enter_self_test();
        fsm.enter_active();
        monitor.update_heartbeat(0);
    }
};

TEST_F(SafetyMonitorTest, HeartbeatTimeout) {
    EXPECT_TRUE(monitor.check_heartbeat_timeout(50000, fsm));
    EXPECT_FALSE(monitor.check_heartbeat_timeout(150000, fsm));
    EXPECT_TRUE(fsm.fault_bitmask() & static_cast<uint32_t>(FaultCode::HEARTBEAT_TIMEOUT));
}

TEST_F(SafetyMonitorTest, SensorRange) {
    EXPECT_TRUE(monitor.check_sensor_range(0.0f, 1000, fsm));
    EXPECT_TRUE(monitor.check_sensor_range(50.0f, 2000, fsm));
    EXPECT_FALSE(monitor.check_sensor_range(150.0f, 3000, fsm));
}

TEST_F(SafetyMonitorTest, CrcCheck) {
    uint8_t frame[] = {0x12, 0x34};
    uint8_t bad_crc = 0xFF;
    EXPECT_FALSE(monitor.check_crc(frame, 2, bad_crc, 1000, fsm));
}

TEST_F(SafetyMonitorTest, SequenceCheck) {
    EXPECT_TRUE(monitor.check_sequence(1, 1000, fsm));
    EXPECT_TRUE(monitor.check_sequence(2, 2000, fsm));
    EXPECT_FALSE(monitor.check_sequence(5, 3000, fsm));
}
