/* PID Controller with Anti-Windup and Slew Rate Limiting */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

class PidController {
private:
  float kp_, ki_, kd_;
  float integral_ = 0.0f;
  float last_error_ = 0.0f;
  float integral_limit_;
  float output_limit_;
  float slew_rate_limit_; // units/second
  float last_output_ = 0.0f;
  uint64_t last_time_us_ = 0;

public:
  PidController(float kp, float ki, float kd, float integral_limit,
                float output_limit, float slew_rate_limit)
      : kp_(kp), ki_(ki), kd_(kd), integral_limit_(integral_limit),
        output_limit_(output_limit), slew_rate_limit_(slew_rate_limit) {}

  float update(float setpoint, float feedback, uint64_t now_us) {
    float error = setpoint - feedback;

    // Time delta
    if (last_time_us_ == 0) {
      last_time_us_ = now_us;
      last_error_ = error;
      return 0.0f;
    }

    uint64_t dt_us = now_us - last_time_us_;
    float dt = dt_us / 1e6f;

    if (dt < 0.0001f)
      return last_output_; // Debounce

    // P term
    float p_term = kp_ * error;

    // I term with anti-windup
    integral_ += error * dt;
    integral_ = std::clamp(integral_, -integral_limit_, integral_limit_);
    float i_term = ki_ * integral_;

    // D term
    float error_rate = (error - last_error_) / dt;
    float d_term = kd_ * error_rate;

    // Compute output
    float output = p_term + i_term + d_term;

    // Clamp to output limits
    output = std::clamp(output, -output_limit_, output_limit_);

    // Apply slew rate limiting
    float max_delta = slew_rate_limit_ * dt;
    float delta = output - last_output_;
    if (std::abs(delta) > max_delta) {
      output = last_output_ + std::copysign(max_delta, delta);
    }

    last_output_ = output;
    last_error_ = error;
    last_time_us_ = now_us;

    return output;
  }

  float last_output() const { return last_output_; }
  void reset() {
    integral_ = 0.0f;
    last_error_ = 0.0f;
    last_output_ = 0.0f;
    last_time_us_ = 0;
  }
};
