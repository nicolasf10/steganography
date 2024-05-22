#include "StegoLib.h"
#include <string>
#include <iostream>
#undef Success
#include <Eigen/Core>

using namespace std;

void messageToVector(string path, vector<unsigned char> *vec) {
	// Iterating over each character and adding it to the vector
	for (int i = 0; i < path.length(); i++) {
		vec->push_back(path[i]);
	}
}

bool getBit(unsigned char value, int position) {
    // Shift the bit to the rightmost position
    unsigned char shiftedValue = value >> position;

    // Mask the value to get only the rightmost bit
    unsigned char result = shiftedValue & 0x01;

    // Return the result (1 or 0)
    return result != 0;
}

string loadStringFromFile(string filename) {
    std::ifstream inputFile(filename); // Replace "example.txt" with the path to your file

    if (!inputFile.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return "";
    }

    // Read the entire file into a string
    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string fileContents = buffer.str();

    // Output the contents of the string
    return fileContents;
}

// saveStringToFile();
// Description: Saves a string to a file
// Parameters: string filename - the name of the file to save the string to
//             string contents - the string to save to the file
void saveStringToFile(string filename, string contents) {
    ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        cerr << "Error opening file for writing!" << endl;
        return;
    }

    outputFile << contents;
    outputFile.close();
}