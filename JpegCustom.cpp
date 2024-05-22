#include <queue>
#include <algorithm>
#include "JpegCustom.h"

using namespace std;
using namespace cimg_library;

static const int OFFSET = 0;

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

// encodeJpeg()
// Description: Encodes an image to a (custom) jpg file - no steganography
// Input: string outputFilename - path to the output jpg file
// Output: No return value, modifies the output jpg file
void JpegImage::encodeJpeg(const std::string& outputFilename) {
    encodeJpeg(outputFilename, false, "");
}

// encodeJpeg()
// Description: Encodes an image to a (custom) jpg file - with steganography
// Input: string outputFilename - path to the output jpg file
//        string message - message to be encoded
// Output: No return value, modifies the output jpg file
void JpegImage::encodeJpeg(const string& outputFilename, const string& message) {
    encodeJpeg(outputFilename, true, message);
}

// encodeJpeg()
// Description: Encodes an image to a (custom) jpg file
// Input: string outputFilename - path to the output jpg file
// Output: No return value, modifies the output jpg file
void JpegImage::encodeJpeg(const std::string& outputFilename, const bool useStego, const std::string& message) {
    if (!ycbcrLoaded && !rgbLoaded) {
        cout << "No data loaded - JpegImage::encodeJpeg" << endl;
        return;
    }

    // If RGB data is loaded, convert to YCbCr
    if (rgbLoaded && !ycbcrLoaded) {
        log("Converting RGB to YCbCr");
        rgbToYCbCr();
    }

    // If YCbCr data is loaded, generate DCT blocks
    if (ycbcrLoaded) {
        log("Generating DCT blocks");
        generateDCTBlocks();
    }

    // If DCT blocks are generated, quantize the blocks
    if (dctBlocksGenerated) {
        log("Quantizing blocks");
        generateQuantizedBlocks();
    }

    if (useStego) {
        log("Encoding message within LSB of quantized DCT coefficients");
        encodeLSBOnQuantizedBlocks(message);
    }

    successfullyEncoded = true;

    // If quantized blocks are generated, convert to zigzag sequence
    log("Converting to zigzag sequence");
    vector<int> sequence;
    zigzag(sequence);

    // Perform RLE encoding on the zigzag sequence
    log("Performing RLE encoding");
    vector<int> rleSequence;
    encodeRLE(sequence, rleSequence);

    // Perform Huffman encoding on the RLE sequence
    log("Performing Huffman encoding");

    log("Calculating frequencies");
    map<int, int> frequencies = calculateFrequencies(rleSequence);

    log("Building Huffman tree");
    HuffmanNode* huffmanTree = buildHuffmanTree(frequencies);

    log("Generating Huffman codes");
    map<int, string> huffmanCodes = generateHuffmanCodes(huffmanTree);

    log("Encoding data with Huffman codes");
    string encodedData = encodeData(rleSequence, huffmanCodes);

    // Opening file for writing if encoding is successful
    if (!successfullyEncoded) {
        cout << "Failed to encode image - JpegImage::encodeJpeg" << endl;
        return;
    }

    log("Writing to file");
    ofstream file(outputFilename, ios::binary);

    // Writing jpeg quality
    file.write((char*)&quality, sizeof(int));

    // Writing integer of height and width
    file.write((char*)&height, sizeof(int));
    file.write((char*)&width, sizeof(int));

    // Writing size of frequency table
    unsigned int freqSize = frequencies.size();
    file.write((char*)&freqSize, sizeof(unsigned int));

    // Writing frequency table
    log("Writing frequency table");
    for (auto const& x : frequencies) {
        file.write((char*)&x.first, sizeof(int));
        file.write((char*)&x.second, sizeof(int));
    }

    // Writing size of encoded data
    int encodedSize = encodedData.size() / 8;
    file.write((char*)&encodedSize, sizeof(int));

    // Write RLE sequence size
    int rleSize = rleSequence.size();
    file.write((char*)&rleSize, sizeof(int));

    log("Writing RLE sequence size: " + to_string(rleSize));

    log("Writing encoded data size: " + to_string(encodedSize));

    // Closing file and appending encoded data
    file.close();

    log("Appending encoded data");
    appendEncodedDataToFile(encodedData, outputFilename);
    cout << "Encoded data appended to file " << outputFilename << endl;

    // Success message
    log("Image successfully encoded to " + outputFilename);
}


// decodeJpeg()
// Description: Decodes a (custom) jpg file to an image
// Input: string inputFilename - path to the input jpg file
void JpegImage::decodeJpeg(const std::string& inputFilename) {
    // Variable declaration
    int fileQuality;

    // Opening file for reading
    log("Reading from file");
    ifstream file(inputFilename, ios::binary);

    // Reading jpeg quality
    file.read((char*)&fileQuality, sizeof(int));
    setQuality(fileQuality);

    log("Jpeg quality: " + to_string(fileQuality));

    // Reading height and width
    file.read((char*)&height, sizeof(int));
    file.read((char*)&width, sizeof(int));

    log("Height: " + to_string(height) + ", Width: " + to_string(width));

    // Reading size of frequency table
    unsigned int freqSize;
    file.read((char*)&freqSize, sizeof(unsigned int));

    // Reading frequency table
    log("Reading frequency table");

    map<int, int> frequencies;
    for (int i = 0; i < freqSize; i++) {
        int key, value;
        file.read((char*)&key, sizeof(int));
        file.read((char*)&value, sizeof(int));
        frequencies[key] = value;
    }

    // Reading size of encoded data
    int encodedSize;
    file.read((char*)&encodedSize, sizeof(int));

    log("Encoded data size: " + to_string(encodedSize));

    // Read RLE sequence size
    int rleSize;
    file.read((char*)&rleSize, sizeof(int));

    log("RLE sequence size: " + to_string(rleSize));

    // Starting byte
    int startByte = file.tellg();

    // Closing file
    file.close();

    // Reading encoded data
    log("Reading encoded data from file");

    string encodedData = readEncodedDataFromFile(inputFilename, startByte, encodedSize);

    // Generating Huffman tree
    log("Building Huffman tree");

    HuffmanNode* huffmanTree = buildHuffmanTree(frequencies);

    // Decoding Huffman encoded data
    log("Decoding Huffman encoded data");

    vector<int> rleSequence = decodeData(encodedData, huffmanTree);

    // Decoding RLE sequence
    log("Decoding RLE sequence");

    vector<int> sequence;
    decodeRLE(rleSequence, sequence, rleSize);

    // Inverse zigzag
    log("Performing inverse zigzag");
    inverseZigzag(sequence);

    // Dequantize blocks
    log("Dequantizing blocks");
    dequantizeBlocks();

    // Invert DCT blocks
    log("Inverting DCT blocks");
    invertDCTBlocks();

    // Convert YCbCr to RGB
    log("Converting YCbCr to RGB");
    yCbCrToRGB();
}

