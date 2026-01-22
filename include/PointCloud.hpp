#pragma once
#ifndef POINTSPIRE_POINTCLOUD_HPP
#define POINTSPIRE_POINTCLOUD_HPP

#include "tga/tga.hpp"
#include "tga/tga_math.hpp"
#include <string>
#include <vector>

/**
 * @brief Represents a single point in the point cloud.
 *
 * This struct is aligned to 16 bytes to match the std430 layout requirements
 * for GPU storage buffers.
 */
struct Point {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
    float intensity;
};

/**
 * @brief Axis-Aligned Bounding Box (AABB) defining the spatial extents of the cloud.
 */
struct AABB {
    alignas(16) glm::vec3 min;
    alignas(16) glm::vec3 max;
};

/**
 * TODO write docs
 */
struct LPCUniforms {
    AABB bounds;
    uint32_t numPoints;
    uint32_t numUnique;
};

/**
 * TODO write docs
 */
struct Node {
    uint32_t parent;
    uint32_t left;
    uint32_t right;
    uint32_t isLeaf;
    uint32_t mortonCode;
    uint32_t prefixLen;
    uint32_t pointStart;
    uint32_t pointCount;
};

/**
 * @brief Manages the loading, processing, and GPU resource allocation for a point cloud.
 *
 * This class handles loading LAS/LAZ files via PDAL, normalizing coordinates,
 * and creating the necessary Vulkan buffers (Source, Visible, Indirect, and Uniforms)
 * required for compute culling and rendering.
 */
class PointCloud {
public:
    /**
     * @brief Constructs the PointCloud instance and initializes GPU resources.
     *
     * @param tgai Reference to the TGA interface for resource creation.
     */
    PointCloud(tga::Interface& tgai);

    ~PointCloud();

    /**
     * @brief Loads point cloud data from a file using PDAL.
     *
     * Performs two passes over the data:
     * 1. Calculates global bounds for normalization.
     * 2. Normalizes positions and remaps the coordinate system (Z-up to Y-up).
     *
     * @param filepath The path to the .las or .laz file.
     */
    void loadLAS(const std::string& filepath);

    /**
     * @brief Gets the buffer containing all loaded points.
     * @return A const reference to the GPU storage buffer containing the full dataset.
     */
    const tga::Buffer& getSourceBuffer() const { return m_pointBuffer; }

    /**
     * @brief Gets the buffer destined to hold culled, visible points.
     * @return A const reference to the GPU storage buffer for visible points.
     */
    const tga::Buffer& getVisibleBuffer() const { return m_visiblePointBuffer; }

    /**
     * @brief Gets the buffer containing indirect draw commands.
     * @return A const reference to the indirect buffer modified by the compute shader.
     */
    const tga::Buffer& getIndirectBuffer() const { return m_indirectDrawBuffer; }

    /**
     * @brief Gets the Uniform Buffer Object containing the total point count.
     * @return A const reference to the uniform buffer used for compute bounds checking.
     */
    const tga::Buffer& getCullInfoUBO() const { return m_pointCountBuffer; }

    /**
     * @brief Gets the total number of points loaded on the CPU.
     * @return The number of points in the dataset.
     */
    uint32_t getTotalPointCount() const { return static_cast<uint32_t>(m_points.size()); }

    /**
     * @brief Gets the Axis-Aligned Bounding Box of the normalized point cloud.
     * @return A struct containing the min and max coordinates.
     */
    const AABB& getBounds() const { return m_bounds; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getMortonCodesBuffer() const { return m_mortonCodesBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getSortIndicesBuffer() const { return m_sortIndicesBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getLPCUniformsBuffer() const { return m_lpcUniformsBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getBitonicParamsBuffer() const { return m_bitonicParamsBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getHeadFlagsBuffer() const { return m_headFlagsBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getScannedIndicesBuffer() const { return m_scannedIndicesBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getUniqueCodesBuffer() const { return m_uniqueCodesBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getVoxelStartsBuffer() const { return m_voxelStartsBuffer; }

    /**
     * TODO write docs
     * @return
     */
    const tga::Buffer& getNodesBuffer() const { return m_nodesBuffer; }


private:
    tga::Interface& m_tgai;

    // Data
    std::vector<Point> m_points;
    AABB m_bounds;

    // Buffers
    tga::Buffer m_pointBuffer;
    tga::Buffer m_visiblePointBuffer;
    tga::Buffer m_indirectDrawBuffer;
    tga::Buffer m_pointCountBuffer;
    tga::Buffer m_mortonCodesBuffer;
    tga::Buffer m_sortIndicesBuffer;
    tga::Buffer m_lpcUniformsBuffer;
    tga::Buffer m_bitonicParamsBuffer;
    tga::Buffer m_headFlagsBuffer;
    tga::Buffer m_scannedIndicesBuffer;
    tga::Buffer m_uniqueCodesBuffer;
    tga::Buffer m_voxelStartsBuffer;
    tga::Buffer m_nodesBuffer;

};

#endif // POINTSPIRE_POINTCLOUD_HPP