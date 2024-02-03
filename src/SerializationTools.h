#include "Defs.h"
#include "GeometryTools.h"
#include <fstream>


namespace SerializationTools {

    [[nodiscard]]bool isModelBaked(const std::filesystem::path& path);
    void writeBakedModel(const std::filesystem::path& path, std::vector<Mesh>& meshes);
    void loadBakedModel(const std::filesystem::path& path, std::vector<Mesh>& meshes);
}