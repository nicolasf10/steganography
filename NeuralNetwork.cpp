//
// Created by Nicolas Fuchs on 02/04/2024.
//

#include "NeuralNetwork.h"

// Returns output of network when input is a
Eigen::VectorXd NeuralNetwork::feedforward(const Eigen::VectorXd &a) {
    Eigen::VectorXd z;
    Eigen::VectorXd activation = a;

    for (int i = 0; i < num_layers - 1; i++) {
        // previous activation
        if (debug) {
            cout << "Layer: " << i << endl;
            cout << "Prev activation: " << activation.transpose() << endl;
        }

        z = (weights[i] * activation) + biases[i];

        activation = sigmoid(z);

        if (debug) {
            // layer
            // print the weights and biases
            cout << "Weights: " << weights[i] << endl;
            cout << "Biases: " << biases[i] << endl;
            cout << "z: " << z << endl;
            cout << "Activation: " << activation << endl;
        }

    }

    if (debug) {
        cout << "Input: " << a.transpose() << endl;
        cout << "Output: " << activation.transpose() << endl;
    }

    return activation;
}

void NeuralNetwork::SGD(std::vector<std::pair<Eigen::VectorXd, Eigen::VectorXd>>& training_data,
         int epochs, int mini_batch_size, double eta,
         const std::vector<std::pair<Eigen::VectorXd, Eigen::VectorXd>>* test_data) {
    int n_test = test_data ? test_data->size() : 0;
    int n = training_data.size();
    random_device rd;
    mt19937 g(rd());

    for (int epoch = 0; epoch < epochs; epoch++) {
        shuffle(training_data.begin(), training_data.end(), g);
        vector<vector<pair<Eigen::VectorXd, Eigen::VectorXd>>> mini_batches;

        for (int k = 0; k < n; k += mini_batch_size) {
            std::vector<std::pair<Eigen::VectorXd, Eigen::VectorXd>> mini_batch(
                    training_data.begin() + k,
                    training_data.begin() + std::min(k + mini_batch_size, n));
            mini_batches.push_back(mini_batch);
        }

        for (auto& batch : mini_batches) {
//            cout << "Batch size: " << batch.size() << endl;
            update_mini_batch(batch, eta);
        }

        if (test_data) {
            cout << "Epoch " << epoch << ": " << evaluate(*test_data) << " / " << n_test << endl;
        } else {
            cout << "Epoch " << epoch << " complete" << endl;
        }
    }
}

void NeuralNetwork::update_mini_batch(const vector<pair<Eigen::VectorXd, Eigen::VectorXd>>& mini_batch, double eta) {
    vector<Eigen::MatrixXd> nabla_w;
    vector<Eigen::VectorXd> nabla_b;

    for (int i = 0; i < num_layers - 1; i++) {
        nabla_w.push_back(Eigen::MatrixXd::Zero(layers[i + 1], layers[i]));
        nabla_b.push_back(Eigen::VectorXd::Zero(layers[i + 1]));
    }

    // For each example in the mini batch calculate the gradient using backpropagation
    for (auto& example : mini_batch) {
        pair<vector<Eigen::MatrixXd>, vector<Eigen::VectorXd>> delta = backprop(example.first, example.second);

        for (size_t i = 0; i < nabla_w.size(); ++i) {
            nabla_w[i] += delta.first[i]; // Accumulate weight gradients
        }

        for (size_t i = 0; i < nabla_b.size(); ++i) {
            nabla_b[i] += delta.second[i]; // Accumulate bias gradients
        }
    }

    size_t mini_batch_size = mini_batch.size();

    // Update weights
    for (size_t i = 0; i < weights.size(); ++i) {
        weights[i] -= (eta / mini_batch_size) * nabla_w[i];
    }

    // Update biases
    for (size_t i = 0; i < biases.size(); ++i) {
        biases[i] -= (eta / mini_batch_size) * nabla_b[i];
    }
}

// Backpropagation
pair<vector<Eigen::MatrixXd>, vector<Eigen::VectorXd>> NeuralNetwork::backprop(const Eigen::VectorXd &x, const Eigen::VectorXd &y) {
    vector<Eigen::MatrixXd> nabla_w;
    vector<Eigen::VectorXd> nabla_b;

    // Initialize the vectors to store the gradients
    for (auto & weight: this->weights) {
        nabla_w.push_back(Eigen::MatrixXd::Zero(weight.rows(), weight.cols()));
    }

    for (auto & bias : this->biases) {
        nabla_b.push_back(Eigen::VectorXd::Zero(bias.size()));
    }

    // Feedforward
    Eigen::VectorXd activation = x;
    vector<Eigen::VectorXd> activations;
    activations.push_back(x);

    vector<Eigen::VectorXd> zs;

    for (int i = 0; i < num_layers - 1; i++) {
        Eigen::VectorXd z = (weights[i] * activation) + biases[i];

        zs.push_back(z);
        activation = sigmoid(z);
        activations.push_back(activation);
    }

    // Backward pass
    Eigen::VectorXd delta = cost_derivative(activations.back(), y).cwiseProduct(sigmoid_prime(zs.back()));
    nabla_b.back() = delta;
    nabla_w.back() = delta * activations[activations.size() - 2].transpose();

    // Loop over the layers in reverse order where l = 1 is the last layer
    for (int l = 2; l < num_layers; ++l) {
        Eigen::VectorXd z = zs[zs.size() - l];
        Eigen::VectorXd sp = sigmoid_prime(z);
        delta = (weights[weights.size() - l + 1].transpose() * delta).cwiseProduct(sp);
        nabla_b[nabla_b.size() - l] = delta;
        nabla_w[nabla_w.size() - l] = delta * activations[activations.size() - l - 1].transpose();
    }

    return make_pair(nabla_w, nabla_b);
}

