#!/usr/bin/env bash
# Run C++ benchmarks (sequential and parallel).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
CPP_DIR="$ROOT_DIR/cpp"
IMAGES_DIR="$ROOT_DIR/images"
RESULTS_DIR="$ROOT_DIR/results"

NP="${NP:-4}"

mkdir -p "$RESULTS_DIR"

log() { echo "[C++] $*"; }

# Build
log "Building C++ programs..."
(cd "$CPP_DIR" && make all) || { log "Build failed"; exit 1; }
log "Build successful"

for SIZE in 256 512 1024; do
    NOISE_IMG="$IMAGES_DIR/noise_${SIZE}x${SIZE}.pgm"
    BIN_IMG="$IMAGES_DIR/binary_${SIZE}x${SIZE}.pgm"

    if [ ! -f "$NOISE_IMG" ] || [ ! -f "$BIN_IMG" ]; then
        log "Skipping ${SIZE}x${SIZE}: images not found"
        continue
    fi

    log ""
    log "=== ${SIZE}x${SIZE} ==="

    # Sequential median filter
    log "Sequential median filter..."
    "$CPP_DIR/sequential/median_filter" "$NOISE_IMG" "$RESULTS_DIR/cpp_seq_median_${SIZE}.pgm" 2>&1

    # Sequential connected components
    log "Sequential connected components..."
    "$CPP_DIR/sequential/connected_components" "$BIN_IMG" "$RESULTS_DIR/cpp_seq_cc_${SIZE}.pgm" 2>&1

    # Parallel median filter
    log "Parallel median filter (NP=$NP)..."
    mpirun -np "$NP" "$CPP_DIR/parallel/median_filter_mpi" "$NOISE_IMG" "$RESULTS_DIR/cpp_par_median_${SIZE}.pgm" 2>&1 || log "Parallel median filter failed"

    # Parallel connected components
    log "Parallel connected components (NP=$NP)..."
    mpirun -np "$NP" "$CPP_DIR/parallel/connected_components_mpi" "$BIN_IMG" "$RESULTS_DIR/cpp_par_cc_${SIZE}.pgm" 2>&1 || log "Parallel CC failed"
done

log "C++ benchmarks done."
