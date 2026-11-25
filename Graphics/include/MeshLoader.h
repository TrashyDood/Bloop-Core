#pragma once

#include "Graphics.h"

#include <string>
#include <vector>

namespace Graphics
{
    void ParseOBJFile(
        std::istream &file, std::vector<std::string> &outNames,
        std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices,
        UInt32 &outSucceededCount, UInt32 &outFailedCount);
    void ReadAndParseModeFile(
        const char *path, std::vector<std::string> &outNames,
        std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices,
        UInt32 &outSucceededCount, UInt32 &outFailedCount);
    bool LoadMeshFileObjLoader(
        const char *path, std::vector<std::string> &outNames,
        std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices);
}
