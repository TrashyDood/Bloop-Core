#include "MeshLoader.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <utility>

bool Graphics::ParseOBJFile(
    std::istream &stream, std::vector<std::string> &outNames,
    std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices)
{
    outNames.clear();
    outVertices.clear();
    outIndices.clear();

    std::vector<Vector2f> uvs;
    std::vector<Vector3f> positions, normals;
    std::string currentName;
    MeshVertices currentVertices;
    MeshIndices currentIndices;
    std::string currentLine;
    bool currentValid = true;

    const auto flushObject = [&]
    {
        if (!currentValid || currentIndices.empty())
        {
            if (!currentValid)
                LOG_ERROR("OBJ parser: Skipping invalid OBJ object.\n");
            currentVertices.clear();
            currentIndices.clear();
            return;
        }

        outNames.emplace_back(std::move(currentName));
        outVertices.emplace_back(std::move(currentVertices));
        outIndices.emplace_back(std::move(currentIndices));
        currentName.clear();
        currentVertices.clear();
        currentIndices.clear();
    };

    // Stream >> string = read until whitespace

    while (std::getline(stream, currentLine))
    {
        std::istringstream lineStream(currentLine);
        std::string type;
        lineStream >> type;

        if (type == "o") // Mark new object found
        {
            flushObject();
            currentValid = true;
            currentName.clear();
            lineStream >> std::ws;
            std::getline(lineStream, currentName);
            continue;
        }

        if (type == "v") // Read vertex position for current object
        {
            Vector3f position;
            const bool lineValid = static_cast<bool>(lineStream >> position.x >> position.y >> position.z);
            currentValid = currentValid && lineValid;
            if (lineValid)
                positions.push_back(position);
        }
        else if (type == "vt") // Read vertex texture coordinate/UV for current object
        {
            Vector2f uv;
            const bool lineValid = static_cast<bool>(lineStream >> uv.x >> uv.y);
            currentValid = currentValid && lineValid;
            if (lineValid)
                uvs.push_back(uv);
        }
        else if (type == "vn") // Read vertex normal
        {
            Vector3f normal;
            const bool lineValid = static_cast<bool>(lineStream >> normal.x >> normal.y >> normal.z);
            currentValid = currentValid && lineValid;
            if (lineValid)
                normals.push_back(normal);
        }
        else if (type == "f") // Read face vertex-indices
        {
            if (!currentValid)
                continue;

            for (std::size_t i = 0; i < 3 && currentValid; ++i)
            {
                int position, uv, normal;
                char trailing; // check for trailing characters after the third index which is invalid.
                std::string token; // read the next vertex index triplet (position/uv/normal)
                if (!(lineStream >> token) ||
                    std::sscanf(token.c_str(), "%d/%d/%d%c", &position, &uv, &normal, &trailing) != 3 ||
                    position <= 0 || uv <= 0 || normal <= 0 || static_cast<std::size_t>(position) > positions.size() ||
                    static_cast<std::size_t>(uv) > uvs.size() || static_cast<std::size_t>(normal) > normals.size())
                {
                    currentValid = false;
                    break;
                }

                currentVertices.emplace_back(positions[position - 1], normals[normal - 1], uvs[uv - 1]);
                currentIndices.push_back(static_cast<std::uint32_t>(currentVertices.size() - 1));
            }
        }
    }

    flushObject();
    return !outVertices.empty();
}

bool Graphics::LoadMeshFile(
    const char *path, std::vector<std::string> &outNames,
    std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices)
{
    outNames.clear();
    outVertices.clear();
    outIndices.clear();

    std::ifstream file(path);
    if (!file)
    {
        LOG_ERROR("Failed to open mesh source: %s\n", path);
        return false;
    }

    if (ParseOBJFile(file, outNames, outVertices, outIndices) && !file.bad())
        return true;

    outNames.clear();
    outVertices.clear();
    outIndices.clear();
    LOG_ERROR("Failed to parse mesh: source '%s' has invalid, unsupported, or unreadable OBJ data.\n", path);
    return false;
}
