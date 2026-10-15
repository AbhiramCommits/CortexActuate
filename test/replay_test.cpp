/* Trace Replay Tests */
#include <algorithm>
#include <cstring>
#include <gtest/gtest.h>
#include <node_state_machine.hpp>
#include <pid_controller.hpp>
#include <safety_monitor.hpp>

class ReplayTest : public ::testing::Test {
protected:
  NodeStateMachine fsm;
  SafetyMonitor monitor;
  PidController controller{1.0f, 0.1f, 0.05f, 100.0f, 50.0f, 100.0f};

  void SetUp() override {
    fsm.enter_self_test();
    fsm.enter_active();
    monitor.update_heartbeat(0);
  }
};

TEST_F(ReplayTest, NominalNoFaults) {
  // Simulate nominal trace: 100 Hz commands for 30 seconds
  for (int i = 0; i < 3000; ++i) {
    uint64_t t_us = i * 10000;
    float setpoint = std::min(i / 60.0f, 50.0f);

    // Heartbeat
    monitor.update_heartbeat(t_us);

    // Run control loop every ~1000 us
    if (i % 1 == 0) {
      float feedback = setpoint * 0.8f; // Simulate ramp
      monitor.check_heartbeat_timeout(t_us, fsm);
      monitor.check_sensor_range(feedback, t_us, fsm);
      controller.update(setpoint, feedback, t_us);
    }
  }

  EXPECT_TRUE(fsm.is_active());
  EXPECT_EQ(fsm.fault_bitmask(), 0);
}

TEST_F(ReplayTest, HeartbeatTimeout) {
  // 1500 frames @ 100 Hz, then silence for > 100 ms
  for (int i = 0; i < 1500; ++i) {
    uint64_t t_us = i * 10000;
    monitor.update_heartbeat(t_us);
    monitor.check_heartbeat_timeout(t_us, fsm);
  }

  EXPECT_TRUE(fsm.is_active());

  // After 100+ ms silence, should timeout
  uint64_t t_timeout = 1500 * 10000 + 150000; // +150 ms
  monitor.check_heartbeat_timeout(t_timeout, fsm);
  EXPECT_TRUE(fsm.fault_bitmask() &
              static_cast<uint32_t>(FaultCode::HEARTBEAT_TIMEOUT));
}

TEST_F(ReplayTest, BadCrcDetection) {
  uint8_t frame[4] = {0x64, 0x00, 0x32, 0x00};
  uint8_t bad_crc = 0xFF;

  monitor.check_crc(frame, 4, bad_crc, 1000, fsm);
  EXPECT_TRUE(fsm.fault_bitmask() & static_cast<uint32_t>(FaultCode::BAD_CRC));
}

TEST_F(ReplayTest, SequenceGap) {
  EXPECT_TRUE(monitor.check_sequence(1, 1000, fsm));
  EXPECT_TRUE(monitor.check_sequence(2, 2000, fsm));
  EXPECT_FALSE(monitor.check_sequence(5, 3000, fsm)); // Gap: expected 3, got 5
  EXPECT_TRUE(fsm.fault_bitmask() &
              static_cast<uint32_t>(FaultCode::BAD_SEQUENCE));
}

TEST_F(ReplayTest, DeterministicRun) {
  // Run same scenario twice, expect identical state
  NodeStateMachine fsm1, fsm2;
  SafetyMonitor mon1, mon2;

  fsm1.enter_self_test();
  fsm1.enter_active();
  mon1.update_heartbeat(0);

  fsm2.enter_self_test();
  fsm2.enter_active();
  mon2.update_heartbeat(0);

  for (int run = 0; run < 2; ++run) {
    NodeStateMachine &fsm_ref = (run == 0) ? fsm1 : fsm2;
    SafetyMonitor &mon_ref = (run == 0) ? mon1 : mon2;

    for (int i = 0; i < 100; ++i) {
      uint64_t t_us = i * 10000;
      mon_ref.update_heartbeat(t_us);
      mon_ref.check_heartbeat_timeout(t_us, fsm_ref);
    }
  }

  EXPECT_EQ(fsm1.state(), fsm2.state());
  EXPECT_EQ(fsm1.fault_bitmask(), fsm2.fault_bitmask());
}
