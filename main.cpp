#include "tga/tga.hpp"
#include <iostream>
#include "Application.hpp"
#include "Camera.hpp"
#include "Scene.hpp"

int main() {
    try {
        tga::Interface tgai;
        Application app(tgai);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return -1;
    }
    return 0;
}
