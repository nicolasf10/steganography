//
// Created by Nicolas Fuchs on 21/04/2024.
//

#include "NeuralNetwork.h"
#include "crow_all.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "StegoLib.h"
#include "JpegCustom.h"

using namespace std;
using namespace crow;

#define INPUT_PNG_SIZE (32 * 32 * 3)

typedef struct rgb_channels {
    int r;
    int g;
    int b;
} rgb_channels;

// Function prototypes
string steganalysis_png(string filename);
string steganalysis_jpeg(string filename);
rgb_channels getRGBFromPercentage(double percentage);

string read_file(const string& path) {
    ifstream in(path, ios::binary | ios::ate);
    if (!in) return "";

    streamsize size = in.tellg();
    in.seekg(0, ios::beg);
    string buffer(size, ' ');
    if (in.read(&buffer[0], size)) {
        return buffer;
    }
    return "";
}

void save_file(const string& filename, const string& content) {
    ofstream out("./" + filename, ios::binary);
    if (out) {
        out.write(content.data(), content.size());
        out.close();
    }
}

void add_cors_headers(response& res) {
    res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, X-Requested-With");
    res.add_header("Access-Control-Allow-Credentials", "true");
}

int main()
{
    SimpleApp app;

    // PNG steganography routes
    // /steganography/png/encode route which receives and also returns a png file
    CROW_ROUTE(app, "/steganography/png/encode").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).png
        string filename = "temp_" + to_string(rand()) + ".png";

        save_file(filename, file_content);

        // Get input message which was passed in the body
        size_t pos_mesage = body.find("name=\"message\"");
        string message;
        if (pos_mesage != string::npos) {
            size_t start = body.find("\r\n\r\n", pos_mesage) + 4;
            size_t end = body.find("\r\n--", start);
            message = body.substr(start, end - start);
        }

        // Encode using the Image class
        Image *image = new Image(filename);

        image->generateBitmap();
        image->encodeLSB("stego_" + filename, message);

        delete image;

        // Read the file back to return it
        ifstream file("./stego_" + filename, ios::binary | ios::ate);
        if (file) {
            streamsize size = file.tellg();
            file.seekg(0, ios::beg);
            vector<char> buffer(size);
            if (file.read(buffer.data(), size)) {
                res.set_header("Content-Type", "image/png");
                res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
                res.write(string(buffer.begin(), buffer.end()));
                res.end();
            } else {
                res.code = 500;
                res.write("Failed to read the file");
                res.end();
            }
        } else {
            res.code = 500;
            res.write("Failed to process the file");
            res.end();
        }
    });

    CROW_ROUTE(app, "/steganography/png/decode").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).png
        string filename = "temp_decode_" + to_string(rand()) + ".png";

        save_file(filename, file_content);

        // Encode using the Image class
        Image *image = new Image(filename);

        image->generateBitmap();

        string decoded_message = image->decodeLSB();

        delete image;

        res.code = 200;
        res.set_header("Content-Type", "text/plain");
        res.write(decoded_message);
        res.end();
    });

    // JPEG steganography routes
    CROW_ROUTE(app, "/steganography/jpeg/encode").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).png
        string filename = "temp_" + to_string(rand()) + ".png";

        save_file(filename, file_content);

        // Get input message which was passed in the body
        size_t pos_mesage = body.find("name=\"message\"");
        string message;
        if (pos_mesage != string::npos) {
            size_t start = body.find("\r\n\r\n", pos_mesage) + 4;
            size_t end = body.find("\r\n--", start);
            message = body.substr(start, end - start);
        }

        // Get jpeg quality which was passed in the body
        size_t pos_quality = body.find("name=\"quality\"");
        string quality = "50";
        if (pos_quality != string::npos) {
            size_t start = body.find("\r\n\r\n", pos_quality) + 4;
            size_t end = body.find("\r\n--", start);
            quality = body.substr(start, end - start);
        }

        // Encode using the Jpeg class
        JpegImage *image = new JpegImage();
        image->loadPng(filename);
        // set quality but parse to int
        int num_quality = stoi(quality);

        image->setQuality(num_quality);

        string new_filename = "stego_" + filename.substr(0, filename.size() - 4) + ".dat";
        image->encodeJpeg(new_filename, message);

        delete image;

        // Read the file back to return it
        ifstream file("./" + new_filename, ios::binary | ios::ate);
        if (file) {
            streamsize size = file.tellg();
            file.seekg(0, ios::beg);
            vector<char> buffer(size);
            if (file.read(buffer.data(), size)) {
                // set header for .dat file
                res.set_header("Content-Type", "application/octet-stream");
                res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
                res.write(string(buffer.begin(), buffer.end()));
                res.end();
            } else {
                res.code = 500;
                res.write("Failed to read the file");
                res.end();
            }
        } else {
            res.code = 500;
            res.write("Failed to process the file");
            res.end();
        }
    });

    CROW_ROUTE(app, "/steganography/jpeg/get_png").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).png
        string filename = "temp_" + to_string(rand()) + ".png";

        save_file(filename, file_content);

        // Encode using the Jpeg class
        JpegImage *image = new JpegImage();

        image->decodeJpeg(filename);

        string new_filename = "png_" + filename.substr(0, filename.size() - 4) + ".png";
        image->savePng(new_filename);

        delete image;


        // Read the file back to return it
        ifstream file("./" + new_filename, ios::binary | ios::ate);
        if (file) {
            streamsize size = file.tellg();
            file.seekg(0, ios::beg);
            vector<char> buffer(size);
            if (file.read(buffer.data(), size)) {
                res.set_header("Content-Type", "image/png");
                res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
                res.write(string(buffer.begin(), buffer.end()));
                res.end();
            } else {
                res.code = 500;
                res.write("Failed to read the file");
                res.end();
            }
        } else {
            res.code = 500;
            res.write("Failed to process the file");
            res.end();
        }
    });

    CROW_ROUTE(app, "/steganography/jpeg/decode").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).dat
        string filename = "temp_decode_" + to_string(rand()) + ".dat";

        save_file(filename, file_content);

        // Encode using the Image class
        JpegImage *image = new JpegImage();
        image->decodeJpeg(filename);

        string decoded_message = image->decodeLSBOnQuantizedBlocks();

        res.code = 200;
        res.set_header("Content-Type", "text/plain");
        res.write(decoded_message);
        res.end();
    });

    // crow route /steganalysis/png which returns json {isStego: double}
    CROW_ROUTE(app, "/steganalysis/png").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).png
        string filename = "temp_" + to_string(rand()) + ".png";

        save_file(filename, file_content);

        string heatmap_filename;
        string analysis = steganalysis_png(filename);

        // Delete the file
        remove(filename.c_str());

        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(analysis);
        res.end();
    });

    CROW_ROUTE(app, "/steganalysis/jpeg").methods(HTTPMethod::Post)([](const request& req, response& res){
        add_cors_headers(res);

        string body = req.body;
        // Naive extraction assuming a single file in the request
        size_t pos = body.find("filename=\"");
        size_t start = body.find("\r\n\r\n", pos) + 4;
        size_t end = body.find("\r\n--", start);
        string file_content = body.substr(start, end - start);

        // set filename to temp_(randomNumber).png
        string filename = "temp_" + to_string(rand()) + ".data";

        save_file(filename, file_content);

        string heatmap_filename;
        string analysis = steganalysis_jpeg(filename);

        // Delete the file
        remove(filename.c_str());

        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(analysis);
        res.end();
    });


    // Route to retrieve images stored in the server
    CROW_ROUTE(app, "/images/<string>") ([](const crow::request& req, const std::string& filename){
        std::string path = "./" + filename;
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open()) {
            return crow::response(404);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        crow::response res(buffer.str());
        res.add_header("Content-Type", "image/jpeg");  // Adjust according to actual image type
        return res;
    });

    app.port(18080).multithreaded().run();
}

