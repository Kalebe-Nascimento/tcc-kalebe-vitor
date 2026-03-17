#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
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

    // Skip comments
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

PGMImage applyMedianFilter(const PGMImage& input, int kernelSize = 3) {
    PGMImage output = input;
    int half = kernelSize / 2;

    for (int i = 0; i < input.height; i++) {
        for (int j = 0; j < input.width; j++) {
            std::vector<int> neighbors;
            for (int ki = -half; ki <= half; ki++) {
                for (int kj = -half; kj <= half; kj++) {
                    int ni = std::max(0, std::min(input.height - 1, i + ki));
                    int nj = std::max(0, std::min(input.width - 1, j + kj));
                    neighbors.push_back(input.pixels[ni][nj]);
                }
            }
            std::sort(neighbors.begin(), neighbors.end());
            output.pixels[i][j] = neighbors[neighbors.size() / 2];
        }
    }
    return output;
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
        PGMImage result = applyMedianFilter(img, 3);
        auto end = std::chrono::high_resolution_clock::now();

        double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        std::cout << "Median filter (sequential) time: " << elapsed << " ms\n";

        writePGM(argv[2], result);
        std::cout << "Output written to " << argv[2] << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
