#!/bin/bash
# Verify SIL determinism: identical traces produce bit-identical telemetry

set -e

cd build-host

echo "Running nominal trace twice with same seed..."
./cortexactuate_sil --trace ../sil/traces/nominal_drive.csv --telemetry /tmp/run1.bin --seed 42
./cortexactuate_sil --trace ../sil/traces/nominal_drive.csv --telemetry /tmp/run2.bin --seed 42

echo "Comparing binary files..."
if cmp -s /tmp/run1.bin /tmp/run2.bin; then
    echo "✓ DETERMINISM VERIFIED: Telemetry files are identical"
    exit 0
else
    echo "✗ DETERMINISM FAILED: Telemetry files differ"
    cmp -l /tmp/run1.bin /tmp/run2.bin | head -10
    exit 1
fi
