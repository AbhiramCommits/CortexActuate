/* Simulated HAL implementations */
#pragma once

#include "interfaces.hpp"
#include <cstring>
#include <queue>
#include <vector>

namespace hal {

class SimClock : public IClock {
private:
  mutable uint64_t sim_time_us_ = 0;

public:
  uint64_t micros() const override { return sim_time_us_; }
  void advance_us(uint64_t us) { sim_time_us_ += us; }
  void set_time_us(uint64_t us) { sim_time_us_ = us; }
};

class SimUart : public IUart {
private:
  std::vector<uint8_t> tx_buffer_;
  std::queue<uint8_t> rx_queue_;

public:
  size_t write(const uint8_t *data, size_t len) override {
    if (!data)
      return 0;
    tx_buffer_.insert(tx_buffer_.end(), data, data + len);
    return len;
  }

  size_t read(uint8_t *data, size_t len) override {
    if (!data || len == 0)
      return 0;
    size_t count = 0;
    while (count < len && !rx_queue_.empty()) {
      data[count++] = rx_queue_.front();
      rx_queue_.pop();
    }
    return count;
  }

  const std::vector<uint8_t> &tx_data() const { return tx_buffer_; }
  void inject_rx(const uint8_t *data, size_t len) {
    if (!data)
      return;
    for (size_t i = 0; i < len; ++i) {
      rx_queue_.push(data[i]);
    }
  }
};

class SimSpi : public ISpi {
private:
  std::vector<uint8_t> tx_log_;

public:
  bool transfer(const uint8_t *tx, uint8_t *rx, size_t n) override {
    if (tx) {
      tx_log_.insert(tx_log_.end(), tx, tx + n);
    }
    if (rx) {
      std::memset(rx, 0, n); // Return zeros for reads
    }
    return true;
  }

  const std::vector<uint8_t> &tx_log() const { return tx_log_; }
};

class SimCanBus : public ICanBus {
private:
  std::queue<CanFrame> rx_queue_;
  std::vector<CanFrame> tx_log_;

public:
  bool send(const CanFrame &frame) override {
    tx_log_.push_back(frame);
    return true;
  }

  bool receive(CanFrame &frame) override {
    if (rx_queue_.empty())
      return false;
    frame = rx_queue_.front();
    rx_queue_.pop();
    return true;
  }

  void inject_frame(const CanFrame &frame) { rx_queue_.push(frame); }

  const std::vector<CanFrame> &tx_log() const { return tx_log_; }
};

class SimTimer : public ITimer {
private:
  uint32_t period_us_ = 0;
  TimerCallback callback_;
  bool enabled_ = false;

public:
  void start_periodic(uint32_t period_us, TimerCallback callback) override {
    period_us_ = period_us;
    callback_ = callback;
    enabled_ = true;
  }

  void stop() override { enabled_ = false; }

  uint32_t period() const { return period_us_; }
  TimerCallback callback() const { return callback_; }
  bool is_enabled() const { return enabled_; }
};

class SimWatchdog : public IWatchdog {
private:
  uint32_t timeout_ms_ = 0;
  bool enabled_ = false;
  uint32_t kick_count_ = 0;

public:
  void enable(uint32_t timeout_ms) override {
    timeout_ms_ = timeout_ms;
    enabled_ = true;
    kick_count_ = 0;
  }

  void kick() override {
    if (enabled_)
      kick_count_++;
  }

  bool is_enabled() const override { return enabled_; }
  uint32_t timeout_ms() const { return timeout_ms_; }
  uint32_t kick_count() const { return kick_count_; }
};

class SimGpio : public IGpio {
private:
  bool state_ = false;

public:
  void set(bool value) override { state_ = value; }
  bool get() const override { return state_; }
};

} // namespace hal
