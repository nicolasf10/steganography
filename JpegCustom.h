#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "CImg.h"
#include "StegoLib.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <map>
#include <fstream>
#include <bitset>
#include <chrono>

#define INPUT_JPEG_SIZE (4 * 4 * 8 * 8 * 3)
#define PROCESSED_JPEG_INPUT (16)

using namespace std;

// Static variables
static const int zigzagOrder[64][2] = {{0, 0}, {0, 1}, {1, 0}, {2, 0}, {1, 1}, {0, 2}, {0, 3}, {1, 2}, {2, 1}, {3, 0}, {4, 0}, {3, 1}, {2, 2}, {1, 3}, {0, 4}, {0, 5}, {1, 4}, {2, 3}, {3, 2}, {4, 1}, {5, 0}, {6, 0}, {5, 1}, {4, 2}, {3, 3}, {2, 4}, {1, 5}, {0, 6}, {0, 7}, {1, 6}, {2, 5}, {3, 4}, {4, 3}, {5, 2}, {6, 1}, {7, 0}, {7, 1}, {6, 2}, {5, 3}, {4, 4}, {3, 5}, {2, 6}, {1, 7}, {2, 7}, {3, 6}, {4, 5}, {5, 4}, {6, 3}, {7, 2}, {7, 3}, {6, 4}, {5, 5}, {4, 6}, {3, 7}, {4, 7}, {5, 6}, {6, 5}, {7, 4}, {7, 5}, {6, 6}, {5, 7}, {6, 7}, {7, 6}, {7, 7}};

// Struct definitions
typedef struct quantizationTables {
    int luminance[8][8];
    int chrominance[8][8];
} quantizationTable;

typedef struct DCTBlock {
    int **Y;
    int **Cb;
    int **Cr;
} DCTBlock;

typedef struct RunLengthPair {
    int run;
    int value;
} RunLengthPair;

// Enum definitions
//enum class encoding_status {
//    MESSAGE,
//    TERMINATOR,
//    DONE
//};


class HuffmanNode {
public:
    int data; // For simplicity, assume we're dealing with characters.
    unsigned frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(int data, unsigned frequency)
            : data(data), frequency(frequency), left(nullptr), right(nullptr) {}

    HuffmanNode(unsigned frequency, HuffmanNode* left, HuffmanNode* right)
            : data(-1), frequency(frequency), left(left), right(right) {}

    HuffmanNode(unsigned frequency)
            : data(-1), frequency(frequency), left(nullptr), right(nullptr) {}
};

// Comparator for the priority queue
class Compare {
public:
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->frequency > r->frequency;
    }
};

// Enum definitions
enum Channel {
    Y,
    Cb,
    Cr
};