// displayImage()
// Description: Displays the image using CImg
// Output: No return value, displays the image
void JpegImage::displayImage() {
    // Checking if RGB data is loaded
    if (!rgbLoaded && !ycbcrLoaded) {
        cout << "No data loaded - JpegImage::displayImage" << endl;
        return;
    }

    // Checking if RGB data is loaded
    if (!rgbLoaded) {
        log("Converting YCbCr to RGB");
        yCbCrToRGB();
    }

    // Creating a CImg object
    log("Creating CImg object");
    CImg<unsigned char> image(width, height, 1, 3);

    // Filling the CImg object with pixel data
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            image(j, i, 0, 0) = pixelsRGB[i][j].r;
            image(j, i, 0, 1) = pixelsRGB[i][j].g;
            image(j, i, 0, 2) = pixelsRGB[i][j].b;
        }
    }

    // Displaying the image
    log("Displaying image");
    CImgDisplay display(image, "Loaded Image");

    // Move the display window and set display size
    display.move(100, 100);
    display.resize(800, (int)(800 * ((double)height / width)));

    // Wait for the user to close the display window
    while (!display.is_closed()) {
        display.wait();
    }
}

// loadPng()
// Description: Loads a PNG image and stores the RGB values in the pixels 2D vector
// Input: string filename - path to the PNG image
// Output: No return value, modifies the pixels 2D vector attribute
void JpegImage::loadPng(string filename) {
    CImg<unsigned char>* image;

    image = new CImg<unsigned char>(filename.c_str());

    // Getting image dimensions and resizing to multiples of 8
    width = image->width() - (image->width() % 8);
    height = image->height() - (image->height() % 8);

    // Defining size of the pixels 2D vector
    pixelsRGB.resize(height);
    for (int i = 0; i < height; i++) {
        pixelsRGB[i].resize(width);
    }

    cimg_forXY(*image, x, y) {
            if (x >= width || y >= height) {
                break;
            }

            // Access pixel values at (x, y)
            unsigned char pixelValue = (*image)(x, y, 0, 0);

            // Adding pixel data to the pixels 2D vector
            pixelsRGB[y][x].r = (*image)(x, y, 0, 0);
            pixelsRGB[y][x].g = (*image)(x, y, 0, 1);
            pixelsRGB[y][x].b = (*image)(x, y, 0, 2);
        }

    rgbLoaded = true;

    delete image;
}

// savePng()
// Description: Saves the RGB values in the pixels 2D vector to a PNG image
// Input: string filename - path to the output PNG image
// Output: No return value, saves the PNG image
void JpegImage::savePng(string filename) {
    // Checking if RGB data is loaded
    if (!rgbLoaded) {
        cout << "No RGB data loaded - JpegImage::savePng" << endl;
        return;
    }

    // Creating a CImg object
    CImg<unsigned char> image(width, height, 1, 3);

    // Filling the CImg object with pixel data
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            image(j, i, 0, 0) = pixelsRGB[i][j].r;
            image(j, i, 0, 0) = max(0, min(255, (int)image(j, i, 0, 0)));
            image(j, i, 0, 1) = pixelsRGB[i][j].g;
            image(j, i, 0, 1) = max(0, min(255, (int)image(j, i, 0, 1)));
            image(j, i, 0, 2) = pixelsRGB[i][j].b;
            image(j, i, 0, 2) = max(0, min(255, (int)image(j, i, 0, 2)));
        }
    }

    // Saving the image
    image.save(filename.c_str());

    // Success message
    cout << "Image successfully saved to " << filename << endl;
}

// rgbToYCbCr()
// Description: Converts RGB data to YCbCr and stores the values in the pixelsYCbCr 2D vector
// Output: No return value, modifies the pixelsYCbCr 2D vector attribute
void JpegImage::rgbToYCbCr() {
    // Checking if RGB data is loaded
    if (!rgbLoaded) {
        cout << "No RGB data loaded - JpegImage::rgbToYCbCr" << endl;
        return;
    }

    pixelsYCbCr.resize(height);

    // Converting RGB to YCbCr
    for (int i = 0; i < height; i++) {
        pixelsYCbCr[i].resize(width);

        for (int j = 0; j < width; j++) {
            pixelsYCbCr[i][j].y = min(255, max(0, int(0.299 * pixelsRGB[i][j].r + 0.587 * pixelsRGB[i][j].g + 0.114 * pixelsRGB[i][j].b)));
            pixelsYCbCr[i][j].cb = min(255, max(0, int(128 - 0.168736 * pixelsRGB[i][j].r - 0.331264 * pixelsRGB[i][j].g + 0.5 * pixelsRGB[i][j].b)));
            pixelsYCbCr[i][j].cr = min(255, max(0, int(128 + 0.5 * pixelsRGB[i][j].r - 0.418688 * pixelsRGB[i][j].g - 0.081312 * pixelsRGB[i][j].b)));
        }
    }

    ycbcrLoaded = true;
}

// yCbCrToRGB()
// Description: Converts YCbCr data to RGB and stores the values in the pixelsRGB 2D vector
// Output: No return value, modifies the pixelsRGB 2D vector attribute
void JpegImage::yCbCrToRGB() {
    // Checking if YCbCr data is loaded
    if (!ycbcrLoaded) {
        cout << "No YCbCr data loaded - JpegImage::yCbCrToRGB" << endl;
        return;
    }

    pixelsRGB.resize(height);

    // Converting YCbCr to RGB
    for (int i = 0; i < height; i++) {
        pixelsRGB[i].resize(width);

        for (int j = 0; j < width; j++) {
            pixelsRGB[i][j].r = min(255, max(0, int(pixelsYCbCr[i][j].y + 1.402 * (pixelsYCbCr[i][j].cr - 128))));
            pixelsRGB[i][j].g = min(255, max(0, int(pixelsYCbCr[i][j].y - 0.344136 * (pixelsYCbCr[i][j].cb - 128) - 0.714136 * (pixelsYCbCr[i][j].cr - 128))));
            pixelsRGB[i][j].b = min(255, max(0, int(pixelsYCbCr[i][j].y + 1.772 * (pixelsYCbCr[i][j].cb - 128))));
        }
    }

    rgbLoaded = true;
}

