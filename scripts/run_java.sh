#!/usr/bin/env bash
# Run Java benchmarks (sequential and parallel).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
JAVA_DIR="$ROOT_DIR/java"
IMAGES_DIR="$ROOT_DIR/images"
RESULTS_DIR="$ROOT_DIR/results"

NP="${NP:-4}"
MPJ_HOME="${MPJ_HOME:-/opt/mpj}"

mkdir -p "$RESULTS_DIR"

log() { echo "[Java] $*"; }

# Build sequential
log "Building Java sequential programs..."
(cd "$JAVA_DIR" && make sequential) || { log "Sequential build failed"; exit 1; }

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
    (cd "$JAVA_DIR" && java -cp . sequential.MedianFilter "$NOISE_IMG" "$RESULTS_DIR/java_seq_median_${SIZE}.pgm") 2>&1

    # Sequential connected components
    log "Sequential connected components..."
    (cd "$JAVA_DIR" && java -cp . sequential.ConnectedComponents "$BIN_IMG" "$RESULTS_DIR/java_seq_cc_${SIZE}.pgm") 2>&1
done

# Build and run parallel if MPJ_HOME is set and exists
if [ -d "$MPJ_HOME" ] && [ -f "$MPJ_HOME/lib/mpj.jar" ]; then
    log "Building Java parallel programs..."
    (cd "$JAVA_DIR" && make parallel) || { log "Parallel build failed"; }

    for SIZE in 256 512 1024; do
        NOISE_IMG="$IMAGES_DIR/noise_${SIZE}x${SIZE}.pgm"
        BIN_IMG="$IMAGES_DIR/binary_${SIZE}x${SIZE}.pgm"

        if [ ! -f "$NOISE_IMG" ] || [ ! -f "$BIN_IMG" ]; then continue; fi

        log ""
        log "=== Parallel ${SIZE}x${SIZE} ==="

        log "Parallel median filter (NP=$NP)..."
        "$MPJ_HOME/bin/mpjrun.sh" -np "$NP" -cp "$JAVA_DIR" parallel.MedianFilterMPI \
            "$NOISE_IMG" "$RESULTS_DIR/java_par_median_${SIZE}.pgm" 2>&1 || log "MPJ median filter failed"

        log "Parallel connected components (NP=$NP)..."
        "$MPJ_HOME/bin/mpjrun.sh" -np "$NP" -cp "$JAVA_DIR" parallel.ConnectedComponentsMPI \
            "$BIN_IMG" "$RESULTS_DIR/java_par_cc_${SIZE}.pgm" 2>&1 || log "MPJ CC failed"
    done
else
    log "MPJ_HOME not found or mpj.jar missing. Skipping parallel Java benchmarks."
    log "Set MPJ_HOME to your MPJ Express installation directory."
fi

log "Java benchmarks done."
