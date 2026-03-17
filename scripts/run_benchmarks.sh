#!/usr/bin/env bash
# Run all benchmarks (C++, Java, Python) and save results.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
IMAGES_DIR="$ROOT_DIR/images"
RESULTS_DIR="$ROOT_DIR/results"
CPP_DIR="$ROOT_DIR/cpp"
JAVA_DIR="$ROOT_DIR/java"
PYTHON_DIR="$ROOT_DIR/python"
SCRIPTS_DIR="$SCRIPT_DIR"

NP="${NP:-4}"  # Number of MPI processes
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_FILE="$RESULTS_DIR/benchmark_${TIMESTAMP}.txt"

mkdir -p "$RESULTS_DIR"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$RESULT_FILE"; }

log "=== TCC Benchmark Run - $TIMESTAMP ==="
log "MPI processes: $NP"
log ""

# Generate test images if needed
if [ ! -f "$IMAGES_DIR/noise_512x512.pgm" ]; then
    log "Generating test images..."
    python3 "$SCRIPTS_DIR/generate_test_image.py" || true
fi

# Run C++ benchmarks
log "--- C++ Benchmarks ---"
bash "$SCRIPTS_DIR/run_cpp.sh" 2>&1 | tee -a "$RESULT_FILE" || log "C++ benchmarks failed or skipped"

log ""

# Run Java benchmarks
log "--- Java Benchmarks ---"
bash "$SCRIPTS_DIR/run_java.sh" 2>&1 | tee -a "$RESULT_FILE" || log "Java benchmarks failed or skipped"

log ""

# Run Python benchmarks
log "--- Python Benchmarks ---"
bash "$SCRIPTS_DIR/run_python.sh" 2>&1 | tee -a "$RESULT_FILE" || log "Python benchmarks failed or skipped"

log ""
log "=== Benchmark complete. Results saved to $RESULT_FILE ==="
