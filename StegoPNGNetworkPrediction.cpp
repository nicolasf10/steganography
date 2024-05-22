#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include "NeuralNetwork.h"
#include <string>
#include "StegoLib.h"
#include <filesystem>

using namespace Eigen;
using namespace std;
namespace fs = std::filesystem;

#define INPUT_PNG_SIZE (32 * 32 * 3)

void generateLSBStegos(const string &inputDirectory, const string &outputDirectory, const string &message) {
    try {
        int i = 0;
        // Loop over each entry in the directory
        for (const auto& entry : fs::directory_iterator(inputDirectory)) {
            // Check if the entry is a file
            if (entry.is_regular_file()) {
                std::cout << "Generating LSB stego for: " << entry.path() << std::endl;

                Image *image = new Image(entry.path());
                image->generateBitmap();



                // Message to encode
                // Maximum bits = 32 * 32 * 3 - 8 = 3072 - 8 = 3064
                // Maximum chars = 3064 / 8 = 383
                // Get substring of random size between 100 and 380
                int messageLength = 100 + (rand() % 280);
                int messageStart = rand() % (message.length() - messageLength);
                string messageToEncode = message.substr(messageStart, messageLength);

                image->encodeLSB(outputDirectory + "/" + entry.path().filename().string(), messageToEncode);

                delete image;
            }

            i++;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void load_LSB_data(vector<pair<VectorXd, VectorXd>> &data, const string &coverDirectory, const string &stegoDirectory) {
    const int MAX_FILES_PER_DIR = 5000;
    try {
        // Loop over each entry in the directory
        string directories[] = {coverDirectory, stegoDirectory};

        for (auto& dir : directories) {
            cout << "Loading data from: " << dir << endl;
            // store total number of files in directory
            int n_files = distance(fs::directory_iterator(dir), fs::directory_iterator{});
            int j = 0;
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (j % 10 == 0)
                    cout << "Loading: " << entry.path() << " | (" << j << " / " << min(n_files, MAX_FILES_PER_DIR) << (dir == coverDirectory ? ") (1": ") (2") << " / 2)" << endl;
                j++;

                // Check if the entry is a file and a png
                if (entry.is_regular_file() && entry.path().extension() == ".png" && j < MAX_FILES_PER_DIR) {
                    Image *image = new Image(entry.path());
                    image->generateBitmap();

                    VectorXd input(image->height * image->width * 3);
                    // vector xd of two zeros
                    VectorXd output = VectorXd::Zero(2);
                    int i = 0;

                    for (int y = 0; y < image->height; ++y) {
                        for (int x = 0; x < image->width; ++x) {
                            color pixel = image->pixels[y][x];
                            input(i++) = pixel.r & 0x01;
                            input(i++) = pixel.g & 0x01;
                            input(i++) = pixel.b & 0x01;
                        }
                    }

                    // Setting correct output vector
                    if (dir == coverDirectory) {
                        output(0) = 1;
                    } else {
                        output(1) = 1;
                    }

                    data.emplace_back(input, output);

                    delete image;
                }
            }
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void trainPNGNeuralNetwork(vector<pair<VectorXd, VectorXd>> &data, const string &saveNetworkFilepath) {
    // Split into training and test data
    vector<pair<VectorXd, VectorXd>> testing_data;
    vector<pair<VectorXd, VectorXd>> training_data;

    for (int i = 0; i < data.size(); i++) {
        if (i % 10 == 0) {
            testing_data.push_back(data[i]);
        } else {
            training_data.push_back(data[i]);
        }
    }

    cout << "Training data size: " << training_data.size() << endl;
    cout << "Testing data size: " << testing_data.size() << endl;

    // Print the training data
//    for (int i = 0; i < training_data.size(); i++) {
//        cout << "Input " << i << ": " << training_data[i].first.transpose() << " Output: " << training_data[i].second.transpose() << endl;
//    }

    // Create a neural network with 3 layers
    VectorXi layers {{INPUT_PNG_SIZE, 10, 2}};

    NeuralNetwork nn(layers);
//    nn.debug = false;

    nn.SGD(training_data, 10, 10, 0.3, &testing_data);

    // Save model to
     nn.saveModelToBinary(saveNetworkFilepath);
}

string inputDirectory = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/cifar10/train/automobile";
string outputDirectory = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/LSBStegos/automobile";
string textFile = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/script.txt";

string inputDirectoryTrain = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/flickrReadyDataset";
string outputDirectoryTrain = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/flickrPngStegos";

string dataFilePath = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/nn_data.dat";
string dataFilePathFlickr = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/nn_data_flickr.dat";
string networkFilepath = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/stegoNet.dat";
string networkFilepathFlickr = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/stegoNet.dat";


int mai4n() {
    /*
        Generate stego images using LSB steganography
            - string text = NeuralNetwork::txtToString(textFile);
            - generateLSBStegos(inputDirectory, outputDirectory, text);
     */

    /*
        Load data from stego images into binary data
            - vector<pair<VectorXd, VectorXd>> data;
            - load_LSB_data(data, inputDirectory, outputDirectory);
            - saveData(data, dataFilePath);
     */

    /*
        Load data from binary data file
            - vector<pair<VectorXd, VectorXd>> data;
            - NeuralNetwork::readData(data, dataFilePath, INPUT_PNG_SIZE, 2);
     */

    /*
        Train neural network using data
            - trainPNGNeuralNetwork(data, networkFilepath);
     */


//    string text = NeuralNetwork::txtToString(textFile);
//    generateLSBStegos(inputDirectoryTrain, outputDirectoryTrain, text);


//    vector<pair<VectorXd, VectorXd>> data;
//    load_LSB_data(data, inputDirectoryTrain, outputDirectoryTrain);
//    NeuralNetwork::saveData(data, dataFilePathFlickr);


//    vector<pair<VectorXd, VectorXd>> data;
//    NeuralNetwork::readData(data, dataFilePathFlickr, INPUT_PNG_SIZE, 2);
//    trainPNGNeuralNetwork(data, networkFilepathFlickr);


    // Load neural network model
    NeuralNetwork nn(networkFilepathFlickr);

    string filename = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/flickrReadyDataset/3835083416_block_6_0.png";

    // Handling image
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

    // Evaluate each block using the neural network
    vector<VectorXd> output;

    // Output = (x, y) ; x = probability of being a cover block, y = probability of being a stego block

    output.reserve(data.size());
    for (auto &d : data) {
        cout << "Input: " << d.transpose() << endl;
        cout << "Output: " << nn.feedforward(d).transpose() << endl;

        output.push_back(nn.feedforward(d));
    }

    // Printt output

    return 0;
}