// ORIGINAL DCT FUNCTIONS
//void JpegImage::dct2OnBlock(int **block, int **result) {
//    double sum;
//    for (int m = 0; m < M; m++) {
//        for (int n = 0; n < N; n++) {
//            sum = 0;
//            for (int x = 0; x < M; x++) {
//                for (int y = 0; y < N; y++) {
//                    sum += (block[x][y] - OFFSET) * cosX[m][x] * cosY[n][y];
//                }
//            }
//            result[m][n] = round(Cm[m] * Cn[n] * sum);
//        }
//    }
//}
//
//void JpegImage::idct2OnBlock(int **block, int **result) {
//    double sum;
//    for (int x = 0; x < M; x++) {
//        for (int y = 0; y < N; y++) {
//            sum = 0;
//            for (int m = 0; m < M; m++) {
//                for (int n = 0; n < N; n++) {
//                    sum += Cm[m] * Cn[n] * block[m][n] * cosX[m][x] * cosY[n][y];
//                }
//            }
//            result[x][y] = round(sum + OFFSET);
//        }
//    }
//}
// END OF ORIGINAL DCT FUNCTIONS
//


//Compute forward dct
void JpegImage::dct2OnBlock(int** block, int** result) {
    float component[64]; // Temporary array to store the floating-point version of the block

    // Convert the int** input to a float* component
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            component[i * 8 + j] = static_cast<float>(block[i][j]);
        }
    }

    /*Forward DCT constants*/
    static const float a1 = 0.707;
    static const float a2 = 0.541;
    static const float a3 = 0.707;
    static const float a4 = 1.307;
    static const float a5 = 0.383;

    static const float s0 = 0.353553;
    static const float s1 = 0.254898;
    static const float s2 = 0.270598;
    static const float s3 = 0.300672;
    static const float s4 = s0;
    static const float s5 = 0.449988;
    static const float s6 = 0.653281;
    static const float s7 = 1.281458;

    /*Forward DCT constants*/
    for(int i = 0; i < 8; i++)
    {
        const float b0 = component[0*8 + i] + component[7*8 + i];
        const float b1 = component[1*8 + i] + component[6*8 + i];
        const float b2 = component[2*8 + i] + component[5*8 + i];
        const float b3 = component[3*8 + i] + component[4*8 + i];
        const float b4 =-component[4*8 + i] + component[3*8 + i];
        const float b5 =-component[5*8 + i] + component[2*8 + i];
        const float b6 =-component[6*8 + i] + component[1*8 + i];
        const float b7 =-component[7*8 + i] + component[0*8 + i];

        const float c0 = b0 + b3;
        const float c1 = b1 + b2;
        const float c2 =-b2 + b1;
        const float c3 =-b3 + b0;
        const float c4 =-b4 - b5;
        const float c5 = b5 + b6;
        const float c6 = b6 + b7;
        const float c7 = b7;

        const float d0 = c0 + c1;
        const float d1 =-c1 + c0;
        const float d2 = c2 + c3;
        const float d3 = c3;
        const float d4 = c4;
        const float d5 = c5;
        const float d6 = c6;
        const float d7 = c7;
        const float d8 = (d4+d6) * a5;

        const float e0 = d0;
        const float e1 = d1;
        const float e2 = d2 * a1;
        const float e3 = d3;
        const float e4 = -d4 * a2 - d8;
        const float e5 = d5 * a3;
        const float e6 = d6 * a4 - d8;
        const float e7 = d7;

        const float f0 = e0;
        const float f1 = e1;
        const float f2 = e2 + e3;
        const float f3 = e3 - e2;
        const float f4 = e4;
        const float f5 = e5 + e7;
        const float f6 = e6;
        const float f7 = e7 - e5;

        const float g0 = f0;
        const float g1 = f1;
        const float g2 = f2;
        const float g3 = f3;
        const float g4 = f4 + f7;
        const float g5 = f5 + f6;
        const float g6 = -f6 + f5;
        const float g7 = f7 - f4;

        component[0*8 + i] = g0 * s0;
        component[4*8 + i] = g1 * s4;
        component[2*8 + i] = g2 * s2;
        component[6*8 + i] = g3 * s6;
        component[5*8 + i] = g4 * s5;
        component[1*8 + i] = g5 * s1;
        component[7*8 + i] = g6 * s7;
        component[3*8 + i] = g7 * s3;
    }

    for(int i = 0; i < 8; i++)
    {
        const float b0 = component[i*8 + 0]  + component[i*8 + 7];
        const float b1 = component[i*8 + 1]  + component[i*8 + 6];
        const float b2 = component[i*8 + 2]  + component[i*8 + 5];
        const float b3 = component[i*8 + 3]  + component[i*8 + 4];
        const float b4 =-component[i*8 + 4]  + component[i*8 + 3];
        const float b5 =-component[i*8 + 5]  + component[i*8 + 2];
        const float b6 =-component[i*8 + 6]  + component[i*8 + 1];
        const float b7 =-component[i*8 + 7]  + component[i*8 + 0] ;

        const float c0 = b0 + b3;
        const float c1 = b1 + b2;
        const float c2 =-b2 + b1;
        const float c3 =-b3 + b0;
        const float c4 =-b4 - b5;
        const float c5 = b5 + b6;
        const float c6 = b6 + b7;
        const float c7 = b7;

        const float d0 = c0 + c1;
        const float d1 =-c1 + c0;
        const float d2 = c2 + c3;
        const float d3 = c3;
        const float d4 = c4;
        const float d5 = c5;
        const float d6 = c6;
        const float d7 = c7;
        const float d8 = (d4+d6) * a5;

        const float e0 = d0;
        const float e1 = d1;
        const float e2 = d2 * a1;
        const float e3 = d3;
        const float e4 = -d4 * a2 - d8;
        const float e5 = d5 * a3;
        const float e6 = d6 * a4 - d8;
        const float e7 = d7;

        const float f0 = e0;
        const float f1 = e1;
        const float f2 = e2 + e3;
        const float f3 = e3 - e2;
        const float f4 = e4;
        const float f5 = e5 + e7;
        const float f6 = e6;
        const float f7 = e7 - e5;

        const float g0 = f0;
        const float g1 = f1;
        const float g2 = f2;
        const float g3 = f3;
        const float g4 = f4 + f7;
        const float g5 = f5 + f6;
        const float g6 = -f6 + f5;
        const float g7 = f7 - f4;

        component[i*8 + 0] = g0 * s0;
        component[i*8 + 4] = g1 * s4;
        component[i*8 + 2] = g2 * s2;
        component[i*8 + 6] = g3 * s6;
        component[i*8 + 5] = g4 * s5;
        component[i*8 + 1] = g5 * s1;
        component[i*8 + 7] = g6 * s7;
        component[i*8 + 3] = g7 * s3;
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            result[i][j] = round(component[i * 8 + j] + 0.5f); // Round to nearest int
        }
    }
}

