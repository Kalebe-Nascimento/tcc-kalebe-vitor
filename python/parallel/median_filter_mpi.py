#!/usr/bin/env python3
"""
Parallel Median Filter using mpi4py.
"""
import sys
import time

from mpi4py import MPI


def read_pgm(filename):
    """Read P2 PGM. Returns (width, height, maxval, flat_pixels_list)."""
    with open(filename, 'r') as f:
        lines = [l.strip() for l in f if l.strip() and not l.strip().startswith('#')]
    if lines[0] != 'P2':
        raise ValueError(f"Not a P2 PGM file")
    w, h = map(int, lines[1].split())
    maxval = int(lines[2])
    vals = []
    for line in lines[3:]:
        vals.extend(map(int, line.split()))
    return w, h, maxval, vals


def write_pgm(filename, pixels, width, height, maxval):
    with open(filename, 'w') as f:
        f.write(f"P2\n{width} {height}\n{maxval}\n")
        for i in range(height):
            f.write(' '.join(map(str, pixels[i * width:(i + 1) * width])) + '\n')


def filter_rows(data, width, local_rows, halo_size=1):
    """Apply median filter to local_rows rows. data includes halo rows."""
    total_rows = local_rows + 2 * halo_size
    result = [0] * (local_rows * width)
    for i in range(local_rows):
        actual_i = i + halo_size
        for j in range(width):
            neighbors = []
            for ki in range(-halo_size, halo_size + 1):
                for kj in range(-halo_size, halo_size + 1):
                    ni = max(0, min(total_rows - 1, actual_i + ki))
                    nj = max(0, min(width - 1, j + kj))
                    neighbors.append(data[ni * width + nj])
            neighbors.sort()
            result[i * width + j] = neighbors[len(neighbors) // 2]
    return result


def main():
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    if len(sys.argv) < 3:
        if rank == 0:
            print(f"Usage: {sys.argv[0]} <input.pgm> <output.pgm>", file=sys.stderr)
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    dims = None
    all_pixels = None

    if rank == 0:
        width, height, maxval, all_pixels = read_pgm(input_file)
        dims = (width, height, maxval)
        print(f"Image loaded: {width}x{height}")

    dims = comm.bcast(dims, root=0)
    all_pixels = comm.bcast(all_pixels, root=0)

    width, height, maxval = dims

    base_rows = height // size
    extra = height % size
    my_rows = base_rows + (1 if rank < extra else 0)
    start_row = rank * base_rows + min(rank, extra)

    half = 1
    local_data = []
    for i in range(-half, my_rows + half):
        src_row = max(0, min(height - 1, start_row + i))
        local_data.extend(all_pixels[src_row * width:(src_row + 1) * width])

    t_start = time.perf_counter()
    local_result = filter_rows(local_data, width, my_rows, halo_size=half)
    t_end = time.perf_counter()

    # Gather results
    all_results = comm.gather(local_result, root=0)

    if rank == 0:
        elapsed_ms = (t_end - t_start) * 1000.0
        print(f"Median filter (parallel, {size} procs) time: {elapsed_ms:.2f} ms")

        flat_result = []
        for part in all_results:
            flat_result.extend(part)

        write_pgm(output_file, flat_result, width, height, maxval)
        print(f"Output written to {output_file}")


if __name__ == '__main__':
    main()
