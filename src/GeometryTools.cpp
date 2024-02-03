#include "GeometryTools.h"
#include <memory>
#include <functional>
#include <unordered_set>

namespace GeometryTools
{
    struct EdgeEntry
    {
        uint32_t i0;
        uint32_t i1;
        uint32_t i2;

        uint32_t face;
        EdgeEntry* next;
    };

    size_t vecHash(const glm::vec3& vec)
    {
        size_t h = 0;

        for (uint32_t i = 0; i < 3; ++i)
        {
            uint32_t highOrd = h & 0xf8000000;
            h = h << 5;
            h = h ^ (highOrd >> 27);
            h = h ^ size_t(vec[i]);
        }

        return h;
    }

    glm::vec4 computeNormal(glm::vec3* tri)
    {
        
        glm::vec3 p0 = tri[0];
        glm::vec3 p1 = tri[1];
        glm::vec3 p2 = tri[2];

        glm::vec3 v01 = p0 - p1;
        glm::vec3 v02 = p0 - p2;

        return glm::vec4(glm::normalize(glm::cross(v01, v02)), 0);
    }

    // Compute number of triangle vertices already exist in the meshlet
    uint32_t computeReuse (const Meshlet& meshlet, uint32_t (&triIndices)[3])
    {
        uint32_t count = 0;

        for(uint32_t i = 0; i < static_cast<uint32_t>(meshlet.uniqueVertexIndices.size()); ++i){
            for(uint32_t j = 0; j < 3u; ++j)
            {
                if (meshlet.uniqueVertexIndices[i] == triIndices[j]){
                    ++count;
                }
            }
        }
        return count;
    }


    // Computes a candidacy score based on spatial locality, orientational coherence, and vertex re-use within a meshlet.
    float computeScore(const Meshlet& meshlet, glm::vec4 sphere, glm::vec4 normal, uint32_t (&triIndices)[3], glm::vec3* triVerts){
        const float reuseWeight = 0.344f;
        const float locWeight = 0.333f;
        const float oriWeight = 0.333f;

        //Vertex reuse
        uint32_t reuse = computeReuse(meshlet, triIndices);
        float reuseScore = 1 - (float(reuse) / 3.0);

        float maxSq = 0;

        for(uint32_t i = 0; i < 3u; ++i){
            glm::vec4 v = sphere - glm::vec4(triVerts[i], 0.0);
            maxSq = std::max(maxSq, glm::dot(v, v));
        }

        float r = sphere.w;
        float r2 = r * r;
        float locScore = std::log(maxSq / r2 + 1);

        //Angle between normal and meshlet cone axis
        glm::vec4 n = computeNormal(triVerts);
        float d = glm::dot(n, normal);
        float oriScore = (-d + 1) / 2.0f;

        float b = reuseWeight * reuseScore + locWeight * locScore + oriWeight * oriScore;

        return b;

    }

    glm::vec4 minimumBoundingSphere(glm::vec3* points, uint32_t count)
    {
        assert(points != nullptr && count != 0);
        
        //min max points indices along each axis
        uint32_t minAxis[3] = {0, 0, 0};
        uint32_t maxAxis[3] = {0, 0, 0};

        for(uint32_t i = 1; i < count; ++i)
        {
            float* point = (float*) (points + i);
            for (uint32_t j = 0; j < 3; ++j)
            {
                float* min = (float*)(&points[minAxis[j]]);
                float* max = (float*)(&points[maxAxis[j]]);

                minAxis[j] = point[j] < min[j] ? i : minAxis[j];
                maxAxis[j] = point[j] > max[j] ? i : maxAxis[j];

            }
        }

        //Max span axis
        float distSqMax = 0;
        uint32_t axis = 0;

        for(uint32_t i = 0; i < 3u; ++i)
        {
            glm::vec3 min = points[minAxis[i]];
            glm::vec3 max = points[maxAxis[i]];

            float distSq = glm::length(max - min);
            if(distSq > distSqMax)
            {
                distSqMax = distSq;
                axis = i;
            }
        }

        glm::vec3 p1 = points[minAxis[axis]];
        glm::vec3 p2 = points[maxAxis[axis]];

        glm::vec3 center = 0.5f * (p1 + p2);
        float radius = 0.5f * glm::length(p2 - p1); 
        float radiusSq = radius * radius;

         // Add all our points to bounding sphere expanding radius & recalculating center point as necessary.
        for(uint32_t i = 0; i < count; ++i)
        {
            glm::vec3 point = points[i];
            float dist = glm::length(point - center);
            float distSq = dist * dist;


            if(distSq > radiusSq){
                glm::vec3 k = (radius/dist) * 0.5f + glm::vec3(0.5f);
                center = center * k + point * (glm::vec3(1) -  k);
                radius = (radius + dist) * 0.5f;
            }
        }

        //return vec4 with center and radius data
        return glm::vec4(center, radius);

    }

