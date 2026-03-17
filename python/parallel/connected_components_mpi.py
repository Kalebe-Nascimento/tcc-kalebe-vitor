#!/usr/bin/env python3
"""
Parallel Connected Components using mpi4py.
Strategy: Each rank does local BFS with unique label offsets,
then exchanges boundary rows to merge cross-partition components.
"""
import sys
import time
from collections import deque

from mpi4py import MPI


def read_pgm(filename):
    with open(filename, 'r') as f:
        lines = [l.strip() for l in f if l.strip() and not l.strip().startswith('#')]
    if lines[0] != 'P2':
        raise ValueError("Not a P2 PGM file")
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


def uf_find(parent, x):
    while parent[x] != x:
        parent[x] = parent[parent[x]]
        x = parent[x]
    return x


def uf_unite(parent, a, b):
    ra, rb = uf_find(parent, a), uf_find(parent, b)
    if ra != rb:
        parent[ra] = rb


def local_bfs(pixels, width, start_row, num_rows, label_offset):
    """BFS on rows [start_row, start_row+num_rows). Returns label array (flat)."""
    labels = [-1] * (num_rows * width)
    directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    num_comp = 0

    for i in range(num_rows):
        for j in range(width):
            if pixels[(start_row + i) * width + j] > 127 and labels[i * width + j] == -1:
                lbl = label_offset + num_comp
                queue = deque()
                queue.append((i, j))
                labels[i * width + j] = lbl
                while queue:
                    r, c = queue.popleft()
                    for dr, dc in directions:
                        nr, nc = r + dr, c + dc
                        if (0 <= nr < num_rows and 0 <= nc < width
                                and pixels[(start_row + nr) * width + nc] > 127
                                and labels[nr * width + nc] == -1):
                            labels[nr * width + nc] = lbl
                            queue.append((nr, nc))
                num_comp += 1

    return labels, num_comp


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

    # Compute label offset
    all_row_counts = comm.allgather(my_rows)
    label_offset = sum(all_row_counts[:rank]) * width

    max_labels = width * height
    parent = list(range(max_labels))

    t_start = time.perf_counter()

    local_labels, _ = local_bfs(all_pixels, width, start_row, my_rows, label_offset)

    # Exchange boundaries with neighbors
    if rank < size - 1:
        next_rank = rank + 1
        my_bottom_px = all_pixels[(start_row + my_rows - 1) * width:(start_row + my_rows) * width]
        my_bottom_lbl = local_labels[(my_rows - 1) * width:my_rows * width]

        their_top_px = comm.sendrecv(my_bottom_px, dest=next_rank, sendtag=0, source=next_rank, recvtag=1)
        their_top_lbl = comm.sendrecv(my_bottom_lbl, dest=next_rank, sendtag=2, source=next_rank, recvtag=3)

        for j in range(width):
            if (my_bottom_px[j] > 127 and their_top_px[j] > 127
                    and my_bottom_lbl[j] != -1 and their_top_lbl[j] != -1):
                uf_unite(parent, my_bottom_lbl[j], their_top_lbl[j])

    if rank > 0:
        prev_rank = rank - 1
        my_top_px = all_pixels[start_row * width:(start_row + 1) * width]
        my_top_lbl = local_labels[:width]

        their_bottom_px = comm.sendrecv(my_top_px, dest=prev_rank, sendtag=1, source=prev_rank, recvtag=0)
        their_bottom_lbl = comm.sendrecv(my_top_lbl, dest=prev_rank, sendtag=3, source=prev_rank, recvtag=2)

        for j in range(width):
            if (my_top_px[j] > 127 and their_bottom_px[j] > 127
                    and my_top_lbl[j] != -1 and their_bottom_lbl[j] != -1):
                uf_unite(parent, my_top_lbl[j], their_bottom_lbl[j])

    # Apply union-find to local labels
    for idx in range(len(local_labels)):
        if local_labels[idx] != -1:
            local_labels[idx] = uf_find(parent, local_labels[idx])

    t_end = time.perf_counter()

    # Gather all labels to rank 0
    all_labels_parts = comm.gather(local_labels, root=0)

    if rank == 0:
        all_labels = []
        for part in all_labels_parts:
            all_labels.extend(part)

        label_map = {}
        comp_count = 0
        out_pixels = [0] * (width * height)
        for i in range(width * height):
            if all_labels[i] != -1:
                root = all_labels[i]
                if root not in label_map:
                    comp_count += 1
                    label_map[root] = comp_count
                out_pixels[i] = label_map[root]

        for i in range(width * height):
            if all_labels[i] == -1:
                out_pixels[i] = 0
            elif comp_count <= 1:
                out_pixels[i] = 255
            else:
                out_pixels[i] = (out_pixels[i] * 255) // comp_count

        elapsed_ms = (t_end - t_start) * 1000.0
        print(f"Connected components found: {comp_count}")
        print(f"Connected components (parallel, {size} procs) time: {elapsed_ms:.2f} ms")

        write_pgm(output_file, out_pixels, width, height, 255)
        print(f"Output written to {output_file}")


if __name__ == '__main__':
    main()
