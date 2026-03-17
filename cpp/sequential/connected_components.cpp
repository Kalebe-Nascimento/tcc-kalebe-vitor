#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>

struct PGMImage {
    int width, height, maxval;
    std::vector<std::vector<int>> pixels;
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

    img.pixels.resize(img.height, std::vector<int>(img.width));
    for (int i = 0; i < img.height; i++)
        for (int j = 0; j < img.width; j++)
            file >> img.pixels[i][j];

    return img;
}

void writePGM(const std::string& filename, const PGMImage& img) {
    std::ofstream file(filename);
    if (!file) throw std::runtime_error("Cannot open output file: " + filename);
    file << "P2\n" << img.width << " " << img.height << "\n" << img.maxval << "\n";
    for (int i = 0; i < img.height; i++) {
        for (int j = 0; j < img.width; j++) {
            file << img.pixels[i][j];
            if (j < img.width - 1) file << " ";
        }
        file << "\n";
    }
}

int connectedComponents(const PGMImage& input, PGMImage& output) {
    output = input;
    int height = input.height, width = input.width;
    std::vector<std::vector<int>> labels(height, std::vector<int>(width, -1));

    int numComponents = 0;
    // 4-connectivity directions
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (input.pixels[i][j] > 127 && labels[i][j] == -1) {
                // BFS
                std::queue<std::pair<int,int>> q;
                q.push({i, j});
                labels[i][j] = numComponents;
                while (!q.empty()) {
                    auto [r, c] = q.front(); q.pop();
                    for (int d = 0; d < 4; d++) {
                        int nr = r + dr[d];
                        int nc = c + dc[d];
                        if (nr >= 0 && nr < height && nc >= 0 && nc < width
                            && input.pixels[nr][nc] > 127 && labels[nr][nc] == -1) {
                            labels[nr][nc] = numComponents;
                            q.push({nr, nc});
                        }
                    }
                }
                numComponents++;
            }
        }
    }

    // Normalize labels to 0-255
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (labels[i][j] == -1)
                output.pixels[i][j] = 0;
            else if (numComponents <= 1)
                output.pixels[i][j] = 255;
            else
                output.pixels[i][j] = (labels[i][j] * 255) / (numComponents - 1);
        }
    }
    output.maxval = 255;
    return numComponents;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.pgm> <output.pgm>\n";
        return 1;
    }

    try {
        PGMImage img = readPGM(argv[1]);
        std::cout << "Image loaded: " << img.width << "x" << img.height << "\n";

        auto start = std::chrono::high_resolution_clock::now();
        PGMImage result;
        int numComp = connectedComponents(img, result);
        auto end = std::chrono::high_resolution_clock::now();

        double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Connected components found: " << numComp << "\n";
        std::cout << "Connected components (sequential) time: " << elapsed << " ms\n";

        writePGM(argv[2], result);
        std::cout << "Output written to " << argv[2] << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
