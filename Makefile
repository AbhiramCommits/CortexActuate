.PHONY: all host arm test sil report check clean

all: host test report

host:
	mkdir -p build-host
	cd build-host && cmake -DCORTEXACTUATE_TARGET=HOST .. && cmake --build .

arm:
	mkdir -p build-arm
	cd build-arm && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-toolchain.cmake -DCORTEXACTUATE_TARGET=STM32F407 .. && cmake --build .

test: host
	cd build-host && ctest --output-on-failure

sil: host
	cd build-host && \
	./cortexactuate_sil --trace ../sil/traces/nominal_drive.csv --telemetry /tmp/nominal.bin; \
	./cortexactuate_sil --trace ../sil/traces/heartbeat_dropout.csv --telemetry /tmp/dropout.bin; \
	./cortexactuate_sil --trace ../sil/traces/corrupt_crc.csv --telemetry /tmp/corrupt.bin; \
	true

report: sil
	mkdir -p reports
	python3 tools/telemetry_report.py /tmp/nominal.bin > reports/telemetry_report.txt 2>&1

check: test sil
	python3 tools/check_budgets.py
	clang-format --dry-run -Werror $$(find src include test sil -name "*.cpp" -o -name "*.hpp") || true

clean:
	rm -rf build-host build-arm reports /tmp/nominal.bin /tmp/dropout.bin /tmp/corrupt.bin

format:
	clang-format -i $$(find src include test sil -name "*.cpp" -o -name "*.hpp")

help:
	@echo "Targets:"
	@echo "  make host       - Build SIL harness and tests"
	@echo "  make arm        - Build ARM firmware (needs arm-none-eabi-gcc)"
	@echo "  make test       - Run unit tests"
	@echo "  make sil        - Run all traces through SIL"
	@echo "  make report     - Generate telemetry reports"
	@echo "  make check      - Run all checks (tests, SIL, budgets, format)"
	@echo "  make format     - Auto-format all sources"
	@echo "  make clean      - Remove all build artifacts"
