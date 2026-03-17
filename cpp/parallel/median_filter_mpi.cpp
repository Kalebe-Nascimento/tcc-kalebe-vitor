#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <mpi.h>

struct PGMImage {
    int width, height, maxval;
    std::vector<int> pixels; // flat 1D array, row-major
};

PGMImage readPGM(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open file: " + filename);

    PGMImage img;
    std::string magic;
    file >> magic;
    if (magic != "P2") throw std::runtime_error("Not a P2 PGM file");

    std::string line;
    std::getline(file, line);
    while (file.peek() == '#') std::getline(file, line);

    file >> img.width >> img.height >> img.maxval;
    img.pixels.resize(img.width * img.height);
    for (int i = 0; i < img.width * img.height; i++)
        file >> img.pixels[i];

    return img;
}

void writePGM(const std::string& filename, const PGMImage& img) {
    std::ofstream file(filename);
    if (!file) throw std::runtime_error("Cannot open output file: " + filename);
    file << "P2\n" << img.width << " " << img.height << "\n" << img.maxval << "\n";
    for (int i = 0; i < img.height; i++) {
        for (int j = 0; j < img.width; j++) {
            file << img.pixels[i * img.width + j];
            if (j < img.width - 1) file << " ";
        }
        file << "\n";
    }
}

int medianOf(std::vector<int>& v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

// Apply median filter to rows [startRow, endRow) of the full image buffer
// localData includes halo rows: row (startRow-1) at index 0, actual rows start at index 1
// Returns filtered pixels for rows [startRow, endRow)
std::vector<int> filterRows(const std::vector<int>& data, int width, int localRows, int half = 1) {
    // data has (localRows + 2*half) rows
    int totalRows = localRows + 2 * half;
    std::vector<int> result(localRows * width);

    for (int i = 0; i < localRows; i++) {
        int actualI = i + half; // index in data
        for (int j = 0; j < width; j++) {
            std::vector<int> neighbors;
            for (int ki = -half; ki <= half; ki++) {
                for (int kj = -half; kj <= half; kj++) {
                    int ni = actualI + ki;
                    int nj = std::max(0, std::min(width - 1, j + kj));
                    // clamp ni
                    if (ni < 0) ni = 0;
                    if (ni >= totalRows) ni = totalRows - 1;
                    neighbors.push_back(data[ni * width + nj]);
                }
            }
            result[i * width + j] = medianOf(neighbors);
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) std::cerr << "Usage: " << argv[0] << " <input.pgm> <output.pgm>\n";
        MPI_Finalize();
        return 1;
    }

    int dims[3]; // width, height, maxval
    std::vector<int> allPixels;

    if (rank == 0) {
        try {
            PGMImage img = readPGM(argv[1]);
            dims[0] = img.width;
            dims[1] = img.height;
            dims[2] = img.maxval;
            allPixels = img.pixels;
            std::cout << "Image loaded: " << img.width << "x" << img.height << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(dims, 3, MPI_INT, 0, MPI_COMM_WORLD);
    int width = dims[0], height = dims[1], maxval = dims[2];

    if (rank != 0) allPixels.resize(width * height);
    MPI_Bcast(allPixels.data(), width * height, MPI_INT, 0, MPI_COMM_WORLD);

    // Determine row distribution
    int baseRows = height / size;
    int extra = height % size;
    int myRows = baseRows + (rank < extra ? 1 : 0);
    int startRow = rank * baseRows + std::min(rank, extra);

    // Build local data with halo rows
    int half = 1;
    std::vector<int> localData((myRows + 2 * half) * width);
    for (int i = -half; i < myRows + half; i++) {
        int srcRow = startRow + i;
        srcRow = std::max(0, std::min(height - 1, srcRow));
        int dstIdx = (i + half) * width;
        for (int j = 0; j < width; j++)
            localData[dstIdx + j] = allPixels[srcRow * width + j];
    }

    double startTime = MPI_Wtime();
    std::vector<int> localResult = filterRows(localData, width, myRows, half);
    double endTime = MPI_Wtime();

    // Gather results
    std::vector<int> recvCounts(size), displs(size);
    for (int i = 0; i < size; i++) {
        int r = baseRows + (i < extra ? 1 : 0);
        recvCounts[i] = r * width;
        displs[i] = (i * baseRows + std::min(i, extra)) * width;
    }

    std::vector<int> result;
    if (rank == 0) result.resize(width * height);

    MPI_Gatherv(localResult.data(), myRows * width, MPI_INT,
                result.data(), recvCounts.data(), displs.data(), MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        double elapsed = (endTime - startTime) * 1000.0;
        std::cout << "Median filter (parallel, " << size << " procs) time: " << elapsed << " ms\n";

        PGMImage out;
        out.width = width; out.height = height; out.maxval = maxval;
        out.pixels = result;
        try {
            writePGM(argv[2], out);
            std::cout << "Output written to " << argv[2] << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error writing output: " << e.what() << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}
