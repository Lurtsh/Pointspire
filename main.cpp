#include "tga/tga.hpp"
#include "tga/tga_math.hpp"

#include <iostream>

#include "Application.hpp"

int main() {
    try {
        Application app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
