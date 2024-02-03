#pragma once
#include "Defs.h"

namespace GeometryTools{
    void bakeMeshlets(uint32_t maxPrimitives, uint32_t maxVertices, uint32_t* indices, uint32_t indexCount, std::vector<Vertex>& vertices, std::vector<Meshlet>& outMeshlets);
    glm::vec4 minimumBoundingSphere(glm::vec3* points, uint32_t count);
}