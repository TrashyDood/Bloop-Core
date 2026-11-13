#include "Graphics.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

void Graphics::CheckGLErrors()
{
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        LOG_ERROR("OpenGL error: 0x%X\n", error);
}

bool Graphics::ReadFileToEnd(const char *filename, std::string &outSource)
{
    if (filename == nullptr)
    {
        LOG_ERROR("Cannot read file: filename is null.\n");
        return false;
    }

    const char *sourcePath = filename;
    std::ifstream file(sourcePath);
    if (!file)
    {
        LOG_ERROR("Failed to open file: %s\n", sourcePath);
        return false;
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad())
    {
        LOG_ERROR("Cannot read file: %s\n", sourcePath);
        return false;
    }

    outSource = contents.str();
    return true;
}

bool Graphics::CompileShader(GLenum type, const char *source, ResourceHandle &outShader)
{
    outShader = 0;
    if (source == nullptr)
    {
        LOG_ERROR("Cannot compile shader: source is null.\n");
        return false;
    }

    outShader = glCreateShader(type);
    if (outShader == 0)
    {
        LOG_ERROR("Failed to create shader.\n");
        return false;
    }

    glShaderSource(outShader, 1, &source, nullptr);
    glCompileShader(outShader);

    GLint success = GL_FALSE;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[512]{};
        glGetShaderInfoLog(outShader, sizeof(log), nullptr, log);
        LOG_ERROR("ShaderResource compilation failed: %s\n", log);
        glDeleteShader(outShader);
        outShader = 0;
        return false;
    }

    return true;
}

bool Graphics::LinkShaders(ResourceHandle &shaderProgram, const ResourceHandle *shaders, std::size_t shadersCount)
{
    if (shaders == nullptr || shadersCount == 0)
    {
        LOG_ERROR("Cannot link shader program: no shader handles provided.\n");
        return false;
    }

    for (std::size_t i = 0; i < shadersCount; ++i)
        if (shaders[i] == 0)
        {
            LOG_ERROR("Cannot link shader program: shader handle at index %zu is invalid.\n", i);
            return false;
        }

    if (shaderProgram == 0)
        shaderProgram = glCreateProgram();
    if (shaderProgram == 0)
    {
        LOG_ERROR("Failed to create shader program.\n");
        return false;
    }

    for (std::size_t i = 0; i < shadersCount; ++i)
        glAttachShader(shaderProgram, shaders[i]);

    glLinkProgram(shaderProgram);

    GLint success = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    for (std::size_t i = 0; i < shadersCount; ++i)
        glDetachShader(shaderProgram, shaders[i]);

    if (success == GL_FALSE)
    {
        char log[512]{};
        glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
        LOG_ERROR("ShaderResource link failed: %s\n", log);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }

    return true;
}

bool Graphics::CreateShaderProgram(
    ResourceHandle &outShaderProgram, const ShaderSource *sources, std::size_t sourcesCount)
{
    outShaderProgram = 0;
    if (sources == nullptr || sourcesCount == 0)
    {
        LOG_ERROR("Cannot create shader program: no shader sources provided.\n");
        return false;
    }

    std::vector<ResourceHandle> shaders(sourcesCount, 0);
    std::vector<std::string> loadedSources(sourcesCount);

    for (std::size_t i = 0; i < sourcesCount; ++i)
    {
        const std::string *source = &sources[i].source;
        if (sources[i].isFile)
        {
            if (!ReadFileToEnd(sources[i].source.c_str(), loadedSources[i]))
            {
                for (const ResourceHandle shader : shaders)
                    if (shader != 0)
                        glDeleteShader(shader);
                return false;
            }
            source = &loadedSources[i];
        }

        if (!CompileShader(sources[i].type, source->c_str(), shaders[i]))
        {
            for (const ResourceHandle shader : shaders)
            {
                if (shader != 0)
                    glDeleteShader(shader);
            }
            return false;
        }
    }

    const bool linkSucceeded = LinkShaders(outShaderProgram, shaders.data(), shaders.size());

    for (const ResourceHandle shader : shaders)
        glDeleteShader(shader);

    return linkSucceeded;
}