// Function to perform steganalysis on a PNG image
// Returns string in json format for server to return
string steganalysis_png(string filename) {
    Image *image = new Image(filename);
    image->generateBitmap();
    double isStegoThreshold = 0.9;

    vector<VectorXd> data;

    // Split image into 32x32 blocks
    for (int y = 0; y <= image->height - 32; y += 32) {
        for (int x = 0; x <= image->width - 32; x += 32) {
            // Input vector
            VectorXd input = VectorXd::Zero(INPUT_PNG_SIZE);

            int i = 0;
            for (int j = y; j < y + 32; j++) {
                for (int k = x; k < x + 32; k++) {
                    color pixel = image->pixels[j][k];
                    input(i++) = pixel.r & 0x01;
                    input(i++) = pixel.g & 0x01;
                    input(i++) = pixel.b & 0x01;
                }
            }

            data.push_back(input);
        }
    }

    cout << "Input: " << data.front().transpose() << endl;

    // Evaluate each block using the neural network
    NeuralNetwork nn("/Users/nicolasfuchs/CLionProjects/stegoProjectMac/stegoNet.dat");
    vector<VectorXd> output;

    // Output = (x, y) ; x = probability of being a cover block, y = probability of being a stego block

    output.reserve(data.size());
    for (auto &d : data) {
//        cout << "Input: " << d.transpose() << endl;
//        cout << "Output: " << nn.feedforward(d).transpose() << endl;

        output.push_back(nn.feedforward(d));
    }

    // print the first output
    cout << "Output: " << output.front().transpose() << endl;

    // Calculating:

    // - Probability of the image containing at least one stego block
    //      P = Probability of no stego blocks
    //      P' = Probability of at least one stego block
    //      P = p1 * p2 * ... * pn
    //      P' = 1 - P
    // - Block with the highest probability of being stego
    // - Expected bytes hidden in the image

    double P = 1;
    double maxStegoProbability = 0;
    double expectedBytes = 0;

    for (auto &o : output) {
        P *= o(0); // Multiplying P by the probability of NOT being a stego block
        maxStegoProbability = max(maxStegoProbability, o(1));
        if (o(1) > 0.9)
            expectedBytes += o(1) * 32 * 32 * 3 / 8;
    }

    double P_prime = 1 - P;

    // Generating heatmap of network's confidence of stego prediciton per block
    // Set random seed
    srand(time(NULL));

    string heatmap_filename = "heatmap_" + to_string(rand()) + ".png";
    string heatmap_filepath =  "./" + heatmap_filename;

    // Print output from neural network to console
    for (int i = 0; i < output.size(); i++) {
//        cout << "Output " << i << ": " << output[i].transpose() << endl;
    }

    // Create heatmap (new CImg)
    CImg<unsigned char> heatmap(image->width + (32 * 5), image->height - (image->height % 32) + (32 * 2), 1, 3, 255);

    rgb_channels channels;

    // Draw heatmap range
    for (int i = 0; i < image->height - (image->height % 32); i++) {
        for (int j = image->width + 16; j < image->width + 32 * 2; j++) {
            channels = getRGBFromPercentage(1 - (float(i) / image->height));
            heatmap(j + 64, i + 32, 0, 0) = channels.r;
            heatmap(j + 64, i + 32, 0, 1) = channels.g;
            heatmap(j + 64, i + 32, 0, 2) = channels.b;
        }
    }

    // Write "100% stego" above the heatmap and "0% stego" below the heatmap
    // Coordinates for the text annotations
    int textX = image->width + 64; // Position text to the right of the heatmap
    int topTextY = 10; // Top of the image
    int bottomTextY = image->height - (image->height % 32) + 32 + 7; // Bottom of the image, adjust as needed

    // Adding text for "100% stego"
    static const unsigned char black[] { 0,0,0 };
    heatmap.draw_text(textX - 5, topTextY, "100%% stego", black, 0, 1, 18);

    // Adding text for "0% stego"
    heatmap.draw_text(textX, bottomTextY, "0%% stego", black, 0, 1, 18);


    // Iterate over each block and color the block according to the probability of being stego
    int block = 0;
    for (int y = 0; y <= image->height - 32; y += 32) {
        for (int x = 0; x <= image->width - 32; x += 32) {
            rgb_channels channels = getRGBFromPercentage(output[block++](1));

            for (int j = y; j < y + 32; j++) {
                for (int k = x; k < x + 32; k++) {
                    heatmap(k + 32, j + 32, 0, 0) = channels.r;
                    heatmap(k + 32, j + 32, 0, 1) = channels.g;
                    heatmap(k + 32, j + 32, 0, 2) = channels.b;
                }
            }
        }
    }

    // Save image to heatmap_filepath
    heatmap.save(heatmap_filepath.c_str());

    // Send response
    return "{\"isStego\": " + to_string(maxStegoProbability > isStegoThreshold) + ", \"maxStegoProbability\": " + to_string(maxStegoProbability) + ", \"expectedBytes\": " + to_string(expectedBytes) + ", \"heatmapUrl\": \"" + heatmap_filename + "\"}";
}

