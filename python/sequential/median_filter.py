#!/usr/bin/env python3
"""
Sequential Median Filter for PGM images.
Uses only Python standard library.
"""
import sys
import time
from typing import List, Tuple


def read_pgm(filename: str) -> Tuple[int, int, int, List[List[int]]]:
    """Read a P2 PGM file. Returns (width, height, maxval, pixels)."""
    with open(filename, 'r') as f:
        lines = []
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                lines.append(line)

    if lines[0] != 'P2':
        raise ValueError(f"Not a P2 PGM file (got {lines[0]})")

    # Parse dimensions
    parts = lines[1].split()
    width, height = int(parts[0]), int(parts[1])
    maxval = int(lines[2])

    # Parse pixel data
    all_values = []
    for line in lines[3:]:
        all_values.extend(map(int, line.split()))

    pixels = []
    idx = 0
    for i in range(height):
        row = all_values[idx:idx + width]
        pixels.append(row)
        idx += width

    return width, height, maxval, pixels


def write_pgm(filename: str, width: int, height: int, maxval: int, pixels: List[List[int]]):
    """Write a P2 PGM file."""
    with open(filename, 'w') as f:
        f.write(f"P2\n{width} {height}\n{maxval}\n")
        for row in pixels:
            f.write(' '.join(map(str, row)) + '\n')


def median_filter(pixels: List[List[int]], height: int, width: int, kernel_size: int = 3) -> List[List[int]]:
    """Apply median filter with given kernel size (must be odd)."""
    half = kernel_size // 2
    output = [[0] * width for _ in range(height)]

    for i in range(height):
        for j in range(width):
            neighbors = []
            for ki in range(-half, half + 1):
                for kj in range(-half, half + 1):
                    ni = max(0, min(height - 1, i + ki))
                    nj = max(0, min(width - 1, j + kj))
                    neighbors.append(pixels[ni][nj])
            neighbors.sort()
            output[i][j] = neighbors[len(neighbors) // 2]

    return output


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.pgm> <output.pgm>", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    width, height, maxval, pixels = read_pgm(input_file)
    print(f"Image loaded: {width}x{height}")

    start = time.perf_counter()
    result = median_filter(pixels, height, width, kernel_size=3)
    end = time.perf_counter()

    elapsed_ms = (end - start) * 1000.0
    print(f"Median filter (sequential) time: {elapsed_ms:.2f} ms")

    write_pgm(output_file, width, height, maxval, result)
    print(f"Output written to {output_file}")


if __name__ == '__main__':
    main()
