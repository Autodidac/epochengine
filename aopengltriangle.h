#pragma once

#include "aengineconfig.h"

#ifdef ALMOND_USING_OPENGL
#ifdef ALMOND_USING_GLM

#include <vector>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

// Basic triangle vertex data (header-only)
inline std::vector<Vertex> getTriangleVertices() {
    return {
        {{0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}},  // Top vertex (Red)
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  // Right vertex (Green)
        {{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // Left vertex (Blue)
    };
}

// Indices for drawing the triangle (header-only)
inline std::vector<uint32_t> getTriangleIndices() {
    return { 0, 1, 2 }; // Triangular face using the three vertices
}

#endif
#endif