int NeuralNetwork::evaluate(const vector<pair<Eigen::VectorXd, Eigen::VectorXd>>& test_data) {
    int sum = 0;
    Eigen::VectorXd outputCounter(Eigen::VectorXd::Zero(2));
    Eigen::RowVectorXi expectedCounter(Eigen::RowVectorXi::Zero(2));

    bool isFirst = true;
    int countExamples = 0;

    int countStegos = 0;
    int countCovers = 0;

    for (auto& example : test_data) {
        Eigen::VectorXd output = feedforward(example.first);
        Eigen::VectorXd::Index max_index;
        output.maxCoeff(&max_index);

        // Increment expected counter
        if (example.second(0) == 1) {
            countCovers++;
        } else {
            countStegos++;
        }

        // print first example
        if (countExamples < 10) {
            cout << "First example: " << endl;
            cout << "Input: " << example.first.transpose() << endl;
            cout << "Output: " << output.transpose() << endl;
            cout << "Expected: " << example.second.transpose() << endl;

            countExamples++;
            isFirst = false;
        }

        outputCounter(max_index)++;

        // Increment sum if the output is correct
        if (example.second(max_index) == 1) {
            sum++;
        }
    }

    // print the counts
    cout << "Stegos: " << countStegos << " Covers: " << countCovers << endl;

    cout << "Output counter: " << outputCounter.transpose() << endl;
//    cout << "Expected counter: " << expectedCounter.transpose() << endl;

    return sum;
}
// Saving and loading functions
void NeuralNetwork::saveModelToBinary(const std::string& filename) {
    std::ofstream out(filename, ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing.");
    }

    // Save layer structure
    int numLayers = layers.size();
    out.write(reinterpret_cast<const char*>(&numLayers), sizeof(numLayers));
    out.write(reinterpret_cast<const char*>(layers.data()), numLayers * sizeof(int));

    // Save weights
    for (const auto& weight : weights) {
        int rows = weight.rows();
        int cols = weight.cols();
        out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
        out.write(reinterpret_cast<const char*>(weight.data()), rows * cols * sizeof(double));
    }

    // Save biases
    for (const auto& bias : biases) {
        int size = bias.size();
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out.write(reinterpret_cast<const char*>(bias.data()), size * sizeof(double));
    }

    out.close();
}

void NeuralNetwork::loadModelFromBinary(const std::string& filename) {

    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file for reading.");
    }

    // Load layer structure
    int numLayers;
    in.read(reinterpret_cast<char*>(&numLayers), sizeof(numLayers));
    layers.resize(numLayers);
    in.read(reinterpret_cast<char*>(layers.data()), numLayers * sizeof(int));

    num_layers = numLayers;

    // Load weights
    weights.clear();
    for (int i = 0; i < numLayers - 1; ++i) {
        int rows, cols;
        in.read(reinterpret_cast<char*>(&rows), sizeof(rows));
        in.read(reinterpret_cast<char*>(&cols), sizeof(cols));
        Eigen::MatrixXd weight(rows, cols);
        in.read(reinterpret_cast<char*>(weight.data()), rows * cols * sizeof(double));
        weights.push_back(weight);
    }

    // Load biases
    biases.clear();
    for (int i = 0; i < numLayers - 1; ++i) {
        int size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        Eigen::VectorXd bias(size);
        in.read(reinterpret_cast<char*>(bias.data()), size * sizeof(double));
        biases.push_back(bias);
    }

    in.close();
}

void NeuralNetwork::normalizeData(vector<pair<Eigen::VectorXd, Eigen::VectorXd>> &training_data) {
    if (training_data.empty()) return;

    // Determine the size of vectors
    size_t numFeatures = training_data.front().first.size();

    // Initialize vectors to store means and standard deviations
    Eigen::VectorXd means = Eigen::VectorXd::Zero(numFeatures);
    Eigen::VectorXd stdDevs = Eigen::VectorXd::Zero(numFeatures);

    // Calculate means
    for (const auto& pair : training_data) {
        means += pair.first;
    }

    means /= training_data.size();

    // Calculate standard deviations
    for (const auto& pair : training_data) {
        Eigen::VectorXd diff = pair.first - means;
        stdDevs += diff.cwiseProduct(diff);
    }
    stdDevs = (stdDevs / training_data.size()).cwiseSqrt(); // element-wise square root

    // Check for zero standard deviation and adjust to prevent division by zero
    for (int i = 0; i < stdDevs.size(); ++i) {
        if (stdDevs[i] == 0) stdDevs[i] = 1;
    }

    // Normalize data directly in the original vector
    for (auto& pair : training_data) {
        pair.first = (pair.first - means).cwiseQuotient(stdDevs);
    }
}