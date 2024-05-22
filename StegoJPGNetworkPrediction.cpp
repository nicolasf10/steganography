#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include "NeuralNetwork.h"  // Include your Neural Network header
#include <string>
#include "StegoLib.h"
#include "JpegCustom.h"
#include <filesystem>

using namespace Eigen;
using namespace std;
namespace fs = std::filesystem;

void generateJpegLSBStegos(const string &inputDirectory, const string &outputDirectory, const string &message) {
    try {
        int i = 0;
        // Loop over each entry in the directory
        for (const auto& entry : fs::directory_iterator(inputDirectory)) {
            // Check if the entry is a file
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                cout << "Generating Jpeg LSB stego for: " << entry.path() << endl;

                auto *image = new JpegImage();
                image->loadPng(entry.path().string());
                image->isDebugging = false;

                // Message to encode
                // Get substring of random size between 100 and 380
                int messageLength = 5 + (rand() % 10);
                messageLength = INPUT_JPEG_SIZE;
                int messageStart = rand() % (message.length() - messageLength);
                string messageToEncode = message.substr(messageStart, messageLength);

                // set name to filename but with extension of .dat
                image->encodeJpeg(outputDirectory + "/" + entry.path().filename().string().substr(0, entry.path().filename().string().size() - 4) + ".dat", messageToEncode);

                delete image;
            }

            i++;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

}

void loadJpgLSBData(vector<pair<VectorXd, VectorXd>> &data, const string &coverDirectory, const string &stegoDirectory) {
    const int MAX_FILES_PER_DIRECTORY = 300;

    try {
        // Loop over each entry in the directory
        string directories[] = {stegoDirectory, coverDirectory};

        for (auto& dir : directories) {
            cout << "Loading data from: " << dir << endl;
            // store total number of files in directory
            int n_files = distance(fs::directory_iterator(dir), fs::directory_iterator{});
            int j = 0;
            for (const auto& entry : fs::directory_iterator(dir)) {
                // Check if the entry is a file and a png
                if (entry.is_regular_file() && entry.path().extension() == ".dat" || entry.path().extension() == ".png") {
                    if (j < MAX_FILES_PER_DIRECTORY) {
                        cout << "Loading: " << entry.path() << " | (" << j << " / " << min(n_files, MAX_FILES_PER_DIRECTORY) << (dir == coverDirectory ? ") (1": ") (2") << " / 2)" << endl;
                        j++;
                        auto *image = new JpegImage();
                        image->isDebugging = false;

                        cout << "decoding" << endl;
                        if (entry.path().extension() == ".dat")
                            image->decodeJpeg(entry.path().string());
                        else
                            image->loadPng(entry.path().string());

                        VectorXd input(INPUT_JPEG_SIZE);

                        int i = 0;

                        for (auto &block : image->quantizedBlocks) {
                            for (auto &dctBlock : block) {
                                for (int l = 0; l < 8; l++) {
                                    for (int n = 0; n < 8; n++) {
                                        input(i++) = dctBlock->Y[l][n];
                                        input(i++) = dctBlock->Cb[l][n];
                                        input(i++) = dctBlock->Cr[l][n];
                                    }
                                }
                            }
                        }

                        VectorXd output(2);
                        output << 0, 0;

                        if (dir == stegoDirectory) {
                            output(1) = 1;
                        } else {
                            output(0) = 1;
                        }

                        data.emplace_back(input, output);

                        delete image;
                    }
                }
            }
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void processDataJpeg(vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &data, vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &processedData) {
    // Loop over all data and check if input does not contain NaN, if so remove
    for (int i = 0; i < data.size(); i++) {
        if (data[i].first.hasNaN()) {
            data.erase(data.begin() + i);
            i--;
        }
    }

    for (auto &d : data) {
        VectorXd input = VectorXd::Zero(PROCESSED_JPEG_INPUT);

        // Making histogram of sequential values
        int count = 0;
        int lastLSB = -1;

        for (int i = 0; i < d.first.size(); i++) {
            if (d.first(i) != 0) {
                int value = (int) d.first(i) & 1;
                if (value == lastLSB && count < 8) {
                    count++;
                } else {
                    if (lastLSB != -1) {
                        input((min(count, 8) - 1) + (lastLSB * 8))++;
                    }
                    lastLSB = (int) d.first(i);
                    lastLSB &= 0x01;
                    count = 1;
                }
            }
        }

        if (lastLSB != -1) {
            input((min(count, 8) - 1) + (lastLSB * 8))++;
        }

        pair<Eigen::VectorXd, Eigen::VectorXd> p;
        p.second = d.second;
        p.first = input;
        processedData.emplace_back(p);
    }

    // Normalize the input data
//    for (auto &d : data) {
//        for (int i = 0; i < d.first.size(); i++) {
//            int value = (int)d.first(i) & 1;
//            d.first(i) = value;
//        }
//    }
}

void trainJpegNetwork(vector<pair<VectorXd, VectorXd>> &data, const string &saveNetworkFilepath) {
    // count how many images are stego and how many cover
    int stegoCount = 0;
    int coverCount = 0;

    for (auto &d : data) {
        if (d.second(1) == 1) {
            stegoCount++;
        } else {
            coverCount++;
        }
    }

    // Balance amount of stego and cover images
    int minCount = min(stegoCount, coverCount);
    stegoCount = 0;
    coverCount = 0;

    for (int i = 0; i < data.size(); i++) {
        if (data[i].second(1) == 1 && stegoCount < minCount) {
            stegoCount++;
        } else if (data[i].second(0) == 1 && coverCount < minCount) {
            coverCount++;
        } else {
            data.erase(data.begin() + i);
            i--;
        }
    }

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

    // Printing data information
    cout << "Training data size: " << training_data.size() << endl;

    //  Print the training data
    for (int i = 0; i < min((int)training_data.size(), 10); i++) {
        cout << "Input " << i << ": " << training_data[i].first.transpose() << " Output: " << training_data[i].second.transpose() << endl;
    }

    // print testing data
    for (int i = 0; i < min((int)testing_data.size(), 10); i++) {
        cout << "Input " << i << ": " << testing_data[i].first.transpose() << " Output: " << testing_data[i].second.transpose() << endl;
    }

    cout << "Stego count: " << stegoCount << " Cover count: " << coverCount << endl;

    VectorXi layers {{PROCESSED_JPEG_INPUT, 8, 2}};

    NeuralNetwork nn(layers);

    // Print networks layers
    cout << "Layers: " << nn.layers << endl;

    // Print amount of data
    cout << "Training data size: " << training_data.size() << endl;
    cout << "Testing data size: " << testing_data.size() << endl;

    // Print the first 10 elements of the training data
    for (int i = 0; i < 10; i++) {
        cout << "Input " << i << ": " << training_data[i].first.transpose() << " Output: " << training_data[i].second.transpose() << endl;
    }

    // Perform Stochastic Gradient Descent
    nn.SGD(training_data, 400, 5, 0.01, &testing_data);

    // Save model to binary file
    nn.saveModelToBinary(saveNetworkFilepath);
}


// Define global variables of filepaths
string inputDirectoryJpg = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/cifar10/train/automobile";
string outputDirectoryJpg = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/jpegStegos/automobile";

string inputDirectoryTrainJpg = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/flickrReadyDataset";
string outputDirectoryTrainJpg = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/flickrJpegStegos";

string textFileJpg = "/Users/nicolasfuchs/Desktop/All/WAN/researchStego/script.txt";
string wordsCSVJpg = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/words.csv";

string dataFilePathJpg = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/nn_jpeg_data2.dat";
string networkFilepathJpg = "/Users/nicolasfuchs/CLionProjects/stegoProjectMac/stegoJpegNet2.dat";

int mainJpeg() {
    /*
        Generate stego images using JPEG steganography
            - string text = NeuralNetwork::txtToString(textFileJpg);
            - generateJpegLSBStegos(inputDirectoryTrainJpg, outputDirectoryTrainJpg, text);
    */

    /*
        Load data from stego images into binary data
            - vector<pair<VectorXd, VectorXd>> data;
            - loadJpgLSBData(data, inputDirectoryTrainJpg, outputDirectoryTrainJpg);

            - NeuralNetwork::saveData(data, dataFilePathJpg);
     */

    /*
        Load data from binary data
            - vector<pair<VectorXd, VectorXd>> data;
            - NeuralNetwork::readData(data, dataFilePathJpg, INPUT_JPEG_SIZE, 2);
            - processDataJpeg(data);
     */

    /*
        Train neural network
            - trainJpegNetwork(data, networkFilepath);
     */

    /*
        Save model to binary file
            - nn.saveModelToBinary(saveNetworkFilepath);
    */

/*
    Cover:
    Input: 11  3  2  2  4  4  2 10 24  4  3  0  0  0  0  0
    Input: 10  7  5  4  2  2  6  8 31  6  0  0  0  0  0  0
    Input:  4  8  7  4  3  1  2 10 21  5  2  0  0  0  0  0
    Output: 0.0450328  0.950416
    Output: 0.0314446  0.970195
    Output:  0.87992 0.122364
*/

/*
    Stego:
    Input:  6  3  2  2  5  4  5  8 24  3  2  0  0  0  0  0
    Input:  7  8  3  5  1  4  3 11 26  5  0  0  0  0  0  0
    Input:  5  5  6  5  1  2  2 11 22  3  3  0  0  0  0  0
    Output: 0.0436546  0.954363
    Output: 0.0320854  0.970544
    Output: 0.882645 0.125117
 */

    // LOADING DATA
//    vector<pair<VectorXd, VectorXd>> data;
//    loadJpgLSBData(data, inputDirectoryTrainJpg, outputDirectoryTrainJpg);
//
//    NeuralNetwork::saveData(data, dataFilePathJpg);

    // TRAINING DATA

    vector<pair<VectorXd, VectorXd>> data;
    vector<pair<VectorXd, VectorXd>> processedData;
    NeuralNetwork::readData(data, dataFilePathJpg, INPUT_JPEG_SIZE, 2);
    processDataJpeg(data, processedData);

    trainJpegNetwork(processedData, networkFilepathJpg);


//     feedforward examples
    NeuralNetwork myNn(networkFilepathJpg);
    VectorXd example = VectorXd::Zero(PROCESSED_JPEG_INPUT);
    example << 5,  1,  1,  2,  1,  3,  1, 12, 13,  0,  1,  0,  0,  0,  0,  0;
//    example << 7  8  3  5  1  4  3 11 26  5  0  0  0  0  0  0;
//    example << 7, 8, 3, 5, 1, 4, 3, 11, 26, 5, 0, 0, 0, 0, 0, 0;

    VectorXd output = myNn.feedforward(example);

    cout << "Output: " << output.transpose() << endl;





    return 0;
}