bool Graphics::ParseMeshSource(const char *path, MeshVertices &outVertices, MeshIndices &outIndices)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        LOG_ERROR("Failed to parse mesh: cannot open file.\n");
        return false;
    }

    outVertices.clear();
    outIndices.clear();

    std::vector<std::size_t> vertexIndices, uvIndices, normalIndices;
    std::vector<Vector3f> normals, positions;
    std::vector<Vector2f> uvs;

    char lineHeader[128];
    while (fscanf(file, "%127s", lineHeader) == 1)
    {
        switch (lineHeader[0])
        {
        case 'v':
            if (lineHeader[1] == 't')
            {
                Vector2f uv{};
                if (fscanf(file, "%f %f", &uv.x, &uv.y) == 2)
                    uvs.push_back(uv);
            }
            else if (lineHeader[1] == 'n')
            {
                Vector3f normal{};
                if (fscanf(file, "%f %f %f", &normal.x, &normal.y, &normal.z) == 3)
                    normals.push_back(normal);
            }
            else if (lineHeader[1] == '\0')
            {
                Vector3f position{};
                if (fscanf(file, "%f %f %f", &position.x, &position.y, &position.z) == 3)
                    positions.push_back(position);
            }
            break;

        case 'f':
        {
            std::array<int, 3> vertexIndex, uvIndex, normalIndex;
            const int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                &vertexIndex[2], &uvIndex[2], &normalIndex[2]);

            if (matches != 9)
            {
                fclose(file);
                LOG_ERROR("Failed to parse mesh: source '%s' contains an unsupported face format.\n", path);
                return false;
            }

            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
            uvIndices.push_back(uvIndex[0]);
            uvIndices.push_back(uvIndex[1]);
            uvIndices.push_back(uvIndex[2]);
            break;
        }

        default:
            break;
        }
    }

    for (std::size_t i = 0; i < vertexIndices.size(); ++i) // Iterate faces
    {
        if (vertexIndices[i] == 0 || vertexIndices[i] > positions.size()
            || uvIndices[i] == 0 || uvIndices[i] > uvs.size()
            || normalIndices[i] == 0 || normalIndices[i] > normals.size())
        {
            fclose(file);
            LOG_ERROR("Face index out of range in mesh source: %s\n", path);
            return false;
        }

        outVertices.emplace_back(
            positions[vertexIndices[i] - 1],
            normals[normalIndices[i] - 1],
            uvs[uvIndices[i] - 1]);

        outIndices.push_back(static_cast<std::uint32_t>(outVertices.size() - 1));
    }

    fclose(file);
    return !outIndices.empty();
}

    bool Graphics::CreateMeshBuffers(ResourceHandle & outVertexArray, ResourceHandle & outVertexBuffer,
        ResourceHandle & outIndexBuffer, const MeshVertices &vertices, const MeshIndices &indices)
    {
        outVertexArray = 0;
        outVertexBuffer = 0;
        outIndexBuffer = 0;

        if (vertices.size() > MaxMeshBufferSize / sizeof(Vertex) ||
            indices.size() > MaxMeshBufferSize / sizeof(std::uint32_t) ||
            sizeof(Vertex) > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
        {
            LOG_ERROR("Cannot create mesh buffers: vertex data, index data, or vertex stride exceeds OpenGL limits.\n");
            return false;
        }

        for (std::size_t i = 0; i < indices.size(); ++i)
            if (indices[i] >= vertices.size())
            {
                LOG_ERROR("Cannot create mesh buffers: index %u at position %zu is outside the vertex range.\n",
                    indices[i], i);
                return false;
            }

        ResourceHandle newVertexArray = 0;
        ResourceHandle newVertexBuffer = 0;
        ResourceHandle newIndexBuffer = 0;

        if (!indices.empty())
            glGenBuffers(1, &newIndexBuffer);
        glGenVertexArrays(1, &newVertexArray);
        glGenBuffers(1, &newVertexBuffer);

        if (newVertexArray == 0 || newVertexBuffer == 0 || (!indices.empty() && newIndexBuffer == 0))
        {
            LOG_ERROR(
                "Cannot create mesh buffers: OpenGL failed to allocate the required vertex array or buffer objects.\n");
            if (newVertexArray != 0)
                glDeleteVertexArrays(1, &newVertexArray);
            if (newIndexBuffer != 0)
                glDeleteBuffers(1, &newIndexBuffer);
            if (newVertexBuffer != 0)
                glDeleteBuffers(1, &newVertexBuffer);
            return false;
        }

        glBindVertexArray(newVertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, newVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
            GL_STATIC_DRAW);

        if (!indices.empty())
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newIndexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
                indices.data(), GL_STATIC_DRAW);
        }

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<const void *>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<const void *>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<const void *>(offsetof(Vertex, UV)));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<const void *>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        outVertexArray = newVertexArray;
        outVertexBuffer = newVertexBuffer;
        outIndexBuffer = newIndexBuffer;
        return true;
    }
