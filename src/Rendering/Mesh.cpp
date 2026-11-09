#include "Rendering/Mesh.h"
#include "Rendering/Rendering.h"

#include <cstddef>
#include <limits>
#include <utility>

void Rendering::Mesh::ReleaseResources() noexcept
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

bool Rendering::Mesh::InitializeResources()
{
    if (!IResourceOwner::InitializeResources())
        return false;

    if (vertexArray != 0 || vertexBuffer != 0 || indexBuffer != 0)
    {
        LOG_ERROR("Cannot initialize mesh: OpenGL resource handles are already assigned.\n");
        return false;
    }

    if (vertices.empty())
    {
        LOG_ERROR("Cannot initialize mesh: no vertices were provided.\n");
        return false;
    }

    return CreateBuffers(vertexArray, vertexBuffer, indexBuffer, vertices, indices);
}

bool Rendering::Mesh::Draw() const
{
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

bool Rendering::Mesh::CreateBuffers(ResourceHandle &outVertexArray, ResourceHandle &outVertexBuffer,
    ResourceHandle &outIndexBuffer, const MeshVertices &vertices, const MeshIndices &indices)
{
    outVertexArray = 0;
    outVertexBuffer = 0;
    outIndexBuffer = 0;

    if (vertices.size() > MaxBufferSize / sizeof(Vertex) || indices.size() > MaxBufferSize / sizeof(std::uint32_t) ||
        sizeof(Vertex) > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot create mesh buffers: vertex data, index data, or vertex stride exceeds OpenGL limits.\n");
        return false;
    }

    for (std::size_t i = 0; i < indices.size(); ++i)
        if (indices[i] >= vertices.size())
        {
            LOG_ERROR("Cannot create mesh buffers: index %u at position %zu is outside the vertex range.\n", indices[i], i);
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
        LOG_ERROR("Cannot create mesh buffers: OpenGL failed to allocate the required vertex array or buffer objects.\n");
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
        reinterpret_cast<const void *>(offsetof(Vertex, textureCoordinates)));
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