void JpegImage::idct2OnBlock(int** block, int** result) {
    static const float m0 = 2.0 * cos(1.0/16.0 * 2.0 * M_PI);
    static const float m1 = 2.0 * cos(2.0/16.0 * 2.0 * M_PI);
    static const float m3 = 2.0 * cos(2.0/16.0 * 2.0 * M_PI);
    static const float m5 = 2.0 * cos(3.0/16.0 * 2.0 * M_PI);
    static const float m2 = m0-m5;
    static const float m4 = m0+m5;

    static const float s0 = cos(0.0/16.0 *M_PI)/sqrt(8);
    static const float s1 = cos(1.0/16.0 *M_PI)/2.0;
    static const float s2 = cos(2.0/16.0 *M_PI)/2.0;
    static const float s3 = cos(3.0/16.0 *M_PI)/2.0;
    static const float s4 = cos(4.0/16.0 *M_PI)/2.0;
    static const float s5 = cos(5.0/16.0 *M_PI)/2.0;
    static const float s6 = cos(6.0/16.0 *M_PI)/2.0;
    static const float s7 = cos(7.0/16.0 *M_PI)/2.0;

    float component[64]; // Temporary array to store the floating-point version of the block

    // Convert the int** input to a float* component
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            component[i * 8 + j] = static_cast<float>(block[i][j]);
        }
    }

    for(int i = 0; i < 8; i++)
    {
        const float g0 = component[0*8 + i] * s0;
        const float g1 = component[4*8 + i] * s4;
        const float g2 = component[2*8 + i] * s2;
        const float g3 = component[6*8 + i] * s6;
        const float g4 = component[5*8 + i] * s5;
        const float g5 = component[1*8 + i] * s1;
        const float g6 = component[7*8 + i] * s7;
        const float g7 = component[3*8 + i] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        component[0 * 8 + i] = b0 + b7;
        component[1 * 8 + i] = b1 + b6;
        component[2 * 8 + i] = b2 + b5;
        component[3 * 8 + i] = b3 + b4;
        component[4 * 8 + i] = b3 - b4;
        component[5 * 8 + i] = b2 - b5;
        component[6 * 8 + i] = b1 - b6;
        component[7 * 8 + i] = b0 - b7;
    }

    for(int i = 0 ; i < 8; i++)
    {
        const float g0 = component[i*8 + 0] * s0;
        const float g1 = component[i*8 + 4] * s4;
        const float g2 = component[i*8 + 2] * s2;
        const float g3 = component[i*8 + 6] * s6;
        const float g4 = component[i*8 + 5] * s5;
        const float g5 = component[i*8 + 1] * s1;
        const float g6 = component[i*8 + 7] * s7;
        const float g7 = component[i*8 + 3] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        component[i * 8 + 0] = b0 + b7;
        component[i * 8 + 1] = b1 + b6;
        component[i * 8 + 2] = b2 + b5;
        component[i * 8 + 3] = b3 + b4;
        component[i * 8 + 4] = b3 - b4;
        component[i * 8 + 5] = b2 - b5;
        component[i * 8 + 6] = b1 - b6;
        component[i * 8 + 7] = b0 - b7;
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            result[i][j] = round(component[i * 8 + j] + 0.5f); // Round to nearest int
        }
    }
}

// dct2OnAllBlockChannels()
// Description: Applies DCT II to all channels of a DCTBlock
// Input: DCTBlock input - input block with Y, Cb, and Cr channels
//        DCTBlock output - output block with Y, Cb, and Cr channels
// Output: No return value, modifies the output DCTBlock
void JpegImage::dct2OnAllBlockChannels(DCTBlock *input, DCTBlock *output) {
    dct2OnBlock(input->Y, output->Y);
    dct2OnBlock(input->Cb, output->Cb);
    dct2OnBlock(input->Cr, output->Cr);
}

// idct2OnAllBlockChannels()
// Description: Applies inverse DCT II to all channels of a DCTBlock
// Input: DCTBlock input - input block with Y, Cb, and Cr channels
//        DCTBlock output - output block with Y, Cb, and Cr channels
// Output: No return value, modifies the output DCTBlock
void JpegImage::idct2OnAllBlockChannels(DCTBlock *input, DCTBlock *output) {
    idct2OnBlock(input->Y, output->Y);
    idct2OnBlock(input->Cb, output->Cb);
    idct2OnBlock(input->Cr, output->Cr);
}

DCTBlock* JpegImage::allocateDCTBlock() {
    DCTBlock* block = new DCTBlock;
    block->Y = allocateMatrix(8, 8);
    block->Cb = allocateMatrix(8, 8);
    block->Cr = allocateMatrix(8, 8);
    return block;
}

int** JpegImage::allocateMatrix(int rows, int cols) {
    int** matrix = new int*[rows];
    for(int i = 0; i < rows; ++i) {
        matrix[i] = new int[cols];
        fill_n(matrix[i], cols, 0); // Initialize all elements to 0
    }
    return matrix;
}

