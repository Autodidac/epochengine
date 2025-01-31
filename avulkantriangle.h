#pragma once

#include "aengineconfig.h"

#ifdef ALMOND_USING_GLM

#include <vector>

namespace almondnamespace::vulkan {

    struct Vertex {
        glm::vec2 pos;       // Position
        glm::vec3 color;     // Color (for non-textured objects)
        glm::vec2 texCoord;  // Texture coordinate (for textured objects)
    };

    // Basic triangle vertex data (with color and texture coordinates)
    inline std::vector<Vertex> getTriangleVertices() {
        return {
            {{0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}},  // Top vertex (Red)
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // Right vertex (Green)
            {{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // Left vertex (Blue)
        };
    }

    // Indices for drawing the triangle (header-only)
    inline std::vector<uint32_t> getTriangleIndices() {
        return { 0, 1, 2 }; // Triangular face using the three vertices
    }
}
#endif
