/* HAL Contract Tests */
#include <gtest/gtest.h>
#include <hal/sim.hpp>

using namespace hal;

class HalContractTest : public ::testing::Test {
protected:
    SimClock clock_;
    SimUart uart_;
    SimSpi spi_;
    SimCanBus can_;
    SimTimer timer_;
    SimWatchdog watchdog_;
    SimGpio gpio_;
};

TEST_F(HalContractTest, ClockTime) {
    EXPECT_EQ(clock_.micros(), 0);
    clock_.advance_us(100);
    EXPECT_EQ(clock_.micros(), 100);
    clock_.set_time_us(1000);
    EXPECT_EQ(clock_.micros(), 1000);
}

TEST_F(HalContractTest, UartWrite) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    size_t written = uart_.write(data, 3);
    EXPECT_EQ(written, 3);
    ASSERT_EQ(uart_.tx_data().size(), 3);
    EXPECT_EQ(uart_.tx_data()[0], 0xAA);
    EXPECT_EQ(uart_.tx_data()[1], 0xBB);
    EXPECT_EQ(uart_.tx_data()[2], 0xCC);
}

TEST_F(HalContractTest, UartRead) {
    uint8_t inject[] = {0x11, 0x22, 0x33};
    uart_.inject_rx(inject, 3);

    uint8_t buffer[10] = {0};
    size_t read = uart_.read(buffer, 10);
    EXPECT_EQ(read, 3);
    EXPECT_EQ(buffer[0], 0x11);
    EXPECT_EQ(buffer[1], 0x22);
    EXPECT_EQ(buffer[2], 0x33);
}

TEST_F(HalContractTest, SpiTransfer) {
    uint8_t tx[] = {0x55, 0x66};
    uint8_t rx[2] = {0};
    bool ok = spi_.transfer(tx, rx, 2);
    EXPECT_TRUE(ok);
    EXPECT_EQ(spi_.tx_log().size(), 2);
}

TEST_F(HalContractTest, CanFrameSend) {
    CanFrame frame{};
    frame.id = 0x123;
    frame.dlc = 8;
    frame.data = 0x0102030405060708ULL;

    bool ok = can_.send(frame);
    EXPECT_TRUE(ok);
    ASSERT_EQ(can_.tx_log().size(), 1);
    EXPECT_EQ(can_.tx_log()[0].id, 0x123);
    EXPECT_EQ(can_.tx_log()[0].dlc, 8);
}

TEST_F(HalContractTest, CanFrameReceive) {
    CanFrame inject_frame{};
    inject_frame.id = 0x456;
    inject_frame.dlc = 4;
    inject_frame.data = 0xDEADBEEF;

    can_.inject_frame(inject_frame);

    CanFrame rx_frame{};
    bool ok = can_.receive(rx_frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(rx_frame.id, 0x456);
    EXPECT_EQ(rx_frame.dlc, 4);
}

TEST_F(HalContractTest, TimerStart) {
    bool callback_fired = false;
    timer_.start_periodic(1000, [&]() { callback_fired = true; });

    EXPECT_TRUE(timer_.is_enabled());
    EXPECT_EQ(timer_.period(), 1000);
    EXPECT_NE(timer_.callback(), nullptr);
}

TEST_F(HalContractTest, WatchdogEnable) {
    watchdog_.enable(500);
    EXPECT_TRUE(watchdog_.is_enabled());
    EXPECT_EQ(watchdog_.timeout_ms(), 500);
}

TEST_F(HalContractTest, WatchdogKick) {
    watchdog_.enable(500);
    watchdog_.kick();
    watchdog_.kick();
    EXPECT_EQ(watchdog_.kick_count(), 2);
}

TEST_F(HalContractTest, GpioSetGet) {
    gpio_.set(true);
    EXPECT_TRUE(gpio_.get());
    gpio_.set(false);
    EXPECT_FALSE(gpio_.get());
}