void JpegImage::deallocateMatrix(int** matrix, int rows) {
    for(int i = 0; i < rows; ++i) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

void JpegImage::deallocateDCTBlock(DCTBlock* block) {
    deallocateMatrix(block->Y, 8);
    deallocateMatrix(block->Cb, 8);
    deallocateMatrix(block->Cr, 8);
    delete block;
}


// generateDCTBlocks()
// Description: Splits the image into 8x8 blocks and applies DCT II to each block
// Input: No parameters, operates on the pixelsYCbCr 2D vector attribute
// Output: No return value, returns through the dctBlocks 2D vector attribute
void JpegImage::generateDCTBlocks() {
    if (!ycbcrLoaded) {
        cout << "No YCbCr data loaded - JpegImage::generateDCTBlocks" << endl;
        return;
    }

    // Resizing the dctBlocks 2D vector to represent 2D vector of 8x8 blocks of the pixels
    this->dctBlocks.resize(height / 8);
    for (int i = 0; i < height / 8; i++) {
        this->dctBlocks[i].resize(width / 8);
    }

    // Allocate memory for DCT Block
    DCTBlock* inputBlock = allocateDCTBlock();

    // Applying DCT II to each 8x8 block
    for (int i = 0; i < height; i += 8) {
        for (int j = 0; j < width; j += 8) {
            // Fill in the block with Y, Cb, and Cr values
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    inputBlock->Y[x][y] = pixelsYCbCr[i + x][j + y].y;
                    inputBlock->Cb[x][y] = pixelsYCbCr[i + x][j + y].cb;
                    inputBlock->Cr[x][y] = pixelsYCbCr[i + x][j + y].cr;
                }
            }

            // Allocate memory for output DCT block
            DCTBlock* outputBlock = allocateDCTBlock();

            // Apply DCT II to the block
            dct2OnAllBlockChannels(inputBlock, outputBlock);

            dctBlocks[i / 8][j / 8] = outputBlock;

        }
    }

    // Deallocate memory for input DCT block
    deallocateDCTBlock(inputBlock);

    dctBlocksGenerated = true;
}

// invertDCTBlocks()
// Description: Converts DCT blocks to pixel data (YCbCr)
// Input: No parameters, operates on the dctBlocks 2D vector attribute
// Output: No return value, modifies the pixelsYCbCr 2D vector attribute
void JpegImage::invertDCTBlocks() {
    if (!dctBlocksGenerated) {
        cout << "No DCT blocks generated - JpegImage::invertDCTBlocks()" << endl;
        return;
    }

    // Resizing the pixelsYCbCr 2D vector to represent 2D vector of YCbCr pixels
    this->pixelsYCbCr.resize(height);

    for (int i = 0; i < height; i++) {
        this->pixelsYCbCr[i].resize(width);
    }

    // DCT Input block pointer
    DCTBlock* inputBlock;

    // Allocate memory for output DCT block
    DCTBlock* outputBlock = allocateDCTBlock();

    // Applying inverse DCT II to each 8x8 block
    for (int i = 0; i < height; i += 8) {
        for (int j = 0; j < width; j += 8) {
            // Allocate memory for DCT Block
            inputBlock = dctBlocks[i / 8][j / 8];

            // Apply inverse DCT II to the block
            idct2OnAllBlockChannels(inputBlock, outputBlock);

            // Fill in the pixelsYCbCr 2D vector with the output block values
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    pixelsYCbCr[i + x][j + y].y = min(max(16, outputBlock->Y[x][y]), 255);
                    pixelsYCbCr[i + x][j + y].cb = min(max(16, outputBlock->Cb[x][y]), 255);
                    pixelsYCbCr[i + x][j + y].cr = min(max(16, outputBlock->Cr[x][y]), 255);
                }
            }
        }
    }

    // Deallocate memory for input DCT block
    deallocateDCTBlock(outputBlock);

    ycbcrLoaded = true;
}

// quantizeBlock()
// Description: Quantizes an 8x8 block using the quantization tables
// Input: int **block - 8x8 block to quantize
//        Channel channel - channel to use for quantization
// Output: No return value, modifies the input block
void JpegImage::quantizeBlock(int **block, Channel channel) {
    // Variable declaration
    int i, j;
    int tableVal;

    // Quantizing the block
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            tableVal = (channel == Y) ? quantTables.luminance[i][j] : quantTables.chrominance[i][j];
            block[i][j] = round(block[i][j] / tableVal);
        }
    }
}

// dequantizeBlock()
// Description: Dequantizes an 8x8 block using the quantization tables
// Input: int **block - 8x8 block to dequantize
//        Channel channel - channel to use for dequantization
// Output: No return value, modifies the input block
void JpegImage::dequantizeBlock(int **block, Channel channel) {
    // Variable declaration
    int i, j;
    int tableVal;

    // Quantizing the block
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            tableVal = (channel == Y) ? quantTables.luminance[i][j] : quantTables.chrominance[i][j];
            block[i][j] = round(block[i][j] * tableVal);
        }
    }
}

// generateQuantizedBlocks()
// Description: Quantizes all DCT blocks
// Input: No parameters, operates on the dctBlocks 2D vector attribute
// Output: No return value, modifies the quantizedBlocks 2D vector attribute
void JpegImage::generateQuantizedBlocks() {
    if (!dctBlocksGenerated) {
        cout << "No DCT blocks generated - JpegImage::generateQuantizedBlocks" << endl;
        return;
    }

    // Resizing the quantizedBlocks 2D vector to represent 2D vector of 8x8 blocks of the pixels
    this->quantizedBlocks.resize(height / 8);
    for (int i = 0; i < height / 8; i++) {
        this->quantizedBlocks[i].resize(width / 8);
    }

    // Quantizing all DCT blocks
    for (int i = 0; i < height / 8; i++) {
        for (int j = 0; j < width / 8; j++) {
            // Allocate memory for quantized DCT Block
            DCTBlock* quantizedBlock = allocateDCTBlock();

            // Copy the DCT block to the quantized block
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    quantizedBlock->Y[x][y] = dctBlocks[i][j]->Y[x][y];
                    quantizedBlock->Cb[x][y] = dctBlocks[i][j]->Cb[x][y];
                    quantizedBlock->Cr[x][y] = dctBlocks[i][j]->Cr[x][y];
                }
            }

            // Quantize the block
            quantizeBlockChannels(quantizedBlock);

            quantizedBlocks[i][j] = quantizedBlock;
        }
    }

    quantizedBlocksGenerated = true;
}

// invertQuantizedBlocks()
// Description: Dequantizes all quantized DCT blocks
// Input: No parameters, operates on the quantizedBlocks 2D vector attribute
// Output: No return value, modifies the dctBlocks 2D vector attribute
void JpegImage::dequantizeBlocks() {
    if (!quantizedBlocksGenerated) {
        cout << "No quantized DCT blocks generated - JpegImage::dequantizeBlocks()" << endl;
        return;
    }

    // Resizing the dctBlocks 2D vector to represent 2D vector of 8x8 blocks of the pixels
    this->dctBlocks.resize(height / 8);
    for (int i = 0; i < height / 8; i++) {
        this->dctBlocks[i].resize(width / 8);
    }

    // Dequantizing all quantized DCT blocks
    for (int i = 0; i < height / 8; i++) {
        for (int j = 0; j < width / 8; j++) {
            DCTBlock* dctBlock;
            // If the dct blocks were previously allocated, use the same memory
            if (dctBlocksGenerated) {
                dctBlock = dctBlocks[i][j];
            } else {
                // Allocate memory for DCT Block
                dctBlock = allocateDCTBlock();
            }


            // Copy the quantized block to the DCT block
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    dctBlock->Y[x][y] = quantizedBlocks[i][j]->Y[x][y];
                    dctBlock->Cb[x][y] = quantizedBlocks[i][j]->Cb[x][y];
                    dctBlock->Cr[x][y] = quantizedBlocks[i][j]->Cr[x][y];
                }
            }

            // Dequantize the block
            dequantizeBlockChannels(dctBlock);

            dctBlocks[i][j] = dctBlock;
        }
    }

    dctBlocksGenerated = true;
}

