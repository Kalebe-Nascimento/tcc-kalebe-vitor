#!/usr/bin/env bash
# Run Python benchmarks (sequential and parallel).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
PYTHON_DIR="$ROOT_DIR/python"
IMAGES_DIR="$ROOT_DIR/images"
RESULTS_DIR="$ROOT_DIR/results"

NP="${NP:-4}"

mkdir -p "$RESULTS_DIR"

log() { echo "[Python] $*"; }

for SIZE in 256 512 1024; do
    NOISE_IMG="$IMAGES_DIR/noise_${SIZE}x${SIZE}.pgm"
    BIN_IMG="$IMAGES_DIR/binary_${SIZE}x${SIZE}.pgm"

    if [ ! -f "$NOISE_IMG" ] || [ ! -f "$BIN_IMG" ]; then
        log "Skipping ${SIZE}x${SIZE}: images not found"
        continue
    fi

    log ""
    log "=== ${SIZE}x${SIZE} ==="

    log "Sequential median filter..."
    python3 "$PYTHON_DIR/sequential/median_filter.py" \
        "$NOISE_IMG" "$RESULTS_DIR/py_seq_median_${SIZE}.pgm" 2>&1

    log "Sequential connected components..."
    python3 "$PYTHON_DIR/sequential/connected_components.py" \
        "$BIN_IMG" "$RESULTS_DIR/py_seq_cc_${SIZE}.pgm" 2>&1

    # Check if mpi4py is available
    if python3 -c "from mpi4py import MPI" 2>/dev/null; then
        log "Parallel median filter (NP=$NP)..."
        mpirun -np "$NP" python3 "$PYTHON_DIR/parallel/median_filter_mpi.py" \
            "$NOISE_IMG" "$RESULTS_DIR/py_par_median_${SIZE}.pgm" 2>&1 || log "Parallel median filter failed"

        log "Parallel connected components (NP=$NP)..."
        mpirun -np "$NP" python3 "$PYTHON_DIR/parallel/connected_components_mpi.py" \
            "$BIN_IMG" "$RESULTS_DIR/py_par_cc_${SIZE}.pgm" 2>&1 || log "Parallel CC failed"
    else
        log "mpi4py not available. Skipping parallel Python benchmarks."
        log "Install with: pip install mpi4py"
    fi
done

log "Python benchmarks done."
