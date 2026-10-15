/* Telemetry Format Tests */
#include <gtest/gtest.h>
#include <telemetry_format.hpp>
#include <cstring>

class TelemetryTest : public ::testing::Test {
protected:
    TelemetryRecord rec{};
};

TEST_F(TelemetryTest, SizeCheck) {
    EXPECT_EQ(sizeof(TelemetryRecord), 33);
}

TEST_F(TelemetryTest, OffsetCheck) {
    EXPECT_EQ(offsetof(TelemetryRecord, loop_count), 0);
    EXPECT_EQ(offsetof(TelemetryRecord, timestamp_us), 4);
    EXPECT_EQ(offsetof(TelemetryRecord, loop_start_jitter_us), 12);
    EXPECT_EQ(offsetof(TelemetryRecord, execution_time_us), 16);
    EXPECT_EQ(offsetof(TelemetryRecord, setpoint_fixed), 20);
    EXPECT_EQ(offsetof(TelemetryRecord, feedback_fixed), 22);
    EXPECT_EQ(offsetof(TelemetryRecord, output_fixed), 24);
    EXPECT_EQ(offsetof(TelemetryRecord, state), 26);
    EXPECT_EQ(offsetof(TelemetryRecord, fault_bitmask), 27);
    EXPECT_EQ(offsetof(TelemetryRecord, stack_hwm), 31);
}

TEST_F(TelemetryTest, PackUnpack) {
    rec.loop_count = 42;
    rec.timestamp_us = 1234567890ULL;
    rec.loop_start_jitter_us = 50;
    rec.execution_time_us = 75;
    rec.setpoint_fixed = 1280;  // 5.0 * 256
    rec.feedback_fixed = 768;   // 3.0 * 256
    rec.output_fixed = 512;     // 2.0 * 256
    rec.state = 2;
    rec.fault_bitmask = 0;
    rec.stack_hwm = 512;

    // Serialize and deserialize
    uint8_t buffer[33];
    std::memcpy(buffer, &rec, 33);

    TelemetryRecord rec2;
    std::memcpy(&rec2, buffer, 33);

    EXPECT_EQ(rec2.loop_count, 42);
    EXPECT_EQ(rec2.timestamp_us, 1234567890ULL);
    EXPECT_EQ(rec2.setpoint_fixed, 1280);
    EXPECT_EQ(rec2.state, 2);
}

TEST_F(TelemetryTest, FixedPointConversion) {
    // Test scale: value * 256
    float val = 25.5f;
    int16_t fixed = static_cast<int16_t>(val * 256.0f);
    float back = fixed / 256.0f;
    EXPECT_NEAR(back, val, 0.01f);
}
