#pragma once
#ifndef POINTSPIRE_BUNNYLOADER_HPP
#define POINTSPIRE_BUNNYLOADER_HPP

#include <vector>
#include <string>
#include <tga/tga_math.hpp>

struct PositionVertex {
    alignas(16) glm::vec3 position;
};

std::vector<PositionVertex> loadBunny(const std::string& confFilePath, const std::string& plyDirectory);

#endif //POINTSPIRE_BUNNYLOADER_HPP