// zigzag()
// Description: Converts DCT quantized blocks to a zigzag sequence
// Input: No parameters, operates on the dctBlocks 2D vector attribute
void JpegImage::zigzag(vector<int> &sequence) {
    // Checking if quantized blocks are generated
    if (!quantizedBlocksGenerated) {
        cout << "No quantized DCT blocks generated - JpegImage::zigzag" << endl;
        return;
    }

    // Looping over all DCT blocks
    for (int i = 0; i < height / 8; i++) {
        for (int j = 0; j < width / 8; j++) {
            // Convert DCT block to zigzag sequence
            zigzagDCTBlock(quantizedBlocks[i][j], sequence);
        }
    }
}

// zigzagBlock()
// Description: Converts an 8x8 block to a zigzag sequence
// Input: int **block - 8x8 block to convert
//        vector<int> &sequence - reference to the sequence to store the zigzag values
// Output: No return value, modifies the sequence
void JpegImage::zigzagBlock(int **block, vector<int> &sequence) {
    // Variable declaration
    int x, y;

    // Looping over zigzag order
    for (int i = 0; i < 64; i++) {
        x = zigzagOrder[i][0];
        y = zigzagOrder[i][1];
        sequence.push_back(block[x][y]);
    }
}

// zigzagDCTBlock()
// Description: Converts a DCT block to a zigzag sequence
// Input: DCTBlock *block - block to convert
//        vector<int> &sequence - reference to the sequence to store the zigzag values
// Output: No return value, modifies the sequence
void JpegImage::zigzagDCTBlock(DCTBlock *block, vector<int> &sequence) {
    zigzagBlock(block->Y, sequence);
    zigzagBlock(block->Cb, sequence);
    zigzagBlock(block->Cr, sequence);
}

// inverseZigzag()
// Description: Converts a zigzag sequence to DCT quantized blocks
// Input: vector<int> &sequence - sequence to convert
// Output: No return value, modifies the quantizedBlocks 2D vector attribute
void JpegImage::inverseZigzag(vector<int> &sequence) {
    // If quantized blocks not yet allocated, allocate memory
    if (!quantizedBlocksGenerated) {
        quantizedBlocks.resize(height / 8);
        for (int i = 0; i < height / 8; i++) {
            quantizedBlocks[i].resize(width / 8);
        }
    }

    int start_index = 0;
    for (int i = 0; i < height / 8; i++) {
        for (int j = 0; j < width / 8; j++) {
            DCTBlock* block = allocateDCTBlock();
            inverseZigzagDCTBlock(sequence, block, start_index);
            start_index += 64 * 3;
            quantizedBlocks[i][j] = block;
        }
    }

    quantizedBlocksGenerated = true;
}

// inverseZigzagBlock()
// Description: Converts a zigzag sequence to an 8x8 block
// Input: vector<int> &sequence - sequence to convert
//        int **block - 8x8 block to store the values
//        int start_index - starting index of the sequence
// Output: No return value, modifies the block
void JpegImage::inverseZigzagBlock(vector<int> &sequence, int **block, int start_index) {
    bool outOfBounds = false;
    for (int i = 0; i < 64 && !outOfBounds; i++) {
        int x = zigzagOrder[i][0];
        int y = zigzagOrder[i][1];

        // Check if the sequence is out of bounds
        if (start_index + i >= sequence.size()) {
          //  cout << "Index out of bounds" << endl;
            outOfBounds = true;
        } else {
            int sequence_item = sequence[start_index + i];
            block[x][y] = sequence_item;
        }
    }
}

// inverseZigzagDCTBlock()
// Description: Converts a zigzag sequence to a DCT block
// Input: vector<int> &sequence - sequence to convert
//        DCTBlock *block - block to store the values
//        int start_index - starting index of the sequence
// Output: No return value, modifies the block
void JpegImage::inverseZigzagDCTBlock(vector<int> &sequence, DCTBlock *block, int start_index) {
    inverseZigzagBlock(sequence, block->Y, start_index);
    inverseZigzagBlock(sequence, block->Cb, start_index + 64);
    inverseZigzagBlock(sequence, block->Cr, start_index + 128);
}

// quantizeBlockChannels()
// Description: Quantizes the Y, Cb, and Cr channels of a DCT block
// Input: DCTBlock *block - block to quantize
// Output: No return value, modifies the input block
void JpegImage::quantizeBlockChannels(DCTBlock *block) {
    quantizeBlock(block->Y, Y);
    quantizeBlock(block->Cb, Cb);
    quantizeBlock(block->Cr, Cr);
}

// dequantizeBlockChannels()
// Description: Dequantizes the Y, Cb, and Cr channels of a DCT block
// Input: DCTBlock *block - block to dequantize
// Output: No return value, modifies the input block
void JpegImage::dequantizeBlockChannels(DCTBlock *block) {
    dequantizeBlock(block->Y, Y);
    dequantizeBlock(block->Cb, Cb);
    dequantizeBlock(block->Cr, Cr);
}