string steganalysis_jpeg(string filename) {
    double isStegoThreshold = 0.9;

    vector<VectorXd> data;

    auto *image = new JpegImage();
    image->decodeJpeg(filename);

    // Split image into blocks of 4 * 4 DCT coefficients
    for (int y = 0; y <= image->dctBlocks.size() - 4; y += 4) {
        for (int x = 0; x <= image->dctBlocks[y].size() - 4; x += 4) {
            // Input vector
            VectorXd input = VectorXd::Zero(INPUT_JPEG_SIZE);
            VectorXd processedInput = VectorXd::Zero(PROCESSED_JPEG_INPUT);

            int i = 0;
            for (int j = y; j < y + 4; j++) {
                for (int k = x; k < x + 4; k++) {
                    DCTBlock *block = image->dctBlocks[j][k];
                    for (int l = 0; l < 8; l++) {
                        for (int n = 0; n < 8; n++) {
                            input(i++) = block->Y[l][n];
                            input(i++) = block->Cb[l][n];
                            input(i++) = block->Cr[l][n];
                        }
                    }
                }
            }

            // Process the input vector, creating pairwise sequence histogram
            int count = 0;
            int lastLSB = -1;

            for (int l = 0; l < input.size(); l++) {
                if (input(l) != 0) {
                    int value = (int) input(l) & 1;
                    if (value == lastLSB && count < 8) {
                        count++;
                    } else {
                        if (lastLSB != -1) {
                            processedInput((min(count, 8) - 1) + (lastLSB * 8))++;
                        }
                        lastLSB = (int) input(l);
                        lastLSB &= 0x01;
                        count = 1;
                    }
                }
            }

            if (lastLSB != -1) {
                processedInput((min(count, 8) - 1) + (lastLSB * 8))++;
            }

            cout << "Processed input: " << processedInput.transpose() << endl;

            data.push_back(processedInput);
        }
    }

    // Print the first input
    for (int input_count = 0; input_count < 1; input_count++) {
        cout << "Input: " << data[input_count].transpose() << endl;
    }

    // Evaluate each block using the neural network
    NeuralNetwork nn("/Users/nicolasfuchs/CLionProjects/stegoProjectMac/stegoJpegNet2.dat");
    vector<VectorXd> output;

    // Output = (x, y) ; x = probability of being a cover block, y = probability of being a stego block

    output.reserve(data.size());
    for (auto &d : data) {
        output.push_back(nn.feedforward(d));
    }

    // print the first output
    for (int count_example = 0; count_example < 1; count_example++) {
        cout << "Output: " << output[count_example].transpose() << endl;
    }

    // Calculating:

    // - Probability of the image containing at least one stego block
    //      P = Probability of no stego blocks
    //      P' = Probability of at least one stego block
    //      P = p1 * p2 * ... * pn
    //      P' = 1 - P
    // - Block with the highest probability of being stego
    // - Expected bytes hidden in the image

    double P = 1;
    double maxStegoProbability = 0;
    double expectedBytes = 0;

    for (auto &o : output) {
        P *= o(0); // Multiplying P by the probability of NOT being a stego block
        maxStegoProbability = max(maxStegoProbability, o(1));
        if (o(1) > 0.9)
            expectedBytes += o(1) * 32 * 32 * 3 / 8;
    }

    double P_prime = 1 - P;

    // Generating heatmap of network's confidence of stego prediciton per block
    // Set random seed
    srand(time(NULL));

    string heatmap_filename = "heatmap_" + to_string(rand()) + ".png";
    string heatmap_filepath =  "./" + heatmap_filename;

    // Print output from neural network to console
    for (int i = 0; i < output.size(); i++) {
//        cout << "Output " << i << ": " << output[i].transpose() << endl;
    }

    // Create heatmap (new CImg)
    CImg<unsigned char> heatmap(image->width + (32 * 5), image->height - (image->height % 32) + (32 * 2), 1, 3, 255);

    rgb_channels channels;

    // Draw heatmap range
    for (int i = 0; i < image->height - (image->height % 32); i++) {
        for (int j = image->width + 16; j < image->width + 32 * 2; j++) {
            channels = getRGBFromPercentage(1 - (float(i) / image->height));
            heatmap(j + 64, i + 32, 0, 0) = channels.r;
            heatmap(j + 64, i + 32, 0, 1) = channels.g;
            heatmap(j + 64, i + 32, 0, 2) = channels.b;
        }
    }

    // Write "100% stego" above the heatmap and "0% stego" below the heatmap
    // Coordinates for the text annotations
    int textX = image->width + 64; // Position text to the right of the heatmap
    int topTextY = 10; // Top of the image
    int bottomTextY = image->height - (image->height % 32) + 32 + 7; // Bottom of the image, adjust as needed

    // Adding text for "100% stego"
    static const unsigned char black[] { 0,0,0 };
    heatmap.draw_text(textX - 5, topTextY, "100%% stego", black, 0, 1, 18);

    // Adding text for "0% stego"
    heatmap.draw_text(textX, bottomTextY, "0%% stego", black, 0, 1, 18);


    // Iterate over each block and color the block according to the probability of being stego
    int block = 0;
    for (int y = 0; y <= image->height - 32; y += 32) {
        for (int x = 0; x <= image->width - 32; x += 32) {
            // Option to show only binary output
//            if (output[block](1) > 0.5) output[block](1) = 1;
//            else output[block](1) = 0;
            rgb_channels channels = getRGBFromPercentage(output[block++](1));

            for (int j = y; j < y + 32; j++) {
                for (int k = x; k < x + 32; k++) {
                    heatmap(k + 32, j + 32, 0, 0) = channels.r;
                    heatmap(k + 32, j + 32, 0, 1) = channels.g;
                    heatmap(k + 32, j + 32, 0, 2) = channels.b;
                }
            }
        }
    }

    // Save image to heatmap_filepath
    heatmap.save(heatmap_filepath.c_str());

    // Send response
    return "{\"isStego\": " + to_string(maxStegoProbability > isStegoThreshold) + ", \"maxStegoProbability\": " + to_string(maxStegoProbability) + ", \"expectedBytes\": " + to_string(expectedBytes) + ", \"heatmapUrl\": \"" + heatmap_filename + "\"}";
}


rgb_channels getRGBFromPercentage(double value) {
    // Ranges from green being 0 and red being 1
    rgb_channels channels;

    float minimum = 0;
    float maximum = 1;

    float ratio = 2 * (value - minimum) / (maximum - minimum);
    channels.b = max(0, static_cast<int>(255 * (1 - ratio)));
    channels.r = max(0, static_cast<int>(255 * (ratio - 1)));
    channels.g = 255 - channels.b - channels.r;

    return channels;
}