#include <Resource.h>
#include <fstream>
#include <sstream>
#include <iostream>

std::string LoadTextResource(std::string path) {
    std::string text;
    std::ifstream file;
    // Ensure ifstream objects can throw exceptions
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Open files
        file.open(std::string(AssetsDirectory) + path);
        std::stringstream stream;
        // Read file's buffer contents into streams
        stream << file.rdbuf();
        // Close file handlers
        file.close();
        // Convert stream into string
        text = stream.str();
    }
    catch (std::ifstream::failure e) {
        std::cout << "Could not load text resource file " << path << std::endl;
    }
    return text;
}