// setQuantizationTables()
// Description: Sets the quantization tables for luminance and chrominance
// Input: int quality - quality of the image (0-100)
// Output: No return value, modifies the quantTables attribute
void JpegImage::setQuantizationTables(int quality) {
    // Variable declaration
    double s;

    // Defining standard quantization tables
    int lum[8][8] = {
            {16, 11, 10, 16, 24, 40, 51, 61},
            {12, 12, 14, 19, 26, 58, 60, 55},
            {14, 13, 16, 24, 40, 57, 69, 56},
            {14, 17, 22, 29, 51, 87, 80, 62},
            {18, 22, 37, 56, 68, 109, 103, 77},
            {24, 35, 55, 64, 81, 104, 113, 92},
            {49, 64, 78, 87, 103, 121, 120, 101},
            {72, 92, 95, 98, 112, 100, 103, 99}
    };

    int chrom[8][8] = {
            {17, 18, 24, 47, 99, 99, 99, 99},
            {18, 21, 26, 66, 99, 99, 99, 99},
            {24, 26, 56, 99, 99, 99, 99, 99},
            {47, 66, 99, 99, 99, 99, 99, 99},
            {99, 99, 99, 99, 99, 99, 99, 99},
            {99, 99, 99, 99, 99, 99, 99, 99},
            {99, 99, 99, 99, 99, 99, 99, 99},
            {99, 99, 99, 99, 99, 99, 99, 99}
    };

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            quantTables.luminance[i][j] = lum[i][j];
            quantTables.chrominance[i][j] = chrom[i][j];
        }
    }

    // S = 5000 / Q for Q < 50 and S = 200 - 2 * Q for Q >= 50
    s = (quality < 50) ? double(5000 / double(quality)) : double(200 - 2 * double(quality));
    if (quality == 100) s = 1;

    // Adjusting luminance and chrominance quantization tables
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            quantTables.luminance[i][j] = max(1, int((s * quantTables.luminance[i][j] + 50) / 100));
            quantTables.chrominance[i][j] = max(1, int((s * quantTables.chrominance[i][j] + 50) / 100));
        }
    }

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
}

// RLE functions
// encodeRLE()
// Description: Encodes a sequence of integers using run-length encoding
// Input: vector<int> &input - sequence to encode
//        vector<int> &output - encoded sequence
// Output: No return value, modifies the output
void JpegImage::encodeRLE(vector<int> &input, vector<int> &output) {
    for (int i = 0; i < input.size(); i++) {
        int count = 1;

        while (i + 1 < input.size() && input[i] == input[i + 1]) {
            count++;
            i++;
        }

        output.push_back(input[i]);
        output.push_back(count);
    }
}

// decodeRLE()
// Description: Decodes a sequence of integers using run-length encoding
// Input: vector<int> &input - encoded sequence
//        vector<int> &output - decoded sequence
// Output: No return value, modifies the output
void JpegImage::decodeRLE(vector<int> &input, vector<int> &output, int size) {
    // Checking if the input is empty or has an odd number of elements
    if (size % 2 != 0 || input.size() == 0) {
        cout << "Invalid input - JpegImage::decodeRLE - " << size << endl;
        return;
    }

    for (int i = 0; i < input.size(); i++) {
        int value = input[i];
        int count = input[++i];

        for (int j = 0; j < count; j++) {
            output.push_back(value);
        }
    }
}

// Huffman encoding
// calculateFrequencies()
// Description: Calculates the frequencies of each value in a sequence
// Input: const vector<int> &data - sequence to calculate frequencies for
// Output: map<int, int> - map of value to frequency
map<int, int> JpegImage::calculateFrequencies(const vector<int>& data) {
    map<int, int> frequencies;

    // Counting frequencies
    for (int i : data) {
        frequencies[i]++;
    }

    return frequencies;
}

// buildHuffmanTree()
// Description: Builds a Huffman tree from a map of frequencies
// Input: const map<int, int>& frequencies - map of value to frequency
// Output: HuffmanNode* - root of the Huffman tree
HuffmanNode* JpegImage::buildHuffmanTree(const map<int, int>& frequencies) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, Compare> pq;

    for (const auto& pair : frequencies) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        HuffmanNode* left = pq.top();
        pq.pop();
        HuffmanNode* right = pq.top();
        pq.pop();

        HuffmanNode* parent = new HuffmanNode(left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;

        pq.push(parent);
    }

    return pq.top();
}

// generateHuffmanCodes()
// Description: Generates Huffman codes for each value in a Huffman tree
// Input: HuffmanNode* root - root of the Huffman tree
// Output: map<int, string> - map of value to Huffman code
map<int, string> JpegImage::generateHuffmanCodes(const HuffmanNode* root) {
    map<int, string> huffmanCodes;
    generateHuffmanCodesRecursive(root, "", huffmanCodes);

    return huffmanCodes;
}

// generateHuffmanCodesRecursive()
// Description: Generates Huffman codes for each value in a Huffman tree recursively
// Input: const HuffmanNode* node - current node in the tree
//        const string& code - current Huffman code
//        map<int, string>& huffmanCodes - map of value to Huffman code
// Output: No return value, modifies the huffmanCodes map
void JpegImage::generateHuffmanCodesRecursive(const HuffmanNode* node, const string &code, map<int, string>& huffmanCodes) {
    if (node == nullptr) return;

    if (node->left == nullptr && node->right == nullptr) {
        huffmanCodes.insert(pair<int, string>(node->data, code));
    }

    generateHuffmanCodesRecursive(node->left, code + '1', huffmanCodes);
    generateHuffmanCodesRecursive(node->right, code + '0', huffmanCodes);
}

// encodeData()
// Description: Encodes a sequence of integers using Huffman codes
// Input: const vector<int>& data - sequence to encode
//        const map<int, string>& huffmanCodes - map of value to Huffman code
// Output: string - encoded data
string JpegImage::encodeData(const vector<int>& data, const map<int, string>& huffmanCodes) {
    string encodedData;

    for (int i : data) {
        encodedData += huffmanCodes.find(i)->second;
    }

    return encodedData;
}

// decodeData()
// Description: Decodes a sequence of integers using Huffman codes
// Input: const string& encodedData - encoded data
//        const HuffmanNode* root - root of the Huffman tree
// Output: vector<int> - decoded data
vector<int> JpegImage::decodeData(const string& encodedData, HuffmanNode* root) {
    // Variable declaration
    vector<int> decodedData;
    HuffmanNode* current = root;
    string currentCode;

    for (int i = 0; i < encodedData.size(); i++) {
        if (encodedData[i] == '1') {
            currentCode.append("1");
            current = current->left;
        } else {
            currentCode.append("0");
            current = current->right;
        }

        if (current->left == nullptr && current->right == nullptr) {
            decodedData.push_back(current->data);
            current = root;
        }
    }

    return decodedData;
}

