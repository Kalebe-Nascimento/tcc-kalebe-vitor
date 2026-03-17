/*
 * Parallel Connected Components using MPI.
 *
 * Strategy:
 * 1. Rank 0 reads image and broadcasts it.
 * 2. Each rank performs local BFS on its row partition, assigning local labels.
 * 3. Boundary rows are exchanged to resolve cross-boundary connections using
 *    a Union-Find data structure with global label offsets.
 * 4. All labels are gathered to rank 0, which performs a final relabeling pass
 *    and writes the output image.
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <numeric>
#include <unordered_map>
#include <mpi.h>

struct PGMImage {
    int width, height, maxval;
    std::vector<int> pixels;
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
    for (auto& p : img.pixels) file >> p;
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

// Simple Union-Find
struct UF {
    std::vector<int> parent;
    void init(int n) { parent.resize(n); std::iota(parent.begin(), parent.end(), 0); }
    int find(int x) { return parent[x] == x ? x : parent[x] = find(parent[x]); }
    void unite(int a, int b) { parent[find(a)] = find(b); }
};

// BFS within rows [startRow, startRow+numRows) of the full image
// Returns label array (same size as pixel subregion), and number of local components
int localBFS(const std::vector<int>& pixels, int width, int startRow, int numRows,
             std::vector<int>& labels, int labelOffset) {
    labels.assign(numRows * width, -1);
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    int numComp = 0;

    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < width; j++) {
            if (pixels[(startRow + i) * width + j] > 127 && labels[i * width + j] == -1) {
                int lbl = labelOffset + numComp;
                std::queue<std::pair<int,int>> q;
                q.push({i, j});
                labels[i * width + j] = lbl;
                while (!q.empty()) {
                    auto [r, c] = q.front(); q.pop();
                    for (int d = 0; d < 4; d++) {
                        int nr = r + dr[d];
                        int nc = c + dc[d];
                        if (nr >= 0 && nr < numRows && nc >= 0 && nc < width
                            && pixels[(startRow + nr) * width + nc] > 127
                            && labels[nr * width + nc] == -1) {
                            labels[nr * width + nc] = lbl;
                            q.push({nr, nc});
                        }
                    }
                }
                numComp++;
            }
        }
    }
    return numComp;
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

    int dims[3];
    std::vector<int> allPixels;

    if (rank == 0) {
        try {
            PGMImage img = readPGM(argv[1]);
            dims[0] = img.width; dims[1] = img.height; dims[2] = img.maxval;
            allPixels = img.pixels;
            std::cout << "Image loaded: " << img.width << "x" << img.height << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Bcast(dims, 3, MPI_INT, 0, MPI_COMM_WORLD);
    int width = dims[0], height = dims[1];

    allPixels.resize(width * height);
    MPI_Bcast(allPixels.data(), width * height, MPI_INT, 0, MPI_COMM_WORLD);

    int baseRows = height / size;
    int extra = height % size;
    int myRows = baseRows + (rank < extra ? 1 : 0);
    int startRow = rank * baseRows + std::min(rank, extra);

    // Compute label offset so each rank uses unique label ranges
    // Each rank gets at most (myRows * width) labels
    std::vector<int> rowCounts(size);
    MPI_Allgather(&myRows, 1, MPI_INT, rowCounts.data(), 1, MPI_INT, MPI_COMM_WORLD);
    int labelOffset = 0;
    for (int i = 0; i < rank; i++) labelOffset += rowCounts[i] * width;

    double startTime = MPI_Wtime();

    std::vector<int> localLabels;
    int localCount = localBFS(allPixels, width, startRow, myRows, localLabels, labelOffset);

    // Exchange boundary rows with neighbors to merge cross-boundary components
    // Send bottom row to next rank, top row to previous rank
    // Then use Union-Find to merge labels at boundaries

    int maxLabels = width * height;
    UF uf;
    uf.init(maxLabels);

    // Exchange with rank+1: send my bottom row, receive their top row
    if (rank < size - 1) {
        int nextRank = rank + 1;
        std::vector<int> myBottomRow(width), theirTopRow(width);
        std::vector<int> myBottomLabels(width), theirTopLabels(width);

        // Pixels
        for (int j = 0; j < width; j++) {
            myBottomRow[j] = allPixels[(startRow + myRows - 1) * width + j];
            myBottomLabels[j] = localLabels[(myRows - 1) * width + j];
        }

        MPI_Sendrecv(myBottomRow.data(), width, MPI_INT, nextRank, 0,
                     theirTopRow.data(), width, MPI_INT, nextRank, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(myBottomLabels.data(), width, MPI_INT, nextRank, 2,
                     theirTopLabels.data(), width, MPI_INT, nextRank, 3,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Merge labels at boundary
        for (int j = 0; j < width; j++) {
            if (myBottomRow[j] > 127 && theirTopRow[j] > 127
                && myBottomLabels[j] != -1 && theirTopLabels[j] != -1) {
                uf.unite(myBottomLabels[j], theirTopLabels[j]);
            }
        }
    }

    if (rank > 0) {
        int prevRank = rank - 1;
        std::vector<int> myTopRow(width), theirBottomRow(width);
        std::vector<int> myTopLabels(width), theirBottomLabels(width);

        for (int j = 0; j < width; j++) {
            myTopRow[j] = allPixels[startRow * width + j];
            myTopLabels[j] = localLabels[j];
        }

        MPI_Sendrecv(myTopRow.data(), width, MPI_INT, prevRank, 1,
                     theirBottomRow.data(), width, MPI_INT, prevRank, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(myTopLabels.data(), width, MPI_INT, prevRank, 3,
                     theirBottomLabels.data(), width, MPI_INT, prevRank, 2,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int j = 0; j < width; j++) {
            if (myTopRow[j] > 127 && theirBottomRow[j] > 127
                && myTopLabels[j] != -1 && theirBottomLabels[j] != -1) {
                uf.unite(myTopLabels[j], theirBottomLabels[j]);
            }
        }
    }

    // Apply union-find to local labels
    for (auto& lbl : localLabels) {
        if (lbl != -1) lbl = uf.find(lbl);
    }

    double endTime = MPI_Wtime();

    // Gather all labels to rank 0
    std::vector<int> recvCounts(size), displs(size);
    for (int i = 0; i < size; i++) {
        int r = baseRows + (i < extra ? 1 : 0);
        recvCounts[i] = r * width;
        displs[i] = (i * baseRows + std::min(i, extra)) * width;
    }

    std::vector<int> allLabels;
    if (rank == 0) allLabels.resize(width * height);

    MPI_Gatherv(localLabels.data(), myRows * width, MPI_INT,
                allLabels.data(), recvCounts.data(), displs.data(), MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Renumber components
        std::unordered_map<int, int> labelMap;
        int compCount = 0;
        std::vector<int> outPixels(width * height, 0);
        for (int i = 0; i < width * height; i++) {
            if (allLabels[i] != -1) {
                int root = allLabels[i]; // already find()-ed above, but do final find
                if (labelMap.find(root) == labelMap.end())
                    labelMap[root] = ++compCount;
                outPixels[i] = labelMap[root];
            }
        }

        // Normalize to 0-255
        for (int i = 0; i < width * height; i++) {
            if (allLabels[i] == -1)
                outPixels[i] = 0;
            else if (compCount <= 1)
                outPixels[i] = 255;
            else
                outPixels[i] = (outPixels[i] * 255) / compCount;
        }

        double elapsed = (endTime - startTime) * 1000.0;
        std::cout << "Connected components found: " << compCount << "\n";
        std::cout << "Connected components (parallel, " << size << " procs) time: " << elapsed << " ms\n";

        PGMImage out;
        out.width = width; out.height = height; out.maxval = 255;
        out.pixels = outPixels;
        try {
            writePGM(argv[2], out);
            std::cout << "Output written to " << argv[2] << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}
