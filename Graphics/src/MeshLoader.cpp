#include "MeshLoader.h"
#include "LogHandler.h"

#include "OBJ_Loader.h"

#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

void Graphics::ParseOBJFile(std::istream &stream, std::vector<std::string> &outNames,
    std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices,
    UInt32 &outSucceededCount, UInt32 &outFailedCount)
{
    outNames.clear();
    outVertices.clear();
    outIndices.clear();
    outSucceededCount = 0;
    outFailedCount = 0;

    std::vector<Vector2f> uvs;
    std::vector<Vector3f> positions, normals;
    std::string currentName;
    MeshVertices currentVertices;
    MeshIndices currentIndices;
    std::string currentLine;
    bool isCurrentValid = true;
    bool hasCurrentObject = false;

    const auto flushObject = [&]
    {
        if (!hasCurrentObject)
            return;

        if (!isCurrentValid || currentIndices.empty())
        {
            if (!isCurrentValid)
                LOG_WARNING("OBJ parser: skipping invalid object '%s'.\n",
                    currentName.empty() ? "(unnamed)" : currentName.c_str());
            ++outFailedCount;
            currentVertices.clear();
            currentIndices.clear();
            currentName.clear();
            hasCurrentObject = false;
            return;
        }

        outNames.emplace_back(std::move(currentName));
        outVertices.emplace_back(std::move(currentVertices));
        outIndices.emplace_back(std::move(currentIndices));
        ++outSucceededCount;
        currentName.clear();
        currentVertices.clear();
        currentIndices.clear();
        hasCurrentObject = false;
    };

    // Stream >> string = read until whitespace for strings
    // Stream >> float = read until no number for floats

    while (std::getline(stream, currentLine))
    {
        std::istringstream lineStream(currentLine);
        std::string type;
        lineStream >> type;

        if (type == "o") // Mark new object found
        {
            flushObject(); // Flush the previous object before starting a new one
            isCurrentValid = true;
            hasCurrentObject = true;
            currentName.clear();
            lineStream >> std::ws; // Skip whitespace before reading the object name, which may contain spaces
            std::getline(lineStream, currentName);
            continue;
        }

        if (!isCurrentValid) // Skip parsing lines for an invalid object until the next object is found
            continue;

        if (type == "v") // Read vertex position for current object
        {
            Vector3f position;
            if (static_cast<bool>(lineStream >> position.x >> position.y >> position.z))
                positions.push_back(position);
        }
        else if (type == "vt") // Read vertex texture coordinate/UV for current object
        {
            Vector2f uv;
            if (static_cast<bool>(lineStream >> uv.x >> uv.y))
                uvs.push_back(uv);
        }
        else if (type == "vn") // Read vertex normal
        {
            Vector3f normal;
            if (static_cast<bool>(lineStream >> normal.x >> normal.y >> normal.z))
                normals.push_back(normal);
        }
        else if (type == "f") // Read face vertex-indices
        {
            hasCurrentObject = true;
            // Base index for this face.
            const auto firstIndex = static_cast<UInt32>(currentVertices.size());
            std::size_t faceVertexCount = 0;
            std::string token; // Read each face vertex.
            while (lineStream >> token)
            {
                if (token[0] == '#') // Trailing comment found, skip the rest of the line
                    break;
                if (faceVertexCount == 4)
                {
                    isCurrentValid = false;
                    break;
                }

                char trailing;
                Int32 positionIndex, uvIndex, normalIndex;
                if (std::sscanf(token.c_str(), "%" SCNd32 "/%" SCNd32 "/%" SCNd32 "%c",
                        &positionIndex, &uvIndex, &normalIndex, &trailing) != 3 ||
                    positionIndex <= 0 || uvIndex <= 0 || normalIndex <= 0 || static_cast<std::size_t>(positionIndex) > positions.size() ||
                    static_cast<std::size_t>(uvIndex) > uvs.size() || static_cast<std::size_t>(normalIndex) > normals.size())
                {
                    isCurrentValid = false;
                    break;
                }

                currentVertices.push_back({positions[positionIndex - 1], normals[normalIndex - 1], uvs[uvIndex - 1]});
                ++faceVertexCount;
            }

            if (faceVertexCount < 3)
            {
                isCurrentValid = false;
                continue;
            }

            // The first three corners form the first triangle.
            currentIndices.insert(currentIndices.end(), {firstIndex, firstIndex + 1, firstIndex + 2});
            if (faceVertexCount == 4)
                // The second triangle shares the quad's first and third corners.
                currentIndices.insert(currentIndices.end(), {firstIndex, firstIndex + 2, firstIndex + 3});
        }
    }

    flushObject();
}

void Graphics::ReadAndParseModeFile(const char *path, std::vector<std::string> &outNames,
    std::vector<MeshVertices> &outVertices, std::vector<MeshIndices> &outIndices,
    UInt32 &outSucceededCount, UInt32 &outFailedCount)
{
    LOG_SCOPE;

    outNames.clear();
    outVertices.clear();
    outIndices.clear();
    outSucceededCount = 0;
    outFailedCount = 0;

    std::ifstream file(path);
    if (!file)
    {
        LOG_ERROR("Failed to open mesh source: %s\n", path);
        outFailedCount = 1;
        return;
    }

    ParseOBJFile(file, outNames, outVertices, outIndices, outSucceededCount, outFailedCount);
    if (!file.bad() && outSucceededCount > 0)
        return;

    outNames.clear();
    outVertices.clear();
    outIndices.clear();
    outFailedCount += outSucceededCount;
    outSucceededCount = 0;
    if (outFailedCount == 0)
        outFailedCount = 1;
    LOG_ERROR("Failed to parse mesh: source '%s' has invalid, unsupported, or unreadable OBJ data.\n", path);
}