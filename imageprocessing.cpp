#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <omp.h>

using namespace std;

// 3 by 3 edge detection convolution filter (kernel)
double edgeDetection[3][3] = {
    {-0.0625, -0.0625, -0.0625},
    {-0.0625,  0.5,    -0.0625},
    {-0.0625, -0.0625, -0.0625}
};

// Function that implements the application of the filter on every pixel of the image
double applyConvolution(std::vector<std::vector<int>>& matrix, size_t x, size_t y) {
    double result = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result += edgeDetection[i][j] * matrix[x - 1 + i][y - 1 + j];
        }
    }
    if (result < 0) return 0;
    return result;
}

// Writes out the file as a clean, universally compatible Plain Text ASCII P2 file
bool writeResults(std::string& header, std::vector<std::vector<int>>& resultMatrix) {
    ofstream writeFile("./imageprocessed.pgm");
    if (!writeFile.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
        return false;
    }
    writeFile << header;
    for (const auto& row : resultMatrix) {
        for (int pixel : row) {
            writeFile << pixel << "\n";
        }
    }
    writeFile.close();
    return true;
}

int main(int argc, char *argv[]) {
    string imagepath;
    std::cout << "Enter image path:" << std::endl;
    cin >> imagepath;

    // Open in binary mode so we can safely parse P2 or P5 data structures
    std::ifstream image(imagepath, std::ios::binary);
    if (!image.is_open()) {
        cout << "Unable to open file." << endl;
        return 0;
    }

    string magic, comment, dimensions, maximumgrey;
    
    // Read headers, ignoring trailing carriage returns safely
    getline(image, magic); if(!magic.empty() && magic.back() == '\r') magic.pop_back();
    
    // Peak ahead to see if the second line is a comment or dimensions
    streampos pos = image.tellg();
    getline(image, comment); if(!comment.empty() && comment.back() == '\r') comment.pop_back();
    if (comment[0] != '#') {
        dimensions = comment;
        comment = "# Processed with OpenMP";
    } else {
        getline(image, dimensions); if(!dimensions.empty() && dimensions.back() == '\r') dimensions.pop_back();
    }
    
    getline(image, maximumgrey); if(!maximumgrey.empty() && maximumgrey.back() == '\r') maximumgrey.pop_back();

    int space = dimensions.find_first_of(" \t");
    string width = dimensions.substr(0, space);
    string height = dimensions.substr(space + 1);

    int columns = stoi(width);
    int rows = stoi(height);

    std::cout << "Format Type detected: " << magic << std::endl;
    std::cout << "Dimensions rows: " << rows << std::endl;
    std::cout << "Dimensions columns: " << columns << std::endl;

    // Allocate matrix with zero-padding borders for convolution support
    std::vector<std::vector<int>> pixels(rows + 2, std::vector<int>(columns + 2, 0));

    auto startReading = std::chrono::high_resolution_clock::now();

    if (magic == "P5") {
        // High Performance parsing for Binary files
        std::vector<unsigned char> buffer(rows * columns);
        image.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        
        int idx = 0;
        for (int r = 1; r <= rows; r++) {
            for (int c = 1; c <= columns; c++) {
                pixels[r][c] = buffer[idx++];
            }
        }
    } else {
        // Standby parser for Plain Text ASCII files
        int val;
        for (int r = 1; r <= rows; r++) {
            for (int c = 1; c <= columns; c++) {
                if (image >> val) {
                    pixels[r][c] = val;
                }
            }
        }
    }
    image.close();

    auto stopReading = std::chrono::high_resolution_clock::now();
    double totalReadingms = std::chrono::duration_cast<std::chrono::nanoseconds>(stopReading - startReading).count() / 1e+6;
    cout << "Total time reading image: " << totalReadingms << " ms" << endl;
	
    string iprocessing_type;
    cout << "Enter operation (edgedetection, brighten, rotate, reflect):" << endl;
    cin >> iprocessing_type;
  
    for (size_t i = 0; i < iprocessing_type.length(); i++) {
        iprocessing_type[i] = tolower(iprocessing_type[i]);
    }
  
    vector<vector<int>> result(rows, vector<int>(columns, 0));
  
    auto startProcessing = std::chrono::high_resolution_clock::now();

    // Multi-threaded calculation engine
    #pragma omp parallel for collapse(2)
    for (int x = 1; x <= rows; x++) {
        for (int y = 1; y <= columns; y++) {
            if (iprocessing_type == "edgedetection") {
                result[x - 1][y - 1] = round(applyConvolution(pixels, x, y));
            }
            else if (iprocessing_type == "brighten") {
                int r = round((pixels[x][y]) * 1.5);
                result[x - 1][y - 1] = (r > 255) ? 255 : r;
            }
            else if (iprocessing_type == "rotate") {
                result[x - 1][y - 1] = pixels[rows - x + 1][columns - y + 1];
            }
            else if (iprocessing_type == "reflect") {
                result[x - 1][y - 1] = pixels[x][columns - y + 1];
            }
        }
    }
  
    auto stopProcessing = std::chrono::high_resolution_clock::now();
    double totalms = std::chrono::duration_cast<std::chrono::nanoseconds>(stopProcessing - startProcessing).count() / 1e+6;
    cout << "Total time processing image: " << totalms << " ms" << endl;
 
    int maxGreyValue = 0;  
    for (const auto& r : result) {
        for (int pixel : r) {
            if (pixel > maxGreyValue) maxGreyValue = pixel;
        }
    }
  
    // We export as P2 so you can inspect the output pixels inside a standard text editor if needed
    string outHeader = "P2\n" + comment + "\n" + dimensions + "\n" + to_string(maxGreyValue) + "\n";

    if (writeResults(outHeader, result)) {
        cout << "File saved successfully as 'imageprocessed.pgm'!" << endl;
    }

    return 0;
}