class JpegImage
{
public:
    // Constructor
    JpegImage() {
        // Initialize to default quality of 50
        quality = 50;

        // Set debugging attribute
        isDebugging = false;

        // Set last time
        lastTime = chrono::steady_clock::now();

        // Precomputing Cm, Cn, cosX, cosY for faster DCT-II implementation
        for (int m = 0; m < M; ++m) {
            Cm[m] = (m == 0) ? sqrt(1.0 / M) : sqrt(2.0 / M);
            for (int x = 0; x < M; ++x) {
                cosX[m][x] = cos((2 * x + 1) * m * M_PI / (2.0 * M));
            }
        }

        for (int n = 0; n < N; ++n) {
            Cn[n] = (n == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
            for (int y = 0; y < N; ++y) {
                cosY[n][y] = cos((2 * y + 1) * n * M_PI / (2.0 * N));
            }
        }

        // Initialize AAN constants
        initAAN();

        // Initialize luminance and chrominance tables
        setQuantizationTables(quality);
    }

    // Destructor
    ~JpegImage() {

    }

    // Log function
    void log(const std::string& message) {
        if (!isDebugging) {
            return;
        }
        auto now = chrono::steady_clock::now();
        chrono::duration<double> elapsed = now - lastTime;
        cout << message << " | " << elapsed.count() << " seconds)" << endl;
        lastTime = now; // Update lastTime to the current time
    }

    void initAAN() {
        for (int i = 0; i < N; ++i) {
            alpha[i] = (i == 0) ? sqrt(1.0 / N) : sqrt(2.0 / N);
            for (int j = 0; j < N; ++j) {
                cosines[i * N + j] = cos((M_PI / N) * i * (j + 0.5));
            }
        }
    }

    void setQuality(int q) {
        quality = q;
        setQuantizationTables(q);
    }

    // Attributes
    int width{};
    int height;
    bool successfullyEncoded = false;

    // Pixel data
    vector<vector<color>> pixelsRGB;
    vector<vector<ycbcr>> pixelsYCbCr;

    // DCT Values
    vector<vector<DCTBlock*>> dctBlocks; // 8x8 grid with DCT coefficients
    vector<vector<DCTBlock*>> quantizedBlocks; // 8x8 grid with quantized DCT coefficients

    // Precomputed values
    const static int M = 8, N = 8;
    double Cm[M], Cn[N], cosX[M][N], cosY[M][N];

    double alpha[N], cosines[N * N];

    quantizationTables quantTables;

    bool rgbLoaded = false;
    bool ycbcrLoaded = false;
    bool dctBlocksGenerated = false;
    bool quantizedBlocksGenerated = false;
    bool isDebugging;
    chrono::steady_clock::time_point lastTime;


    //bool load(string filename);
    //bool save(string filename);

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    // Encoding and decoding main functions
    void encodeJpeg(const std::string& outputFilename);
    void encodeJpeg(const std::string& outputFilename, const bool useStego, const std::string& message);
    void encodeJpeg(const string& outputFilename, const string& message);
    void decodeJpeg(const std::string& outputFilename);


    // File operations
    string readEncodedDataFromFile(const string& filePath, int starting_byte, int num_bytes);
    void writeEncodedDataToFile(const string& encodedData, const string& filePath);
    void appendEncodedDataToFile(const string& encodedData, const string& filePath);

    // Basic methods
    void loadPng(string filename);
    void savePng(string filename);
    void rgbToYCbCr();
    void yCbCrToRGB();

    // Main operations
    void generateDCTBlocks();
    void invertDCTBlocks(); // Converts DCT blocks to pixel data (YCbCr)
    void generateQuantizedBlocks();
    void dequantizeBlocks(); // Converts quantized DCT blocks to DCT blocks

    // RLE Functions
    // Encoding - given zigzagged vector of quantized DCT coefficients, encode it using RLE
    void encodeRLE(vector<int> &input, vector<int> &output); // returns RLE encoding of a vector of ints | 3, 2, 1, 4 = 3 3 1 1 1 1
    void decodeRLE(vector<int> &input, vector<int> &output, int size); // returns RLE decoding of a vector of ints

    // Zigzag scanning
    void zigzag(vector<int> &sequence); // returns zigzagged vector of quantized DCT coefficients
    void zigzagBlock(int **block, vector<int> &sequence); // zigzagging of a vector of ints
    void zigzagDCTBlock(DCTBlock *block, vector<int> &sequence); // zigzagging of a DCT block
    void inverseZigzag(vector<int> &sequence); // inverse zigzagging of a vector of ints
    void inverseZigzagBlock(vector<int> &sequence, int **block, int start_index);
    void inverseZigzagDCTBlock(vector<int> &sequence, DCTBlock *block, int start_index);

    // Huffman encoding
    // 1. Given vector of ints (after RLE encoding), calculate frequencies
    // 2. Build Huffman tree
    // 3. Generate Huffman codes
    // 4. Encode data into string of codes

    static map<int, int> calculateFrequencies(const vector<int>& data);
    HuffmanNode* buildHuffmanTree(const map<int, int>& frequencies);
    map<int, string> generateHuffmanCodes(const HuffmanNode* node);
    void generateHuffmanCodesRecursive(const HuffmanNode* node, const string& code, map<int, string>& huffmanCodes);
    string encodeData(const vector<int>& data, const map<int, string>& huffmanCodes);
    vector<int> decodeData(const string& encodedData, HuffmanNode* huffmanTree);

    // Block operations
    // DCT II Operations - blocks are always 8x8
    void setQuantizationTables(int quality = 50);
    void dct2OnBlock(int **block, int **result);
    void idct2OnBlock(int **block, int **result);
    void dct2OnAllBlockChannels(DCTBlock *input, DCTBlock *output);
    void idct2OnAllBlockChannels(DCTBlock *input, DCTBlock *output);

    // Memory operations
    static DCTBlock* allocateDCTBlock();
    static int** allocateMatrix(int rows, int cols);
    static void deallocateMatrix(int** matrix, int rows);
    static void deallocateDCTBlock(DCTBlock* block);

    // Quantization and Dequantization
    void quantizeBlock(int **block, Channel channel);
    void dequantizeBlock(int **block, Channel channel);
    void quantizeBlockChannels(DCTBlock *block);
    void dequantizeBlockChannels(DCTBlock *block);

    // Steganography operations
    void encodeLSBOnQuantizedBlocks(const string& message);
    string decodeLSBOnQuantizedBlocks();

    // View image
    void displayImage();

private:
    int quality;
};