    // Sort in reverse order to use vector as a queue with pop_back.
    bool compareScores(const std::pair<uint32_t, float>& a, const std::pair<uint32_t, float>& b)
    {
        return a.second > b.second;
    }

    // Determines whether a candidate triangle can be added to a specific meshlet; if it can, does so.
    bool addToMeshlet(uint32_t maxPrimitives, uint32_t maxVertices, Meshlet& meshlet, uint32_t (&tri)[3]){

        if(meshlet.uniqueVertexIndices.size() == maxVertices)
            return false;

        if(meshlet.primitiveIndices.size() == maxPrimitives)
            return false;

        static const uint32_t undef = uint32_t(-1);
        uint32_t indices[3] = {undef, undef, undef};
        uint32_t newCount = 3;

        for(uint32_t i = 0; i < meshlet.uniqueVertexIndices.size(); ++i)
        {
            for(uint32_t j = 0; j < 3; ++j){
                if(meshlet.uniqueVertexIndices[i] == tri[j])
                {
                    indices[j] = i;
                    --newCount;
                }
            }
        }

        // Will this triangle fit ?
        if (meshlet.uniqueVertexIndices.size() + newCount > maxVertices)
            return false;

        // Add unique vertex indices to unique vertex index list
        for (uint32_t j = 0; j < 3; ++j)
        {
            if (indices[j] == undef)
            {
                indices[j] = static_cast<uint32_t>(meshlet.uniqueVertexIndices.size());
                meshlet.uniqueVertexIndices.push_back(tri[j]);
            }
        }

        //Add the primitive
        typename Meshlet::Triangle prim = {};
        prim.i0 = indices[0];
        prim.i1 = indices[1];
        prim.i2 = indices[2];

        meshlet.primitiveIndices.push_back(prim);

        return true;

    }

