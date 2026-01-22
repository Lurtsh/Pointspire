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

    // Morton Codes (uint)
    m_mortonCodesBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        m_points.size() * sizeof(uint32_t)
    });

    // Sort Indices (uint)
    m_sortIndicesBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        m_points.size() * sizeof(uint32_t)
    });

    m_bitonicParamsBuffer = tgai.createBuffer({
    tga::BufferUsage::uniform,
    2 * sizeof(uint32_t) // j, k
    });

    // Head Flags (uint) - Needs TransferSrc for CPU download
    m_headFlagsBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        m_points.size() * sizeof(uint32_t)
    });

    m_scannedIndicesBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        m_points.size() * sizeof(uint32_t)
    });

    m_uniqueCodesBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        m_points.size() * sizeof(uint32_t)
    });

    m_voxelStartsBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        m_points.size() * sizeof(uint32_t)
    });

    // TODO optimize to make as small as possible
    m_nodesBuffer = tgai.createBuffer({
        tga::BufferUsage::storage,
        2 * m_points.size() * sizeof(Node)
    });

    // Set up LPC uniforms
    /// TODO initialize the number of cells in the index correctly!
    LPCUniforms lpcUniforms = {m_bounds, static_cast<uint32_t>(m_points.size()), 0};
    m_lpcUniformsBuffer = tgai.createBuffer({
        tga::BufferUsage::uniform,
        sizeof(LPCUniforms),
        tgai.createStagingBuffer({sizeof(LPCUniforms), tga::memoryAccess(lpcUniforms)})});
}

PointCloud::~PointCloud() {
    if (m_pointBuffer) m_tgai.free(m_pointBuffer);
    if (m_visiblePointBuffer) m_tgai.free(m_visiblePointBuffer);
    if (m_indirectDrawBuffer) m_tgai.free(m_indirectDrawBuffer);
    if (m_pointCountBuffer) m_tgai.free(m_pointCountBuffer);
    if (m_mortonCodesBuffer) m_tgai.free(m_mortonCodesBuffer);
    if (m_sortIndicesBuffer) m_tgai.free(m_sortIndicesBuffer);
    if (m_lpcUniformsBuffer) m_tgai.free(m_lpcUniformsBuffer);
    if (m_bitonicParamsBuffer) m_tgai.free(m_bitonicParamsBuffer);
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

    // =========================================================
    // DEBUG: PRINT LAS STRUCTURE
    // =========================================================
    pdal::PointLayoutPtr layout = view->layout();
    pdal::Dimension::IdList dims = layout->dims();

    std::cout << "\n=== LAS Record Structure ===" << std::endl;
    std::cout << "Total Points: " << pointCount << std::endl;
    std::cout << "Dimensions found:" << std::endl;

    for (auto id : dims) {
        std::string name = layout->dimName(id);
        std::string type = pdal::Dimension::interpretationName(layout->dimType(id));
        size_t size = layout->dimSize(id);

        std::cout << " - " << std::left << std::setw(15) << name
                  << " | Type: " << std::setw(15) << type
                  << " | Size: " << size << " bytes" << std::endl;
    }
    std::cout << "============================\n" << std::endl;
    // =========================================================

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

    // Initialize the member AABB bounds
    m_bounds.min = glm::vec3(std::numeric_limits<float>::max());
    m_bounds.max = glm::vec3(std::numeric_limits<float>::lowest());

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

        // Update the global bounding box
        m_bounds.min = glm::min(m_bounds.min, pos);
        m_bounds.max = glm::max(m_bounds.max, pos);

        // Normalize color intensity (16-bit to float [0-1])
        float r = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Red, idx)) / 65535.0f;
        float g = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Green, idx)) / 65535.0f;
        float b = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Blue, idx)) / 65535.0f;
        float i = static_cast<float>(view->getFieldAs<uint16_t>(pdal::Dimension::Id::Intensity, idx)) / 65535.0f;

        m_points.push_back(Point{pos, {r, g, b}, i});
    }

    std::cout << "Point cloud min: " << "(" << m_bounds.min.x << ", " << m_bounds.min.y << ", " << m_bounds.min.z << ")" << std::endl;
    std::cout << "Point cloud max: " << "(" << m_bounds.max.x << ", " << m_bounds.max.y << ", " << m_bounds.max.z << ")" << std::endl;
}