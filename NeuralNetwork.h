//
// Created by Nicolas Fuchs on 02/04/2024.
//

#ifndef TESTINCIMGMAC_NEURALNETWORK_H
#define TESTINCIMGMAC_NEURALNETWORK_H

#include <iostream>
#include <Eigen/Core>
#include <random>
#include <vector>
#include <fstream>

using namespace Eigen;
using namespace std;

enum activation_function {
    SIGMOID,
    RELU,
    TANH
};

class NeuralNetwork {
public:
    int num_layers;
    Eigen::RowVectorXi layers;
    vector<Eigen::MatrixXd> weights;
    vector<Eigen::VectorXd> biases;
    bool debug;

    NeuralNetwork(const Eigen::RowVectorXi &layers) {
        debug = false;

        random_device rd;
        mt19937 gen(rd());

        normal_distribution<> dist(0, 1);

        // Initialize biases (as vectors)
        for (size_t i = 1; i < layers.size(); ++i) {
            Eigen::VectorXd bias(layers[i]);
            for (int j = 0; j < layers[i]; ++j) {
                bias(j) = dist(gen);  // Fill with normally distributed numbers
            }
            biases.push_back(bias);
        }

        // Initialize weights (as matrices)
        for (size_t i = 0; i < layers.size() - 1; ++i) {
            Eigen::MatrixXd weight(layers[i + 1], layers[i]);
            for (int j = 0; j < layers[i + 1]; ++j) {
                for (int k = 0; k < layers[i]; ++k) {
                    weight(j, k) = dist(gen);  // Fill with normally distributed numbers
                }
            }
            weights.push_back(weight);
        }

        num_layers = layers.size();
        this->layers = layers;
    }

    NeuralNetwork(const string &filename) {
        loadModelFromBinary(filename);
    }

    ~NeuralNetwork() {

    }

    // Sigmoid function which receives vector z and returns a vector
    Eigen::VectorXd sigmoid(const Eigen::VectorXd &z) {
        return (1.0 + (-z.array()).exp()).inverse();
    }

    // Derivative of the sigmoid function
    Eigen::VectorXd sigmoid_prime(const Eigen::VectorXd &z) {
        auto sig = sigmoid(z);
        return sig.array() * (1 - sig.array());
    }

    // ReLU function
    Eigen::VectorXd relu(const Eigen::VectorXd &z) {
        return z.cwiseMax(0);
    }

    // Derivative of the ReLU function
    Eigen::VectorXd relu_prime(const Eigen::VectorXd &z) {
        return (z.array() > 0).cast<double>();
    }

    // Tanh function
    Eigen::VectorXd tanh(const Eigen::VectorXd &z) {
        return z.array().tanh();
    }

    // Derivative of the Tanh function
    Eigen::VectorXd tanh_prime(const Eigen::VectorXd &z) {
        return 1 - z.array().tanh().square();
    }

    // Cost function derivative
    Eigen::VectorXd cost_derivative(const Eigen::VectorXd &output_activations, const Eigen::VectorXd &y) {
        return output_activations - y;
    }

    // Feedforward
    Eigen::VectorXd feedforward(const Eigen::VectorXd &a);

    // Stochastic Gradient Descent training
    void update_mini_batch(const vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &mini_batch, double eta);

    // Function to evaluate the network performance
    int evaluate(const vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &test_data);

    // SGD implementation
    void SGD(vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &training_data,
             int epochs, int mini_batch_size, double eta,
             const vector<pair<Eigen::VectorXd, Eigen::VectorXd>> *test_data = nullptr);

    // Backpropagation
    pair<vector<Eigen::MatrixXd>, vector<Eigen::VectorXd>> backprop(const Eigen::VectorXd &x, const Eigen::VectorXd &y);

    // Saving and loading functions
    void saveModelToBinary(const std::string &filename);

    void loadModelFromBinary(const std::string &filename);

    // Normalize training data
    void static normalizeData(vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &training_data);

    // Helper functions

    // Save data in the neural network training format to a file
    static void saveData(vector<pair<VectorXd, VectorXd>> &data, const string &filename) {
        ofstream file(filename, ios::binary);
        if (!file.is_open()) {
            throw runtime_error("Failed to open file for writing.");
        }

        for (auto &d : data) {
            // Write the input vector
            for (int i = 0; i < d.first.size(); i++) {
                file.write(reinterpret_cast<const char*>(&d.first(i)), sizeof(double));
            }

            // Write the output vector
            for (int i = 0; i < d.second.size(); i++) {
                file.write(reinterpret_cast<const char*>(&d.second(i)), sizeof(double));
            }
        }

        file.close();
    }

    // Load data in the neural network training format from a 32x32 png file
    static void readData(vector<pair<VectorXd, VectorXd>> &data, const string &filename, int inputSize, int outputSize) {
        ifstream file(filename, ios::binary);

        if (!file.is_open()) {
            throw runtime_error("Failed to open file for reading.");
        }

//        int inputSize = 32 * 32 * 3;  // Assuming each input vector has 3072 elements (e.g., an image)
//        int outputSize = 2;           // Assuming each output vector has 2 elements

        while (true) {
            VectorXd input(inputSize);
            VectorXd output(outputSize);

            // Read the input vector
            for (int i = 0; i < inputSize; i++) {
                if (!file.read(reinterpret_cast<char*>(&input(i)), sizeof(double))) {
                    return;  // Exit if we fail to read the size of a double
                }
            }

            // Read the output vector
            for (int i = 0; i < outputSize; i++) {
                if (!file.read(reinterpret_cast<char*>(&output(i)), sizeof(double))) {
                    return;  // Exit if we fail to read the size of a double
                }
            }

            data.emplace_back(input, output);  // Only emplace_back if both reads are successful
        }
    }

    static void read32By32DataJPEG(vector<pair<VectorXd, VectorXd>> &data, const string &filename) {

    }

    static string txtToString(const string &path) {
        ifstream file(path);
        if (!file.is_open()) {
            cerr << "Failed to open file: " << path << endl;
            return "";
        } else {
            cout << "Text file opened successfully." << endl;
        }

        string line;
        string text = "";
        while (getline(file, line)) {
            text += line;
        }

        file.close();

        return text;
    }
};

#endif //TESTINCIMGMAC_NEURALNETWORK_H