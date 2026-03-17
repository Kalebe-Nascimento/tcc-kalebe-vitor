#!/usr/bin/env python3
"""
Generate test PGM images for benchmarking.

Usage:
    python generate_test_image.py [size] [type]

Arguments:
    size: Image size (256, 512, 1024) - default: all sizes
    type: Image type (noise, binary, all) - default: all

Output:
    Images are saved to ../images/
"""
import os
import sys
import random
import math

IMAGES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'images')


def write_pgm(filename, pixels, width, height, maxval=255):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, 'w') as f:
        f.write(f"P2\n{width} {height}\n{maxval}\n")
        for i in range(height):
            f.write(' '.join(map(str, pixels[i * width:(i + 1) * width])) + '\n')
    print(f"Generated: {filename}")


def generate_noise_image(width, height, seed=42):
    """Generate grayscale noise image for median filter testing."""
    rng = random.Random(seed)
    pixels = [rng.randint(0, 255) for _ in range(width * height)]
    # Add salt and pepper noise
    num_noise = int(0.05 * width * height)
    for _ in range(num_noise):
        idx = rng.randint(0, width * height - 1)
        pixels[idx] = 255 if rng.random() > 0.5 else 0
    return pixels


def generate_binary_image(width, height, seed=42):
    """Generate binary image with random circles/rectangles for connected components."""
    rng = random.Random(seed)
    pixels = [0] * (width * height)

    # Use ~1 shape per 2000 pixels to keep components sparse and well-separated
    num_shapes = max(10, (width * height) // 2000)

    for _ in range(num_shapes):
        shape_type = rng.choice(['circle', 'rect'])
        cx = rng.randint(0, width - 1)
        cy = rng.randint(0, height - 1)
        r = rng.randint(max(3, min(width, height) // 20), max(5, min(width, height) // 8))

        if shape_type == 'circle':
            for y in range(max(0, cy - r), min(height, cy + r + 1)):
                for x in range(max(0, cx - r), min(width, cx + r + 1)):
                    if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                        pixels[y * width + x] = 255
        else:
            x0 = max(0, cx - r)
            x1 = min(width, cx + r)
            y0 = max(0, cy - r)
            y1 = min(height, cy + r)
            for y in range(y0, y1):
                for x in range(x0, x1):
                    pixels[y * width + x] = 255

    return pixels


def main():
    args = sys.argv[1:]

    sizes_arg = None
    type_arg = 'all'

    if len(args) >= 1:
        try:
            sizes_arg = [int(args[0])]
        except ValueError:
            type_arg = args[0]
    if len(args) >= 2:
        type_arg = args[1]

    if sizes_arg is None:
        sizes = [256, 512, 1024]
    else:
        sizes = sizes_arg

    os.makedirs(IMAGES_DIR, exist_ok=True)

    for size in sizes:
        width = height = size

        if type_arg in ('noise', 'all'):
            pixels = generate_noise_image(width, height)
            fname = os.path.join(IMAGES_DIR, f"noise_{size}x{size}.pgm")
            write_pgm(fname, pixels, width, height)

        if type_arg in ('binary', 'all'):
            pixels = generate_binary_image(width, height)
            fname = os.path.join(IMAGES_DIR, f"binary_{size}x{size}.pgm")
            write_pgm(fname, pixels, width, height)

    print("Done generating test images.")


if __name__ == '__main__':
    main()
