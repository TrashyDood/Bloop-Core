#pragma once

#include "Graphics.h"

#include <string>
#include <vector>

namespace Graphics
{
    bool ParseOBJFile(
        std::istream &file, std::vector<std::string> &outNames,
        std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices);
    bool ReadAndParseModeFile(
        const char *path, std::vector<std::string> &outNames,
        std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices);
    bool LoadMeshFileObjLoader(
        const char *path, std::vector<std::string> &outNames,
        std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices);
}
