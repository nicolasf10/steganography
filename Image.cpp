#include "StegoLib.h"
#include <string>
#include <iostream>

using namespace std;
using namespace cimg_library;


void Image::generateBitmap() {
	width = image->width();
	height = image->height();

	// Defining size of the pixels 2D vector
	pixels.resize(height);
	for (int i = 0; i < height; i++) {
		pixels[i].resize(width);
	}
	
	cimg_forXY(*image, x, y) {
		// Access pixel values at (x, y)
		unsigned char pixelValue = (*image)(x, y, 0, 0);

		// Adding pixel data to the pixels 2D vector
		pixels[y][x].r = (*image)(x, y, 0, 0);
		pixels[y][x].g = (*image)(x, y, 0, 1);
		pixels[y][x].b = (*image)(x, y, 0, 2);
	}
}

void Image::encodeLSB(string outputFilename, string message) {
	int bitCount = 0;
	int terminatorCount = 8;
	int bitLength = message.length() * sizeof(unsigned char) * 8;
	enum encoding_status status = encoding_status::MESSAGE;
	bool bit;
	unsigned char onMask = 0x01;
	unsigned char offMask = 0xFE;

	// Creating new image
	CImg<unsigned char> encoded_image(width, height, 1, 3);

	// Checking if image is large enough to hold the message.txt
	if (bitCount + 8 > width * height * 3) {
		cout << "Message is too large to encode in this image." << endl;
        return;
	}

	// Initiate array with three elements
	unsigned char color[3];

	// Iterating over all pixels in the image and encoding message.txt
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			color[0] = pixels[y][x].r;
			color[1] = pixels[y][x].g;
			color[2] = pixels[y][x].b;

			// Modifying the least significant bit of each color channel
			switch (status) {
			case encoding_status::MESSAGE:
				for (int i = 0; i < 3; i++) {
					if (bitCount == bitLength || status == encoding_status::TERMINATOR) {
						status = encoding_status::TERMINATOR;
						color[i] &= offMask;
						terminatorCount--;
					}
					else {
						bit = getBit(message[bitCount / 8], 7 - bitCount % 8);

						if (bit) {
							color[i] |= onMask;
						}
						else {
							color[i] &= offMask;
						}

						bitCount++;
					}
				}
				break;
			case encoding_status::TERMINATOR:
				if (terminatorCount > 0) {
					for (int i = 0; i < 3; i++) {
						if (terminatorCount == 0) {
							status = encoding_status::DONE;
							break;
						}

						color[i] &= offMask;
						terminatorCount--;
					}
				}
				break;
			default:
				break;
			}

			encoded_image.draw_point(x, y, color);
		}

	}

	// Displaying both images
	CImgList<unsigned char> list(2);

	list.insert(*image, 0).insert(encoded_image, 1);

	// Displaying CImgList
//	CImgDisplay main_disp(list, "Original vs Encoded Image");
//
//	main_disp.resize(width, height / 2);
//	main_disp.move(100, 100);
//
//	// Wait for the user to close the display window
//	while (!main_disp.is_closed()) {
//		main_disp.wait();
//	}

	// Saving encoded image
	encoded_image.save(outputFilename.c_str());
}

string Image::decodeLSB() {
    bool done = false;
    int bitCount = 0;

    string message = "";

    // Write one character at a time
    unsigned char character = 0;
    unsigned char color[3] = { 0, 0, 0 };

    // print new line
    cout << endl << endl;

    // Iterating over all pixels in the image and decoding message.txt
    for (int y = 0; y < height && !done; ++y) {
        for (int x = 0; x < width && !done; ++x) {
            color[0] = pixels[y][x].r;
            color[1] = pixels[y][x].g;
            color[2] = pixels[y][x].b;

            for (int i = 0; i < 3; i++) {
                if (!done) {
                    // Read the next bit from the image
                    bool bit = color[i] & 0x01;

                    // Write the bit to the file
                    character = (character << 1) | bit;

                    // Increment the bit count
                    bitCount++;

                    // Check if we have read 8 bits
                    if (bitCount % 8 == 0) {
                        // Check for the terminator
                        if (character == 0) {
                            done = true;
                        } else {
                            cout << character;

                            // Write the character to the file
                            message.append(1, character);

                            character = 0;
                        }
                    }
                }
            }
        }
    }

    return message;
}

void Image::decodeLSB(string outputFilename) {
	bool done = false;
	int bitCount = 0;

	// Open a new text file for writing
	std::ofstream outputFile(outputFilename);

	if (!outputFile.is_open()) {
		std::cerr << "Error opening file for writing!" << std::endl;
		return;
	}

	// Write one character at a time
	unsigned char character = 0;
	unsigned char color[3] = { 0, 0, 0 };

	// print new line
	cout << endl << endl;

	// Iterating over all pixels in the image and decoding message.txt
	for (int y = 0; y < height && !done; ++y) {
		for (int x = 0; x < width && !done; ++x) {
			color[0] = pixels[y][x].r;
			color[1] = pixels[y][x].g;
			color[2] = pixels[y][x].b;
			
			for (int i = 0; i < 3; i++) {
                if (!done) {
                    // Read the next bit from the image
                    bool bit = color[i] & 0x01;

                    // Write the bit to the file
                    character = (character << 1) | bit;

                    // Increment the bit count
                    bitCount++;

                    // Check if we have read 8 bits
                    if (bitCount % 8 == 0) {
                        // Check for the terminator
                        if (character == 0) {
                            done = true;
                        } else {
                            cout << character;

                            // Write the character to the file
                            outputFile.put(character);

                            character = 0;
                        }
                    }
                }
			}
		}
	}

	// Close the file
	outputFile.close();
}

void Image::displayImage() {
	cout << "Displaying image..." << endl;
	CImgDisplay display(*image, "Loaded Image");

	// Move the display window and set display size
	display.move(100, 100);
	display.resize(width / 2, height / 2);

	// Wait for the user to close the display window
	while (!display.is_closed()) {
		display.wait();
	}
}

void Image::save_resize(const string &outputFilename, int factor) {
	// Resizing the image
	CImg<unsigned char> resized_image(path.c_str());
	resized_image.resize(factor, factor);

	// Saving the resized image
	resized_image.save(outputFilename.c_str());

	//delete resized_image;
}

void Image::printBitmap() {
	// Print 2d vector of pixel data
	for (int i = 0; i < pixels.size(); i++) {
		for (int j = 0; j < pixels[i].size(); j++) {
			cout << "Pixel at (" << i << ", " << j << "): " << static_cast<int>(pixels[i][j].r) << ", " << static_cast<int>(pixels[i][j].g) << ", " << static_cast<int>(pixels[i][j].b) << endl;
		}
	}
}

void Image::save(const string &outputFilename) {
    image->save(outputFilename.c_str());
}