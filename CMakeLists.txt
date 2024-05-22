cmake_minimum_required(VERSION 3.27)
project(testinCimgMac)

set(CMAKE_CXX_STANDARD 17)

add_executable(testinCimgMac maintest.cpp
        CImg.h
        JpegCustom.cpp
        Image.cpp
        HelperFunctions.cpp
        NeuralNetwork.cpp
        NeuralNetwork.h
        NetworkTest.cpp
        BinaryClassifierExample.cpp
        StegoPNGNetworkPrediction.cpp
        CrowServer.cpp
        StegoJPGNetworkPrediction.cpp
        other.cpp
        main.cpp
)

# Adding Eigen
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
target_link_libraries(testinCimgMac Eigen3::Eigen)

include_directories(/opt/X11/include)
link_directories(/opt/X11/lib)


find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})

target_link_libraries(testinCimgMac ${X11_LIBRARIES})

#add_executable(stegoProjectMac Source.cpp JpegCustom.cpp Image.cpp Utils.cpp)

## Adding Crow
#find_package(crow REQUIRED)
#target_link_libraries(testinCimgMac crow::crow)