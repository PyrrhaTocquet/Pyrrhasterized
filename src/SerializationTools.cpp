#include "SerializationTools.h"

namespace SerializationTools{

    bool isModelBaked(const std::filesystem::path& path)
    {
        std::filesystem::path bakedModelPath = BAKED_ASSETS_PATH;
        bakedModelPath += path;
        if(std::filesystem::exists(bakedModelPath))
        {
            return true;
        }
        return false;

    }

    void loadBakedModel(const std::filesystem::path& path, std::vector<Mesh>&meshes)
    {
    
        std::filesystem::path bakedModelPath = BAKED_ASSETS_PATH;
        bakedModelPath += path;
        std::ifstream file(bakedModelPath, std::ios::binary);

        if (!file.is_open()) {
            // Handle file open error, e.g., throw an exception
            throw std::runtime_error("Failed to open file for reading.");
        }

        // Read the number of meshes
        uint32_t numMeshes;
        file.read(reinterpret_cast<char*>(&numMeshes), sizeof(uint32_t));

        meshes.resize(numMeshes);

        for (Mesh& mesh : meshes) {
            // Read the size of vertices and meshlets
            uint32_t numVertices, numMeshlets;
            file.read(reinterpret_cast<char*>(&numVertices), sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(&numMeshlets), sizeof(uint32_t));

            mesh.vertices.resize(numVertices);
            mesh.meshlets.resize(numMeshlets);

            for (Meshlet& meshlet : mesh.meshlets) {
                // Read the size of primitiveIndices and uniqueVertexIndices
                uint32_t numPrimitives, numUniqueVertices;
                file.read(reinterpret_cast<char*>(&numPrimitives), sizeof(uint32_t));
                file.read(reinterpret_cast<char*>(&numUniqueVertices), sizeof(uint32_t));

                meshlet.primitiveIndices.resize(numPrimitives);
                meshlet.uniqueVertexIndices.resize(numUniqueVertices);
            }
        }

        for (Mesh& mesh : meshes) {
            for (Vertex& v : mesh.vertices) {
                // Read vertex data
                file.read(reinterpret_cast<char*>(&v.pos.x), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.pos.y), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.pos.z), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.normal.x), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.normal.y), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.texCoord.x), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.texCoord.y), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.tangent.x), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.tangent.y), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.tangent.z), sizeof(float));
                file.read(reinterpret_cast<char*>(&v.tangent.w), sizeof(float));
            }

            for (Meshlet& m : mesh.meshlets) {
                for (Meshlet::Triangle& p : m.primitiveIndices) {
                    // Read primitiveIndices data
                    file.read(reinterpret_cast<char*>(&p.i0), sizeof(uint32_t));
                    file.read(reinterpret_cast<char*>(&p.i1), sizeof(uint32_t));
                    file.read(reinterpret_cast<char*>(&p.i2), sizeof(uint32_t));
                }

                for (uint32_t& i : m.uniqueVertexIndices) {
                    // Read uniqueVertexIndices data
                    file.read(reinterpret_cast<char*>(&i), sizeof(uint32_t));
                }
            }
        }

    }

    void writeBakedModel(const std::filesystem::path& path, std::vector<Mesh>& meshes)
    {
        std::filesystem::path bakedModelPath = BAKED_ASSETS_PATH;
        bakedModelPath += path;
        std::filesystem::create_directories(bakedModelPath.parent_path());
        std::ofstream file(bakedModelPath, std::ios::binary);

        /*
        MESHES COUNT
            MESH 0 VERTICES COUNT
            MESH 0 MESHLETS COUNT
                MESHLET 0 TRIANGLE COUNT
                MESHLET 0 INDEX COUNT
                MESHLET 0 BOUNDING SPHERE
            MESH 1 VERTICES COUNT
            ....
        */
        uint32_t numMeshes = meshes.size();
        file.write(reinterpret_cast<char*>(&numMeshes), sizeof(uint32_t));

    for (const Mesh& mesh : meshes) {
        // Write the size of vertices and meshlets
        uint32_t numVertices = mesh.vertices.size();
        uint32_t numMeshlets = mesh.meshlets.size();
        file.write(reinterpret_cast<char*>(&numVertices), sizeof(uint32_t));
        file.write(reinterpret_cast<char*>(&numMeshlets), sizeof(uint32_t));

        for (const Meshlet& meshlet : mesh.meshlets) {
            // Write the size of primitiveIndices and uniqueVertexIndices
            uint32_t numPrimitives = meshlet.primitiveIndices.size();
            uint32_t numUniqueVertices = meshlet.uniqueVertexIndices.size();
            file.write(reinterpret_cast<char*>(&numPrimitives), sizeof(uint32_t));
            file.write(reinterpret_cast<char*>(&numUniqueVertices), sizeof(uint32_t));
        }
    }

    for ( Mesh& mesh : meshes) {
        for ( Vertex& v : mesh.vertices) {
            // Write vertex data
            file.write(reinterpret_cast<char*>(&v.pos.x), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.pos.y), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.pos.z), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.normal.x), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.normal.y), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.texCoord.x), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.texCoord.y), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.tangent.x), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.tangent.y), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.tangent.z), sizeof(float));
            file.write(reinterpret_cast<char*>(&v.tangent.w), sizeof(float));
        }

        for ( Meshlet& m : mesh.meshlets) {
            for ( Meshlet::Triangle& p : m.primitiveIndices) {
                // Write primitiveIndices data
                file.write(reinterpret_cast<char*>(&p.i0), sizeof(uint32_t));
                file.write(reinterpret_cast<char*>(&p.i1), sizeof(uint32_t));
                file.write(reinterpret_cast<char*>(&p.i2), sizeof(uint32_t));
            }

            for (uint32_t i : m.uniqueVertexIndices) {
                // Write uniqueVertexIndices data
                file.write(reinterpret_cast<char*>(&i), sizeof(uint32_t));
            }
        }
    }
        file.close();
    }    
}