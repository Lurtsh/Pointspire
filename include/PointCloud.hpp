#pragma once
#ifndef POINTSPIRE_POINTCLOUD_HPP
#define POINTSPIRE_POINTCLOUD_HPP

#include "tga/tga.hpp"
#include "BunnyLoader.hpp"
#include <span>

struct Point {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
    float intensity;
};

class PointCloud {
public:
    PointCloud(tga::Interface& tgai);

    PointCloud(const PointCloud&) = delete;
    PointCloud& operator=(const PointCloud&) = delete;

    PointCloud(PointCloud&&) = default;
    PointCloud& operator=(PointCloud&&) = default;

    ~PointCloud();

    void draw(tga::CommandRecorder& recorder) const;

    tga::Buffer getBuffer() const {return m_pointBuffer;}
    tga::Buffer getIndirectBuffer() const {return m_indirectBuffer;}

    float getMaxHeight() const { return m_maxHeight; }

    void loadLAS(const std::string& filepath);

private:
    std::reference_wrapper<tga::Interface> m_tgai;
    tga::Buffer m_pointBuffer;
    tga::Buffer m_indirectBuffer;
    uint32_t m_vertexCount;
    std::vector<Point> m_points;
    float m_maxHeight = 0.0f;
};


#endif //POINTSPIRE_POINTCLOUD_HPP