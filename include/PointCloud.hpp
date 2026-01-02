#pragma once
#ifndef POINTSPIRE_POINTCLOUD_HPP
#define POINTSPIRE_POINTCLOUD_HPP

#include "tga/tga.hpp"
#include "BunnyLoader.hpp"
#include <span>

class PointCloud {
public:
    PointCloud(tga::Interface& tgai, const std::vector<PositionVertex>& vertices);

    PointCloud(const PointCloud&) = delete;
    PointCloud& operator=(const PointCloud&) = delete;

    PointCloud(PointCloud&&) = default;
    PointCloud& operator=(PointCloud&&) = default;

    ~PointCloud();

    void draw(tga::CommandRecorder& recorder) const;

    tga::Buffer getBuffer() const {return m_buffer;}

private:
    std::reference_wrapper<tga::Interface> m_tgai;
    tga::Buffer m_buffer;
    uint32_t m_vertexCount;
};


#endif //POINTSPIRE_POINTCLOUD_HPP