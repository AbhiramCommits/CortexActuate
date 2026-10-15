/* SIL Harness — Replay CAN traces through control logic */

#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <hal/sim.hpp>
#include <node_state_machine.hpp>
#include <pid_controller.hpp>
#include <safety_monitor.hpp>
#include <telemetry_format.hpp>

using namespace hal;

struct TraceFrame {
  uint64_t timestamp_us;
  uint32_t id;
  uint8_t dlc;
  std::vector<uint8_t> data;
};

std::vector<TraceFrame> load_trace(const std::string &filename) {
  std::vector<TraceFrame> frames;
  std::ifstream f(filename);
  std::string line;

  // Skip header
  std::getline(f, line);

  while (std::getline(f, line)) {
    if (line.empty())
      continue;

    std::stringstream ss(line);
    std::string t_str, id_str, dlc_str, data_str;
    std::getline(ss, t_str, ',');
    std::getline(ss, id_str, ',');
    std::getline(ss, dlc_str, ',');
    std::getline(ss, data_str);

    TraceFrame frame{};
    frame.timestamp_us = std::stoull(t_str);
    frame.id = std::stoul(id_str, nullptr, 16);
    frame.dlc = std::stoul(dlc_str);

    // Parse hex data
    std::string hex = data_str;
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
      uint8_t byte = std::stoul(hex.substr(i, 2), nullptr, 16);
      frame.data.push_back(byte);
    }

    frames.push_back(frame);
  }
  return frames;
}

int main(int argc, char *argv[]) {
  std::string trace_file = "sil/traces/nominal_drive.csv";
  std::string telemetry_out = "telemetry.bin";
  uint32_t seed = 42;
  uint32_t inject_load_us = 0;

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--trace" && i + 1 < argc) {
      trace_file = argv[++i];
    } else if (arg == "--telemetry" && i + 1 < argc) {
      telemetry_out = argv[++i];
    } else if (arg == "--seed" && i + 1 < argc) {
      seed = std::stoul(argv[++i]);
      (void)seed; // TODO: use for deterministic RNG
    } else if (arg == "--inject-load" && i + 1 < argc) {
      inject_load_us = std::stoul(argv[++i]);
    }
  }

  // Load trace
  auto frames = load_trace(trace_file);
  if (frames.empty()) {
    std::cerr << "Failed to load trace: " << trace_file << "\n";
    return 1;
  }

  // Initialize HAL and control logic
  SimClock clock;
  SimCanBus can_bus;
  SimUart uart;
  SimTimer timer;
  SimWatchdog watchdog;

  PidController controller(1.0f, 0.1f, 0.05f, 100.0f, 50.0f, 100.0f);
  NodeStateMachine fsm;
  SafetyMonitor monitor;

  // Start up
  fsm.enter_self_test();
  fsm.enter_active();
  monitor.update_heartbeat(0);

  // Simulation state
  std::vector<TelemetryRecord> telemetry;
  uint32_t loop_count = 0;
  float feedback = 0.0f;
  bool watchdog_kicked = false;
  uint64_t last_loop_time = 0;

  // Replay trace
  for (size_t frame_idx = 0; frame_idx < frames.size(); ++frame_idx) {
    const auto &frame = frames[frame_idx];
    clock.set_time_us(frame.timestamp_us);

    // Inject CAN frame
    if (frame.id == 0x100 && frame.data.size() >= 4) {
      // Extract command fields from frame
      uint8_t setpoint_int = frame.data[2]; // 0-200 scale
      uint8_t seq = frame.data[3];
      uint8_t expected_crc = frame.data[4];

      // Compute CRC over first 4 bytes
      uint8_t computed_crc = 0xFF;
      for (int j = 0; j < 4; ++j) {
        computed_crc ^= frame.data[j];
      }

      // Verify CRC and sequence
      if (computed_crc == expected_crc) {
        // Check sequence
        if (monitor.check_sequence(seq, frame.timestamp_us, fsm)) {
          // Valid command — update heartbeat and run loop
          monitor.update_heartbeat(frame.timestamp_us);

          float setpoint = setpoint_int / 2.0f; // Scale back to 0-100

          // Run control loop at ~1 kHz (every 1000 us, approximately)
          if (last_loop_time == 0 ||
              (frame.timestamp_us - last_loop_time) >= 1000) {
            uint64_t loop_start_jitter =
                (frame.timestamp_us - last_loop_time - 1000) / 10;

            // Simulate feedback reading
            feedback = std::min(feedback + 0.5f, setpoint);

            // Safety checks
            monitor.check_heartbeat_timeout(frame.timestamp_us, fsm);
            monitor.check_sensor_range(feedback, frame.timestamp_us, fsm);
            monitor.check_sensor_rate_of_change(feedback, frame.timestamp_us,
                                                fsm);

            // Run PID
            float output = 0.0f;
            if (!fsm.is_safe_state()) {
              output =
                  controller.update(setpoint, feedback, frame.timestamp_us);
            } else {
              output = 0.0f; // Safe state = zero output
            }

            monitor.check_output_saturation(output, 50.0f, frame.timestamp_us,
                                            fsm);
            watchdog_kicked = true;
            monitor.check_watchdog_kick(watchdog_kicked, frame.timestamp_us,
                                        fsm);

            // Record telemetry
            TelemetryRecord rec{};
            rec.loop_count = loop_count++;
            rec.timestamp_us = frame.timestamp_us;
            rec.loop_start_jitter_us = loop_start_jitter;
            rec.execution_time_us = inject_load_us;
            rec.setpoint_fixed = static_cast<int16_t>(setpoint * 256.0f);
            rec.feedback_fixed = static_cast<int16_t>(feedback * 256.0f);
            rec.output_fixed = static_cast<int16_t>(output * 256.0f);
            rec.state = static_cast<uint8_t>(fsm.state());
            rec.fault_bitmask = fsm.fault_bitmask();
            rec.stack_hwm = 1024; // Placeholder

            telemetry.push_back(rec);
            last_loop_time = frame.timestamp_us;

            if (frame.timestamp_us % 100000 == 0) {
              std::cout << "T=" << (frame.timestamp_us / 1e6) << "s, "
                        << "loops=" << loop_count << ", "
                        << "state=" << static_cast<int>(fsm.state()) << ", "
                        << "faults=" << std::hex << fsm.fault_bitmask()
                        << std::dec << "\n";
            }
          }
        }
      }
    }
  }

  // Write telemetry
  std::ofstream out(telemetry_out, std::ios::binary);
  for (const auto &rec : telemetry) {
    out.write(reinterpret_cast<const char *>(&rec), sizeof(rec));
  }
  out.close();

  // Print summary
  std::cout << "\n=== SIL Results ===\n";
  std::cout << "Trace: " << trace_file << "\n";
  std::cout << "Frames replayed: " << frames.size() << "\n";
  std::cout << "Loop iterations: " << loop_count << "\n";
  std::cout << "Final state: " << static_cast<int>(fsm.state()) << "\n";
  std::cout << "Final faults: 0x" << std::hex << fsm.fault_bitmask() << std::dec
            << "\n";
  std::cout << "Telemetry file: " << telemetry_out << " (" << telemetry.size()
            << " records)\n";

  return fsm.is_safe_state() || fsm.fault_bitmask() > 0 ? 1 : 0;
}