    void buildAdjacencyList(const uint32_t* indices, uint32_t indexCount, const std::vector<Vertex>& vertices, uint32_t vertexCount, uint32_t* outAdjacency){
        const uint32_t triCount = indexCount / 3;

        //Create a mapping of non-unique vertex indices to unique positions
        std::vector<uint32_t> uniquePositions; //sparse vector of positions indices. (LUT)
        uniquePositions.resize(vertexCount);

        std::unordered_map<size_t, uint32_t> uniquePositionsMap;
        uniquePositions.reserve(vertexCount);

        for(uint32_t i = 0; i < vertexCount; i++){

            glm::vec3 position = vertices[i].pos;
            size_t hash = vecHash(position);

            auto it = uniquePositionsMap.find(hash);
            if(it != uniquePositionsMap.end()){
                
                //Reference previous index when already encountered
                uniquePositions[i] = it->second;
            }
            else{
                uniquePositionsMap.insert(std::make_pair(hash, static_cast<uint32_t>(i)));
                uniquePositions[i] = static_cast<uint32_t>(i);
            }

        }

        //Linked list of edges for each vertex to ddetermine adjacency
        const uint32_t hashSize = vertexCount / 3;
        std::unique_ptr<EdgeEntry*[]> hashTable(new EdgeEntry*[hashSize]);
        std::unique_ptr<EdgeEntry[]> entries(new EdgeEntry[triCount * 3]);

        std::memset(hashTable.get(), 0, sizeof(EdgeEntry*) * hashSize);
        
        uint32_t entryIndex = 0;

        //Hash table entry for each Edge.
        for (uint32_t iFace = 0; iFace < triCount; ++iFace)
        {
            uint32_t index = iFace * 3;

            for(uint32_t iEdge = 0; iEdge < 3; ++iEdge){
                
                uint32_t i0 = uniquePositions[indices[index + iEdge % 3]];
                uint32_t i1 = uniquePositions[indices[index + (iEdge + 1) % 3]];
                uint32_t i2 = uniquePositions[indices[index + (iEdge + 2) % 3]];

                auto& entry = entries[entryIndex++];
                entry.i0 = i0;
                entry.i1 = i1;
                entry.i2 = i2;

                uint32_t key = entry.i0 % hashSize;

                entry.next = hashTable[key];
                entry.face = iFace;

                hashTable[key] = &entry;

            }
        }

        //Adjacency list
        std::memset(outAdjacency, uint32_t(-1), indexCount * sizeof(uint32_t));

        for(uint32_t iFace = 0; iFace < triCount; ++iFace){

            uint32_t index = iFace * 3;
            for (uint32_t point = 0; point < 3; ++point)
            {
                if(outAdjacency[iFace * 3 + point] != uint32_t(-1)) 
                    continue;
                
                // Look for edges directed in the opposite direction
                uint32_t i0 = uniquePositions[indices[index + ((point + 1) % 3)]];
                uint32_t i1 = uniquePositions[indices[index + ((point) % 3)]];
                uint32_t i2 = uniquePositions[indices[index + ((point + 2) % 3)]];

                //find a face sharing this edge
                uint32_t key = i0 % hashSize;

                EdgeEntry* found = nullptr;
                EdgeEntry* foundPrev = nullptr;

                for (EdgeEntry* current = hashTable[key], *prev = nullptr; current != nullptr; prev = current, current = current->next)
                {
                    if(current->i1 == i1 && current->i0)
                    {
                        found = current;
                        foundPrev = prev;
                        break;
                    }
                }

                //Cache the face normal
                glm::vec4 n0;
                {
                    glm::vec4 p0 = glm::vec4(vertices[i1].pos, 0);
                    glm::vec4 p1 = glm::vec4(vertices[i0].pos, 0);
                    glm::vec4 p2 = glm::vec4(vertices[i2].pos, 0);

                    glm::vec4 e0 = p0 - p1;
                    glm::vec4 e1 = p1 - p2;

                    n0 = glm::vec4(glm::normalize(glm::cross(glm::vec3(e0), glm::vec3(e1))), 0);
                }

                //Use face normal dot product to determine best edge-sharing candidate
                float bestDot = -2.0f;
                
                for(EdgeEntry* current = found, *prev = foundPrev; current != nullptr; prev == current, current = current->next)
                {
                    if(bestDot == -2.0f || (current->i1 == i1 && current->i0 == i0))
                    {
                        glm::vec4 p0 = glm::vec4(vertices[current->i0].pos, 0);
                        glm::vec4 p1 = glm::vec4(vertices[current->i1].pos, 0);
                        glm::vec4 p2 = glm::vec4(vertices[current->i2].pos, 0);

                        glm::vec4 e0 = p0 - p1;
                        glm::vec4 e1 = p1 - p2;

                        glm::vec4 n1 = glm::vec4(glm::normalize(glm::cross(glm::vec3(e0), glm::vec3(e1))), 0);

                        float dot = glm::dot(n0, n1);

                        if(dot > bestDot)
                        {
                            found = current;
                            foundPrev = prev;
                            bestDot = dot;
                        }

                    }
                }

                //Update the hash table and adjacency list
                if (found && found->face != uint32_t(-1))
                {
                    if(foundPrev != nullptr){
                        foundPrev->next = found->next;
                    }
                    else 
                    {
                        hashTable[key] = found->next;
                    }

                    //Update adjacency information
                    outAdjacency[iFace * 3 + point] = found->face;

                    //Search and remove this face from the table linked list
                    uint32_t key2 = i1 % hashSize;
                    for (EdgeEntry* current = hashTable[key2], *prev = nullptr; current != nullptr; prev = current, current = current->next){
                        if(current->face == iFace && current->i1 == i0 && current->i0 == i1){
                            if(prev != nullptr){
                                prev->next = current->next;
                            }
                            else 
                            {
                                hashTable[key2] = current->next;
                            }

                            break;
                        }
                    }

                    bool linked = false;
                    for(uint32_t point2 = 0; point2 < point; ++point2){
                        if(found->face == outAdjacency[iFace * 3 + point2]){
                            linked = true;
                            outAdjacency[iFace * 3 + point] = uint32_t(-1);
                            break;
                        }
                    }

                    if(!linked){
                        uint32_t edge2 = 0;
                        for(; edge2 < 3 ;++edge2)
                        {
                            uint32_t k = indices[found->face * 3 + edge2 ];

                            if(k == uint32_t(-1))
                                continue;

                            if(uniquePositions[k] == i0)
                                break;

                        }
                        if (edge2 < 3)
                        {
                            outAdjacency[found->face * 3 + edge2] = iFace;
                        }
                    }


                }


            }

        }

    }
    bool isMeshletFull(uint32_t maxVerts, uint32_t maxPrims, const Meshlet& meshlet)
    {
        assert(meshlet.uniqueVertexIndices.size() <= maxVerts);
        assert(meshlet.primitiveIndices.size() <= maxPrims);

        return meshlet.uniqueVertexIndices.size() == maxVerts || meshlet.primitiveIndices.size() == maxPrims;
    }

