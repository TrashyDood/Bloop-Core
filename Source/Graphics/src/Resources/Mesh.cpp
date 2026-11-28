#include "Mesh.h"

#include "Graphics.h"
#include "LogHandler.h"

#include <cstddef>
#include <limits>
#include <utility>
#include <cinttypes>
#include <cmath>

Graphics::Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<UInt32> &indices)
    : vertices(vertices), indices(indices) { }

Graphics::Mesh::Mesh(std::vector<Vertex> &&vertices, std::vector<UInt32> &&indices)
    : vertices(std::move(vertices)), indices(std::move(indices)) { }

Graphics::Mesh::Mesh(Mesh &&other) noexcept
    : ResourceOwner(std::move(other)),
      vertexArray(std::exchange(other.vertexArray, 0)),
      vertexBuffer(std::exchange(other.vertexBuffer, 0)),
      indexBuffer(std::exchange(other.indexBuffer, 0)),
      vertices(std::move(other.vertices)),
      indices(std::move(other.indices)) { }

Graphics::Mesh &Graphics::Mesh::operator=(Mesh &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        vertexArray = std::exchange(other.vertexArray, 0);
        vertexBuffer = std::exchange(other.vertexBuffer, 0);
        indexBuffer = std::exchange(other.indexBuffer, 0);
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
    }

    return *this;
}

void Graphics::Mesh::ReleaseResources() noexcept
{
    if (vertexArray != 0)
        glDeleteVertexArrays(1, &vertexArray);
    if (indexBuffer != 0)
        glDeleteBuffers(1, &indexBuffer);
    if (vertexBuffer != 0)
        glDeleteBuffers(1, &vertexBuffer);

    vertexArray = 0;
    vertexBuffer = 0;
    indexBuffer = 0;
}

bool Graphics::Mesh::InitializeResources()
{
    LOG_SCOPE;

    if (!ResourceOwner::InitializeResources())
        return false;

    if (vertexArray != 0 || vertexBuffer != 0 || indexBuffer != 0)
    {
        LOG_ERROR("Cannot initialize mesh: Resource handles are already assigned.\n");
        return false;
    }

    if (vertices.empty())
    {
        LOG_ERROR("Cannot initialize mesh: no vertices were provided.\n");
        return false;
    }

    if (!CreateGLMeshBuffers(vertexArray, vertexBuffer, indexBuffer, vertices, indices))
        return false;

    return true;
}

bool Graphics::Mesh::Draw() const
{
    LOG_SCOPE;

    if (!IsInitialized())
    {
        LOG_ERROR("Cannot draw mesh: it is not initialized.\n");
        return false;
    }

    const std::size_t drawCount = indices.empty() ? vertices.size() : indices.size();
    if (drawCount > (std::size_t)(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot draw mesh: draw count exceeds the OpenGL limit.\n");
        return false;
    }

    glBindVertexArray(vertexArray);
    if (indices.empty())
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size()));
    else
        glDrawElements(GL_TRIANGLES, (GLsizei)(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return true;
}

bool Graphics::CreateGLMeshBuffers(ResourceHandle &outVertexArray, ResourceHandle &outVertexBuffer,
    ResourceHandle &outIndexBuffer, const std::vector<Vertex> &vertices, const std::vector<UInt32> &indices)
{
    LOG_SCOPE;

    if (outVertexArray != 0 || outVertexBuffer != 0 || outIndexBuffer != 0)
    {
        LOG_ERROR("Cannot create mesh buffers: output handles are already initialized.\n");
        return false;
    }

    outVertexArray = 0;
    outVertexBuffer = 0;
    outIndexBuffer = 0;

    const static std::size_t MaxMeshBufferSize = static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());
    
    if (vertices.size() > MaxMeshBufferSize / sizeof(Vertex) ||
        indices.size() > MaxMeshBufferSize / sizeof(UInt32) ||
        sizeof(Vertex) > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot create mesh buffers: vertex data, index data, or vertex stride exceeds supported limits.\n");
        return false;
    }

    for (std::size_t i = 0; i < indices.size(); ++i)
        if (indices[i] >= vertices.size())
        {
            LOG_ERROR("Cannot create mesh buffers: index %" PRIu32 " at position %zu is outside the vertex range.\n",
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
        LOG_ERROR("Cannot create mesh buffers: failed to allocate the required vertex array or buffer objects.\n");
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
    glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(UInt32)),
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