/* HAL Interfaces — allocation-free, portable */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace hal {

// CAN frame structure
struct CanFrame {
    uint32_t id;
    uint8_t dlc;
    uint64_t data;  // 0-8 bytes packed
    bool fd;
    uint64_t timestamp_us;
};

// Clock interface
class IClock {
public:
    virtual ~IClock() = default;
    virtual uint64_t micros() const = 0;
};

// UART interface
class IUart {
public:
    virtual ~IUart() = default;
    virtual size_t write(const uint8_t* data, size_t len) = 0;
    virtual size_t read(uint8_t* data, size_t len) = 0;
};

// SPI interface
class ISpi {
public:
    virtual ~ISpi() = default;
    virtual bool transfer(const uint8_t* tx, uint8_t* rx, size_t n) = 0;
};

// CAN interface
class ICanBus {
public:
    virtual ~ICanBus() = default;
    virtual bool send(const CanFrame& frame) = 0;
    virtual bool receive(CanFrame& frame) = 0;
};

// Timer callback type
using TimerCallback = std::function<void()>;

// Timer interface
class ITimer {
public:
    virtual ~ITimer() = default;
    virtual void start_periodic(uint32_t period_us, TimerCallback callback) = 0;
    virtual void stop() = 0;
};

// Watchdog interface
class IWatchdog {
public:
    virtual ~IWatchdog() = default;
    virtual void enable(uint32_t timeout_ms) = 0;
    virtual void kick() = 0;
    virtual bool is_enabled() const = 0;
};

// GPIO interface
class IGpio {
public:
    virtual ~IGpio() = default;
    virtual void set(bool value) = 0;
    virtual bool get() const = 0;
};

}  // namespace hal