    void bakeMeshlets(uint32_t maxPrimitives, uint32_t maxVertices, uint32_t *indices, uint32_t indexCount, std::vector<Vertex>& vertices, std::vector<Meshlet> &outMeshlets){
        
        const uint32_t triCount = indexCount / 3;

        //Primitive adjacency list
        std::vector<uint32_t> adjacency;
        adjacency.resize(indexCount);

        buildAdjacencyList(indices, indexCount, vertices, vertices.size(), adjacency.data());

        outMeshlets.clear();
        outMeshlets.emplace_back();
        auto* curr = &outMeshlets.back();

        //Bitmask all the triangles to determine wether a specific one has been added
        std::vector<bool> checklist;
        checklist.resize(triCount);

        std::vector<glm::vec3> m_positions;
        std::vector<glm::vec3> normals;
        std::vector<std::pair<uint32_t, float>> candidates;
        std::unordered_set<uint32_t> candidateCheck;

        glm::vec4 psphere, normal;

        //Arbitrarily start at triangle 0
        uint32_t triIndex = 0;
        candidates.push_back(std::make_pair(triIndex, 0.0f));
        candidateCheck.insert(triIndex);

        //Continue adding triangles until
        while (!candidates.empty())
        {
            uint32_t index = candidates.back().first;
            candidates.pop_back();

            uint32_t tri[3] = 
            {
                indices[index * 3],
                indices[index * 3 + 1],
                indices[index * 3 + 2],
            };

            assert(tri[0] < vertices.size());
            assert(tri[1] < vertices.size());
            assert(tri[2] < vertices.size());

            //Try to add a triangle to meshlet
            if (addToMeshlet(maxPrimitives, maxVertices, *curr, tri))
            {
                checklist[index] = true; //Marked
            

                glm::vec3 points[3] = {
                    vertices[tri[0]].pos,
                    vertices[tri[1]].pos,
                    vertices[tri[2]].pos,
                };

                m_positions.push_back(points[0]);
                m_positions.push_back(points[1]);
                m_positions.push_back(points[2]);

                glm::vec3 normal = glm::vec3(computeNormal(points));
                normals.push_back(normal);

                //New bounding sphere & normal axis
                psphere = minimumBoundingSphere(m_positions.data(), static_cast<uint32_t>(m_positions.size()));

                glm::vec4 nsphere = minimumBoundingSphere(normals.data(), static_cast<uint32_t>(normals.size()));
                normal = glm::normalize(glm::vec3(nsphere));

                // Find and add all applicable adjacent triangles to candidate list
                const uint32_t adjIndex = index * 3;
                
                uint32_t adj[3] = {
                    adjacency[adjIndex],
                    adjacency[adjIndex + 1],
                    adjacency[adjIndex + 2],
                };

                for (uint32_t i = 0; i < 3u; ++i){
                    //Invalid triangle in adjacency slot
                    if(adj[i] == -1)
                        continue;

                    //Already processed triangke
                    if(checklist[adj[i]])
                        continue;

                    //Already in the candidate list
                    if(candidateCheck.count(adj[i]))
                        continue;

                    candidates.push_back(std::make_pair(adj[i], FLT_MAX));
                    candidateCheck.insert(adj[i]);
                }

                //Re-Score remaining triangles
                for (uint32_t i = 0; i < static_cast<uint32_t>(candidates.size()); i++)
                {
                    uint32_t candidate = candidates[i].first;
                    
                    uint32_t triIndices[3] = 
                    {
                        indices[candidate * 3],
                        indices[candidate * 3 + 1],
                        indices[candidate * 3 + 2],
                    };

                    assert(triIndices[0] < vertices.size());
                    assert(triIndices[1] < vertices.size());
                    assert(triIndices[2] < vertices.size());

                    glm::vec3 triVerts[3] = 
                    {
                        vertices[triIndices[0]].pos,
                        vertices[triIndices[1]].pos,
                        vertices[triIndices[2]].pos,
                    };

                    candidates[i].second = computeScore(*curr, psphere, glm::vec4(normal, 0.0), triIndices, triVerts);

                }

                // Determine wether we need to move to the next meshlet
                if (isMeshletFull(maxVertices, maxPrimitives, *curr))
                {
                    outMeshlets.back().meshletInfo.boundingSphere = minimumBoundingSphere(m_positions.data(), m_positions.size());
                    m_positions.clear();
                    normals.clear();
                    candidateCheck.clear();

                    // Use one of our existing candidates as the next meshlet seed.
                    if(!candidates.empty())
                    {
                        candidates[0] = candidates.back();
                        candidates.resize(1);
                        candidateCheck.insert(candidates[0].first);
                    }

                    outMeshlets.emplace_back();
                    curr = &outMeshlets.back();
                }
                else 
                {
                    std::sort(candidates.begin(), candidates.end(), &compareScores);
                }

            }
            else 
            {
                if(candidates.empty())
                {
                    outMeshlets.back().meshletInfo.boundingSphere = minimumBoundingSphere(m_positions.data(), m_positions.size());
                    m_positions.clear();
                    normals.clear();
                    candidateCheck.clear();

                    outMeshlets.emplace_back();
                    curr = &outMeshlets.back();
                }
            }

            if(candidates.empty())
            {
                while(triIndex < triCount && checklist[triIndex])
                    ++triIndex;

                if (triIndex == triCount)
                    break;

                candidates.push_back(std::make_pair(triIndex, 0.0f));
                candidateCheck.insert(triIndex);
                
            }
        }
        if (outMeshlets.back().primitiveIndices.empty())
        {
            outMeshlets.pop_back();
        }
    }
}
