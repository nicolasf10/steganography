#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include "NeuralNetwork.h"  // Include your Neural Network header

using namespace Eigen;
using namespace std;

vector<pair<VectorXd, VectorXd>> generate_data(int n) {
    // Generate training data
    random_device rd;  // Non-deterministic random number generator
    mt19937 gen(rd());  // Seed for the random number engine
    uniform_real_distribution<> dis(-1.0, 1.0);  // Range between -1.0 and 1.0

    vector<pair<VectorXd, VectorXd>> training_data;

    for (int i = 0; i < n; ++i) {
        double num1 = dis(gen);
        double num2 = dis(gen);

        VectorXd input(2);
        input << num1, num2;

        VectorXd output(2);
        // Set [1, 0] if either input is negative, [0, 1] if both are positive
        if (num1 >= 0 && num2 >= 0) {
            output << 0, 1;
        } else {
            output << 1, 0;
        }

        training_data.push_back(make_pair(input, output));
    }

    return training_data;
}

vector<pair<VectorXd, VectorXd>> generate_data2(int n) {
    random_device rd;  // Non-deterministic random number generator
    mt19937 gen(rd());  // Seed for the random number engine
    uniform_real_distribution<> dis(-1.0, 1.0);  // Range between -1.0 and 1.0

    vector<pair<VectorXd, VectorXd>> training_data;

    for (int i = 0; i < n; ++i) {
        double num1 = dis(gen);
        double num2 = dis(gen);

        VectorXd input(2);
        input << num1, num2;

        VectorXd output(2);
        // Set [1, 0] if either input is negative, [0, 1] if both are positive
        if (cos(num1) >= 0 && sin(num2) >= 0) {
            output << 0, 1;
        } else {
            output << 1, 0;
        }

        training_data.push_back(make_pair(input, output));
    }

    // print the training data
    for (int i = 0; i < 10; i++) {
        cout << "Input: " << training_data[i].first.transpose() << " Output: " << training_data[i].second.transpose() << endl;
    }

    return training_data;
}

int main22() {
    // Create a neural network object with layers [2, 3, 2]
    RowVectorXi layers {{2, 3, 2}};
    NeuralNetwork net(layers);

    // Generate training data
    vector<pair<VectorXd, VectorXd>> training_data = generate_data2(1000);

    // Generate test data
    vector<pair<VectorXd, VectorXd>> test_data = generate_data2(100);

    // Assume a train function exists that takes training data and other parameters such as learning rate and epochs
//    void SGD(vector<pair<Eigen::VectorXd, Eigen::VectorXd>>& training_data,
//             int epochs, int mini_batch_size, double eta,
//             const vector<pair<Eigen::VectorXd, Eigen::VectorXd>>* test_data = nullptr);
    net.SGD(training_data, 1, 1, 0.1, &test_data);  // training for 1000 epochs with a learning rate of 0.1

    // Evaluate the network on the same training data to see if it learned the rule
    int correct_predictions = net.evaluate(training_data);
    cout << "Correct predictions: " << correct_predictions << " out of " << training_data.size() << endl;

    return 0;
}
