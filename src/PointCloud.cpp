#include "PointCloud.hpp"

#include <tga/tga_utils.hpp>

#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>

#include <limits>

#include <iostream>

PointCloud::PointCloud(tga::Interface &tgai) : m_tgai(tgai) {
    // Load the default asset
    loadLAS("assets/neuschwanstein/3DRM_Neuschwanstein.las");

    if (m_points.empty()) return;

    size_t dataSize = m_points.size() * sizeof(Point);

    // Create the Source Buffer.
    // This buffer contains the complete dataset and is read-only for the compute shader.
    m_pointBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        dataSize,
        tgai.createStagingBuffer({dataSize, reinterpret_cast<uint8_t*>(m_points.data())})
    });

    // Create the Visible Buffer.
    // This buffer is written to by the compute shader and read by the vertex shader.
    // It is allocated to match the source size to handle the worst-case scenario (all points visible).
    m_visiblePointBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        dataSize
    });

    // Initialize the Indirect Draw Command.
    // vertexCount = 6 (for a quad), instanceCount = 0 (reset/filled by compute shader).
    tga::DrawIndirectCommand cmd{6, 0, 0, 0};

    m_indirectDrawBuffer = tgai.createBuffer({
        tga::BufferUsage::indirect | tga::BufferUsage::storage,
        sizeof(tga::DrawIndirectCommand),
        tgai.createStagingBuffer({sizeof(cmd), reinterpret_cast<uint8_t*>(&cmd)})
    });

    // Create the Point Count Uniform Buffer.
    // Used by the compute shader to perform bounds checking on the dispatch index.
    uint32_t totalCount = static_cast<uint32_t>(m_points.size());

    tga::StagingBufferInfo stagingInfo{
        sizeof(uint32_t),
        reinterpret_cast<uint8_t*>(&totalCount)
    };

    m_pointCountBuffer = m_tgai.createBuffer({
        tga::BufferUsage::uniform,
        sizeof(uint32_t),
        m_tgai.createStagingBuffer(stagingInfo)
    });
}

PointCloud::~PointCloud() {
    if (m_pointBuffer) m_tgai.free(m_pointBuffer);
    if (m_visiblePointBuffer) m_tgai.free(m_visiblePointBuffer);
    if (m_indirectDrawBuffer) m_tgai.free(m_indirectDrawBuffer);
}

void PointCloud::loadLAS(const std::string& filepath) {
    std::cout << "--- Loading File: " << filepath << " ---" << std::endl;

    // Configure PDAL pipeline
    pdal::Options options;
    options.add("filename", filepath);
    pdal::StageFactory factory;
    pdal::Stage* reader = factory.createStage("readers.las");
    reader->setOptions(options);
    pdal::PointTable table;
    reader->prepare(table);
    pdal::PointViewSet viewSet = reader->execute(table);
    pdal::PointViewPtr view = *viewSet.begin();
    pdal::PointId pointCount = view->size();

    // Pass 1: Calculate global bounding box for normalization
    m_points.reserve(pointCount);

    // Initialize bounds logic
    glm::dvec3 globalMin = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()
    };

    glm::dvec3 globalMax = {
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()
    };

    // Iterate through points to find min/max values
    for (pdal::PointId idx = 0; idx < pointCount; ++idx) {
        double x = view->getFieldAs<double>(pdal::Dimension::Id::X, idx);
        double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, idx);
        double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, idx);

        if (x < globalMin.x) globalMin.x = x;
        if (y < globalMin.y) globalMin.y = y;
        if (z < globalMin.z) globalMin.z = z;

        if (y > globalMax.y) globalMax.y = y;
    }

    // Pass 2: Normalize coordinates and populate the point vector
    for (pdal::PointId idx = 0; idx < pointCount; ++idx) {
        double rawX = view->getFieldAs<double>(pdal::Dimension::Id::X, idx);
        double rawY = view->getFieldAs<double>(pdal::Dimension::Id::Y, idx);
        double rawZ = view->getFieldAs<double>(pdal::Dimension::Id::Z, idx);

        // Normalize X and Z to start at 0
        float normX = static_cast<float>(rawX - globalMin.x);
        float normZ = static_cast<float>(rawZ - globalMin.z);

        // Invert Y axis for coordinate system compatibility.
        // Calculates distance from the maximum Y to flip the axis while keeping values positive.
        float invNormY = static_cast<float>(globalMax.y - rawY);

        glm::vec3 pos{normX, normZ, invNormY};

        // Normalize color intensity (16-bit to float [0-1])
        float r = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Red, idx)) / 65535.0f;
        float g = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Green, idx)) / 65535.0f;
        float b = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Blue, idx)) / 65535.0f;
        float i = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Intensity, idx)) / 65535.0f;

        m_points.push_back(Point{pos, {r, g, b}, i});
    }
}