// writeEncodedDataToFile()
// Description: Writes encoded data to a file
// Input: const string& encodedData - encoded data
//        const string& filePath - path to the output file
// Output: No return value, writes the encoded data to the file
void JpegImage::writeEncodedDataToFile(const string& encodedData, const string& filePath) {
    ofstream outputFile(filePath, ios::binary);
    if (!outputFile.is_open()) {
        cerr << "Failed to open file for writing.\n";
        return;
    }

    vector<unsigned char> buffer;
    for (size_t i = 0; i < encodedData.size(); i += 8) {
        // Take up to 8 bits from the encoded data
        string byteString = encodedData.substr(i, 8);

        // Make sure it's 8 bits long, padding with 0s if necessary
        while (byteString.length() < 8) {
            byteString += '0';
        }

        // Convert the 8-bit string to an actual byte
        bitset<8> bits(byteString);
        buffer.push_back(static_cast<unsigned char>(bits.to_ulong()));
    }

    // Write the buffer to file
    outputFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    outputFile.close();
}

// appendEncodedDataToFile()
// Description: Appends encoded data to a file
// Input: const string& encodedData - encoded data
//        const string& filePath - path to the output file
// Output: No return value, appends the encoded data to the file
void JpegImage::appendEncodedDataToFile(const string& encodedData, const string& filePath) {
    ofstream outputFile(filePath, ios::binary | ios::app);
    if (!outputFile.is_open()) {
        cerr << "Failed to open file for writing.\n";
        return;
    }

    vector<unsigned char> buffer;
    for (size_t i = 0; i < encodedData.size(); i += 8) {
        // Take up to 8 bits from the encoded data
        string byteString = encodedData.substr(i, 8);

        // Make sure it's 8 bits long, padding with 0s if necessary
        while (byteString.length() < 8) {
            byteString += '0';
        }

        // Convert the 8-bit string to an actual byte

        try {
            bitset<8> bits(byteString);
            buffer.push_back(static_cast<unsigned char>(bits.to_ulong()));
        } catch (exception& e) {
//            cout << "Exception - not valid byte string: " << byteString << endl;
        }
    }

    // Write the buffer to file
    outputFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    outputFile.close();
}

// readEncodedDataFromFile()
// Description: Reads encoded data from a file
// Input: const string& filePath - path to the input file
// Output: string - encoded data read from the file
string JpegImage::readEncodedDataFromFile(const string& filePath, int starting_byte, int num_bytes) {
    ifstream inputFile(filePath, ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Failed to open file for reading.\n";
        return "";
    }

    // Get the size of the file
    inputFile.seekg(starting_byte, ios::beg);

    // Read the file into a buffer
    vector<unsigned char> buffer(num_bytes);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), num_bytes);

    // Convert the buffer to a string
    string encodedData;
    for (unsigned char byte : buffer) {
        bitset<8> bits(byte);
        encodedData.append(bits.to_string());
    }

    return encodedData;
}

// Steganography-related functions
void JpegImage::encodeLSBOnQuantizedBlocks(const string& message) {
    int bitCount = 0;
    int terminatorCount = 8;
    int bitLength = message.length() * sizeof(unsigned char) * 8;
    enum encoding_status status = encoding_status::MESSAGE;
    bool bit;
    unsigned int onMask = 0x01;
    unsigned int offMask = 0xFFFFFFFE;
    DCTBlock* block;
    bool done = false;

    // Checking if image is large enough to hold the message
    if (bitCount + 8 > width * height * 3) {
        cout << "Message is too large to encode in this image." << endl;
        successfullyEncoded = false;
        return;
    }

    // Looping over all quantized DCT blocks
    for (int i = 0; i < height / 8 && !done; i++) {
        for (int j = 0; j < width / 8 && !done; j++) {
            block = quantizedBlocks[i][j];

            // Looping over all channels of the block
            for (int k = 0; k < 3; k++) {
                int** channel = (k == 0) ? block->Y : (k == 1) ? block->Cb : block->Cr;

                // Looping over all elements of the channel
                for (int x = 0; x < 8 && !done; x++) {
                    for (int y = 0; y < 8 && !done; y++) {
                        if ((x != 0 || y != 0) && (channel[x][y] != 0) && (channel[x][y] != 1)) {
                            // Check if the message has been fully encoded
                            if (bitCount >= bitLength) {
                                status = encoding_status::TERMINATOR;
                            }

                            // Get the next bit of the message
                            switch (status) {
                                case encoding_status::MESSAGE:
                                    bit = getBit(message[bitCount / 8], 7 - bitCount % 8);
                                    break;
                                case encoding_status::TERMINATOR:
                                    bit = false;
                                    terminatorCount--;
                                    if (terminatorCount == 0) {
                                        done = true;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            // Encode the bit in the least significant bit of the channel element
                            if (bit) {
                                channel[x][y] |= onMask;
                            } else {
                                channel[x][y] &= offMask;
                            }

                            bitCount++;
                        }
                    }
                }
            }
        }
    }

    if (terminatorCount > 0) {
        cout << "Message too large to encode in this image." << endl;
        successfullyEncoded = false;
    }

}

// decodeLSBOnQuantizedBlocks()
// Description: Decodes a message encoded using LSB on quantized DCT blocks
// Input: No parameters, operates on the quantizedBlocks 2D vector attribute
// Output: string - decoded message
string JpegImage::decodeLSBOnQuantizedBlocks() {
    // Check if quantized blocks are generated
    if (!quantizedBlocksGenerated) {
        cout << "No quantized DCT blocks generated - JpegImage::decodeLSBOnQuantizedBlocks" << endl;
        return "";
    }

    // Variable declaration
    string message;
    unsigned char character = 0;
    DCTBlock *block;
    unsigned int bitCount = 0;
    bool bit;
    bool done = false;

    // Looping over quantized blocks
    for (int i = 0; i < height / 8 && !done; i++) {
        for (int j = 0; j < width / 8 && !done; j++) {
            block = quantizedBlocks[i][j];

            for (int k = 0; k < 3; k++) {
                int **channel = (k == 0) ? block->Y : (k == 1) ? block->Cb : block->Cr;

                for (int x = 0; x < 8 && !done; x++) {
                    for (int y = 0; y < 8 && !done; y++) {
                        if ((x != 0 || y != 0) && (channel[x][y] != 0) && (channel[x][y] != 1)) {
                            // Getting LSB of the DCT coefficient
                            bit = channel[x][y] & 0x01;

                            // Shifting character bits left and adding current bit
                            character = (character << 1) | bit;

                            bitCount++;

                            // Checking end of character
                            if (bitCount % 8 == 0) {
                                // Checking for null terminator
                                if (character == 0) {
                                    done = true;
                                } else {
                                    message += (char)character;
                                    character = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return message;
}