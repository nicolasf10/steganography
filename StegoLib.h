#pragma once
#include "CImg.h"
//#include <Eigen/Core>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace cimg_library;

// Helper functions
void messageToVector(string path, vector<unsigned char> *vec);
bool getBit(unsigned char value, int position);
string loadStringFromFile(string filename);

// Image class
typedef struct color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} color;

typedef struct ycbcr {
    unsigned char y;
    unsigned char cb;
    unsigned char cr;
} ycbcr;

// enum definitions
enum class encoding_status {
	MESSAGE,
	TERMINATOR,
	DONE
};

class Image {
public:
	string path;
	CImg<unsigned char> *image;
	vector<vector<color>> pixels;
	unsigned int width;
	unsigned int height;

	Image(string path) {
		this->path = path;
		image = new CImg<unsigned char>(path.c_str());
		width = image->width();
		height = image->height();
	}

	void displayImage();

	void generateBitmap();

	void printBitmap();

	void encodeLSB(string outputFilename, string message);

	void decodeLSB(string outputFilename);

    string decodeLSB();

	void save_resize(const string &outputFilename, int factor);

    void save(const string& outputFilename);
};