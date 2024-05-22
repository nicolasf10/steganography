# Steganography and Steganalysis Tool
Welcome to the Steganography and Steganalysis tool. This project focuses on embedding hidden information (steganography) and detecting hidden information (steganalysis) within JPEG images. Below is a detailed description of the project components and several key algorithms used.

## Project Components
### 1. Neural Network Library
This library is responsible for creating and training neural networks used in steganalysis. The purpose of these neural networks is to detect the hidden information within images.

### 2. JpegCustom
The `JpegCustom` package is a custom implementation of the JPEG compression process. It includes the Discrete Cosine Transform (DCT), Huffman compression, and Run-length Encoding (RLE), all of which are vital for both the compression and decompression of JPEG images.

### 3. Crow Server
This component is a lightweight HTTP server built using Crow framework. The server provides an interface for users to interact with the tool, allowing for the uploading of images for both steganography and steganalysis purposes.

## Algorithms Explanation
### JPEG Compression
JPEG compression involves several steps to reduce the size of an image while maintaining its visual quality. The key components include:

#### 1. Discrete Cosine Transform (DCT)
The DCT is applied to 8x8 blocks of the image, transforming the pixel values into frequency components. The DCT prioritizes low-frequency components (which contain more visual information) and de-emphasizes high-frequency components (which are less visually important).
![dct visualization](https://github.com/nicolasf10/steganography/blob/main/dctimg.png?raw=true)
#### 2. Quantization
Once the image has been transformed using DCT, the resulting coefficients are quantized. This step reduces the precision of the frequency coefficients, allowing more significant compression.

#### 3. Run-Length Encoding (RLE)
After quantization, many of the higher-frequency coefficients will be zero. The coefficients are read in a zigzag pattern, and RLE is used to efficiently compress long sequences of zeros.
![zig zag visualization](https://github.com/nicolasf10/steganography/blob/main/zigzagimg.png?raw=true)

#### 4. Huffman Coding
Finally, Huffman coding is applied to further compress the data. This lossless compression technique uses variable-length codes to represent more frequent coefficients with shorter codes and less frequent coefficients with longer codes.

### Neural Network Class
The Neural Network class in this project is responsible for creating, training, and deploying neural networks for steganalysis. Below is a simplified explanation of its components and workflow:
![neural network visualization](https://github.com/nicolasf10/steganography/blob/main/neuralnetwork.png?raw=true)

1. **Initialization**: The network’s architecture is defined, usually involving multiple layers of neurons, including input, hidden, and output layers.

2. **Forward Pass**: During the forward pass, input data is passed through the network, layer by layer, to generate predictions.

3. **Loss Calculation**: The divergence between the prediction and the actual label is calculated to quantify the network's error.

4. **Backward Pass (Backpropagation)**: The error is propagated backward through the network to update the weights using gradient descent, thus minimizing the loss.

5. **Training Loop**: The network iterates through many epochs, repeatedly adjusting its weights to improve accuracy.


### Steganalysis
Steganalysis is the method used for detecting the presence of hidden information within images. In this project, neural networks are employed for the steganalysis of both PNG and JPEG images. 

#### Training the Network
To train a neural network for steganalysis, a labeled dataset of images is necessary. This dataset includes both clean images (without hidden data) and steganographic images (with hidden data).

1. **Dataset Preparation**:
    - **PNG Images**: For PNG steganography, images are typically less compressed and retain more of their original structure. This makes them more suitable for hiding data.
    - **(Custom) JPEG Images**: In the JPEG format, the hidden data is embedded in the DCT coefficients of the image. This makes it more challenging to detect.

2. **Network Structure**:
    - The network is designed to process small 32x32 pixel patches of the image. This ensures that even subtle variations in the image can be detected.
    - A rolling window is employed to slide across the entire image, analyzing each 32x32 patch sequentially. This helps to ensure that steganographic data anywhere in the image can be detected.

3. **Training Process**:
    - The neural network is trained separately for PNG and JPEG images.
    - For each image type, the network learns the statistical differences between clean and steganographic images from the training dataset.
    - The training pipeline involves passing 32x32 patches through the network, calculating the loss, and backpropagating the error to update the network weights.

### Author
This project was created by Nicolas Fuchs