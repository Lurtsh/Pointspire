#include "PointCloud.hpp"

#include <tga/tga_utils.hpp>

PointCloud::PointCloud(tga::Interface &tgai, const std::vector<PositionVertex> &vertices) : m_tgai(tgai), m_vertexCount(vertices.size()) {
    if (vertices.size() > 0) {
        tga::StagingBufferInfo stagingInfo{
            vertices.size() * sizeof(PositionVertex),
            reinterpret_cast<const uint8_t *>(vertices.data())
        };

        tga::BufferInfo bufferInfo{
            tga::BufferUsage::storage,
            vertices.size() * sizeof(PositionVertex),
            tgai.createStagingBuffer(stagingInfo)
        };

        m_buffer = tgai.createBuffer(bufferInfo);
    }
}

PointCloud::~PointCloud() {
    if (m_buffer)
        m_tgai.get().free(m_buffer);
}

void PointCloud::draw(tga::CommandRecorder& recorder) const {
    if (!m_buffer) return;

    recorder.draw(6, 0, m_vertexCount, 0);
}
