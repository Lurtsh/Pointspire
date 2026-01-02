#include "BunnyLoader.hpp"
#include "happly.h"
#include "fstream"
#include "sstream"
#include "iostream"
#include "filesystem"

namespace fs = std::filesystem;

std::vector<PositionVertex> loadBunny(const std::string& confFilePath, const std::string& plyDirectory) {
    std::vector<PositionVertex> vertices;
    std::ifstream confFile(confFilePath);

    if (!confFile.is_open()) {
        std::cerr << "Error: Could not open configuration file: " << confFilePath << std::endl;
        return {};
    }

    std::string line;
    while (std::getline(confFile, line)) {
        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "bmesh") {
            std::string filename;
            float tx, ty, tz;
            float qx, qy, qz, qw;

            ss >> filename >> tx >> ty >> tz >> qx >> qy >> qz >> qw;

            fs::path fullPlyPath(plyDirectory + "/" + filename);

            if (!fs::exists(fullPlyPath)) {
                std::cerr << "Error: PLY file not found: " << fullPlyPath << std::endl;
                continue;
            }

            try {
                happly::PLYData plyIn(fullPlyPath.string());

                std::vector<std::array<double, 3>> rawPositions = plyIn.getVertexPositions();

                glm::vec3 translation(tx, ty, tz);

                glm::quat rotation(qw, qx, qy, qz);

                size_t currentSize = vertices.size();

                vertices.reserve(currentSize + rawPositions.size());

                for (const auto& pos : rawPositions) {
                    glm::vec3 localPos(
                        static_cast<float>(pos[0]),
                        static_cast<float>(pos[1]),
                        static_cast<float>(pos[2])
                    );

                    glm::vec3 worldPos = rotation * localPos + translation;

                    vertices.push_back({worldPos});
                }

                std::cout << "Loaded " << filename << ": " << rawPositions.size() << " vertices.\n";
            } catch (std::exception& e) {
                std::cerr << "Error parsing " << filename << ": " << e.what() << std::endl;
            }
        }
    }

    std::cout << "Total Bunny vertices: " << vertices.size() << std::endl;
    return vertices;
}