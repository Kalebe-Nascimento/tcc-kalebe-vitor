#!/usr/bin/env python3
"""
Sequential Connected Components labeling for binary PGM images.
Uses only Python standard library.
"""
import sys
import time
from collections import deque
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

    parts = lines[1].split()
    width, height = int(parts[0]), int(parts[1])
    maxval = int(lines[2])

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


def connected_components(pixels: List[List[int]], height: int, width: int) -> Tuple[List[List[int]], int]:
    """
    BFS-based connected components labeling.
    Pixels > 127 are foreground.
    Returns (labeled_image normalized 0-255, num_components).
    """
    labels = [[-1] * width for _ in range(height)]
    num_comp = 0

    directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]

    for i in range(height):
        for j in range(width):
            if pixels[i][j] > 127 and labels[i][j] == -1:
                queue = deque()
                queue.append((i, j))
                labels[i][j] = num_comp
                while queue:
                    r, c = queue.popleft()
                    for dr, dc in directions:
                        nr, nc = r + dr, c + dc
                        if (0 <= nr < height and 0 <= nc < width
                                and pixels[nr][nc] > 127 and labels[nr][nc] == -1):
                            labels[nr][nc] = num_comp
                            queue.append((nr, nc))
                num_comp += 1

    # Normalize labels to 0-255
    output = [[0] * width for _ in range(height)]
    for i in range(height):
        for j in range(width):
            if labels[i][j] == -1:
                output[i][j] = 0
            elif num_comp <= 1:
                output[i][j] = 255
            else:
                output[i][j] = (labels[i][j] * 255) // (num_comp - 1)

    return output, num_comp


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.pgm> <output.pgm>", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    width, height, maxval, pixels = read_pgm(input_file)
    print(f"Image loaded: {width}x{height}")

    start = time.perf_counter()
    result, num_comp = connected_components(pixels, height, width)
    end = time.perf_counter()

    elapsed_ms = (end - start) * 1000.0
    print(f"Connected components found: {num_comp}")
    print(f"Connected components (sequential) time: {elapsed_ms:.2f} ms")

    write_pgm(output_file, width, height, 255, result)
    print(f"Output written to {output_file}")


if __name__ == '__main__':
    main()
