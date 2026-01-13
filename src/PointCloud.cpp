#include "PointCloud.hpp"

#include <tga/tga_utils.hpp>

#include <pdal/pdal.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>

#include <iomanip> // Required for setprecision
#include <limits>  // Required for max_digits10

PointCloud::PointCloud(tga::Interface &tgai) : m_tgai(tgai) {
    // if (vertices.size() > 0) {
    //     tga::StagingBufferInfo stagingInfo{
    //         vertices.size() * sizeof(PositionVertex),
    //         reinterpret_cast<const uint8_t *>(vertices.data())
    //     };
    //
    //     tga::BufferInfo bufferInfo{
    //         tga::BufferUsage::storage,
    //         vertices.size() * sizeof(PositionVertex),
    //         tgai.createStagingBuffer(stagingInfo)
    //     };
    //
    //     m_buffer = tgai.createBuffer(bufferInfo);
    // }

    loadLAS(std::string("assets/neuschwanstein/3DRM_Neuschwanstein.las"));
    m_vertexCount = m_vertices.size();
    std::cout << "Loaded " << m_vertices.size() << " vertices" << std::endl;

    if (m_vertices.size() > 0) {
        tga::StagingBufferInfo stagingInfo{
            500000 * sizeof(Point),
            reinterpret_cast<const uint8_t *>(m_vertices.data())
        };

        std::vector<Point> sliced_points(500000);

        std::ranges::copy(m_vertices.begin(), m_vertices.begin() + 500000, sliced_points.begin());

        for (int i = 0; i < 10; i++) {
            std::cout << "Point " << i << ": (" << sliced_points[i].position.x << ", " << sliced_points[i].position.y << ", " << sliced_points[i].position.z << ")" << std::endl;
        }

        tga::BufferInfo bufferInfo{
            tga::BufferUsage::storage,
            500000 * sizeof(Point),
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
    if (!m_buffer) {
        std::cerr << "Could not initialize the Point cloud buffer!" << std::endl;
        return;
    }

    // recorder.draw(6, 0, m_vertexCount, 0);
    recorder.draw(6, 0, 500000, 0);
}

// void PointCloud::loadLAS(const std::string filepath) {
//     std::cout << "--- Loading File: " << filepath << " ---" << std::endl;
//
//     // 1. Setup options
//     pdal::Options options;
//     options.add("filename", filepath);
//
//     // 2. Create the reader
//     pdal::StageFactory factory;
//     pdal::Stage* reader = factory.createStage("readers.las");
//     reader->setOptions(options);
//
//     // 3. Prepare the PointTable
//     // 'prepare' reads the header and establishes the schema (PointLayout)
//     pdal::PointTable table;
//     reader->prepare(table);
//
//     // --- TRACING: Print Attribute Structure ---
//     pdal::PointLayoutPtr layout = table.layout();
//     pdal::Dimension::IdList dims = layout->dims();
//
//     std::cout << "Attribute Structure (Dimensions):" << std::endl;
//     for (auto id : dims) {
//         std::string name = layout->dimName(id);
//         pdal::Dimension::Type type = layout->dimType(id);
//         std::string typeName = pdal::Dimension::interpretationName(type);
//
//         std::cout << "  - " << name << " (" << typeName << ")" << std::endl;
//     }
//     // ------------------------------------------
//
//     // 4. Execute the reader to load data
//     pdal::PointViewSet viewSet = reader->execute(table);
//     pdal::PointViewPtr view = *viewSet.begin();
//
//     // --- TRACING: Print Count ---
//     std::cout << "Total Points Loaded: " << view->size() << std::endl;
//     std::cout << "-----------------------------------" << std::endl;
//     // ----------------------------,
//
//     // 5. Extract data into our vector
//     std::vector<Point> points;
//     points.reserve(view->size());
//
//     glm::dvec3 minBounds = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
//
//     for (pdal::PointId idx = 0; idx < view->size(); ++idx) {
//         std::cout << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10);
//         if (idx < 10) {
//             std::cout << "Point " << idx << " as double: (" << view->getFieldAs<double>(pdal::Dimension::Id::X, idx) << ", " << view->getFieldAs<double>(pdal::Dimension::Id::Y, idx) << ", " << view->getFieldAs<double>(pdal::Dimension::Id::Z, idx) << ")" << std::endl;
//             std::cout << "Point " << idx << " as float: (" << view->getFieldAs<float>(pdal::Dimension::Id::X, idx) << ", " << view->getFieldAs<float>(pdal::Dimension::Id::Y, idx) << ", " << view->getFieldAs<float>(pdal::Dimension::Id::Z, idx) << ")" << std::endl;
//         }
//         points.push_back(Point{
//             {
//                 view->getFieldAs<float>(pdal::Dimension::Id::X, idx),
//                 view->getFieldAs<float>(pdal::Dimension::Id::Y, idx),
//                 view->getFieldAs<float>(pdal::Dimension::Id::Z, idx)
//             }
//         });
//     }
//
//     m_vertices = points;
// }

void PointCloud::loadLAS(const std::string filepath) {
    std::cout << "--- Loading File: " << filepath << " ---" << std::endl;

    // 1. Setup options
    pdal::Options options;
    options.add("filename", filepath);

    // 2. Create the reader
    pdal::StageFactory factory;
    pdal::Stage* reader = factory.createStage("readers.las");
    reader->setOptions(options);

    // 3. Prepare the PointTable
    pdal::PointTable table;
    reader->prepare(table);

    // 4. Execute the reader to load data
    pdal::PointViewSet viewSet = reader->execute(table);
    pdal::PointViewPtr view = *viewSet.begin();
    pdal::PointId pointCount = view->size();

    std::cout << "Total Points Loaded: " << pointCount << std::endl;

    // --- PASS 1: Calculate Bounds (Double Precision) ---
    // We need the absolute minimum X, Y, Z to use as our "World Origin" offset
    glm::dvec3 minBounds = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()
    };

    for (pdal::PointId idx = 0; idx < pointCount; ++idx) {
        double x = view->getFieldAs<double>(pdal::Dimension::Id::X, idx);
        double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, idx);
        double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, idx);

        if (x < minBounds.x) minBounds.x = x;
        if (y < minBounds.y) minBounds.y = y;
        if (z < minBounds.z) minBounds.z = z;
    }

    // Store this offset in the class so we can reconstruct world coordinates later if needed
    // (Assuming you have a member variable m_offset, otherwise just use it locally)
    // m_offset = minBounds;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Calculated Offset (MinBounds): "
              << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << std::endl;

    // --- PASS 2: Normalize and Fill Vector ---
    std::vector<Point> points;
    points.reserve(pointCount);

    for (pdal::PointId idx = 0; idx < pointCount; ++idx) {
        // 1. Get High Precision Raw Data
        double rawX = view->getFieldAs<double>(pdal::Dimension::Id::X, idx);
        double rawY = view->getFieldAs<double>(pdal::Dimension::Id::Y, idx);
        double rawZ = view->getFieldAs<double>(pdal::Dimension::Id::Z, idx);

        // 2. Normalize (Subtract Offset)
        // The result is now small enough to fit into a float without precision loss
        float normX = static_cast<float>(rawX - minBounds.x);
        float normY = static_cast<float>(rawY - minBounds.y);
        float normZ = static_cast<float>(rawZ - minBounds.z);

        // Debug Print for the first few points to verify fix
        if (idx < 5) {
            std::cout << "Pt " << idx << " | Raw: " << rawX
                      << " -> Norm: " << normX << std::endl;
        }

        // 3. Store
        points.push_back(Point{
            {normX, normY, normZ}
        });
    }

    std::cout << "-----------------------------------" << std::endl;

    m_vertices = points;
}