#include "Rendering/Mesh.h"

#include <cstddef>
#include <limits>
#include <utility>

Rendering::Mesh::~Mesh() { ReleaseResources(); }

Rendering::Mesh::Mesh(Mesh &&other) noexcept
    : vertexArray(std::exchange(other.vertexArray, 0)),
      vertexBuffer(std::exchange(other.vertexBuffer, 0)),
      indexBuffer(std::exchange(other.indexBuffer, 0)),
      geometry(std::move(other.geometry)) {}

Rendering::Mesh &Rendering::Mesh::operator=(Mesh &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        geometry = std::move(other.geometry);
        vertexArray = std::exchange(other.vertexArray, 0);
        vertexBuffer = std::exchange(other.vertexBuffer, 0);
        indexBuffer = std::exchange(other.indexBuffer, 0);
    }

    return *this;
}

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
        LOG_ERROR("Cannot initialize mesh: OpenGL handles are already set.\n");
        return false;
    }

    if (geometry.vertices.empty())
    {
        LOG_ERROR("Cannot initialize mesh: no vertices provided.\n");
        return false;
    }

    const std::size_t maxBufferSize = static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());
    if (geometry.vertices.size() > maxBufferSize / sizeof(Vertex) ||
        geometry.indices.size() > maxBufferSize / sizeof(std::uint32_t) ||
        sizeof(Vertex) > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot initialize mesh: vertex or index data is too large.\n");
        return false;
    }

    for (std::size_t i = 0; i < geometry.indices.size(); ++i)
        if (geometry.indices[i] >= geometry.vertices.size())
        {
            LOG_ERROR("Cannot initialize mesh: index %u at position %zu is outside the vertex range.\n", geometry.indices[i], i);
            return false;
        }

    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    if (!geometry.indices.empty())
        glGenBuffers(1, &indexBuffer);

    if (vertexArray == 0 || vertexBuffer == 0 || (!geometry.indices.empty() && indexBuffer == 0))
    {
        LOG_ERROR("Cannot initialize mesh: failed to create OpenGL buffers.\n");
        ReleaseResources();
        return false;
    }

    glBindVertexArray(vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(geometry.vertices.size() * sizeof(Vertex)),
                 geometry.vertices.data(),
                 GL_STATIC_DRAW);

    if (!geometry.indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(geometry.indices.size() * sizeof(std::uint32_t)),
                     geometry.indices.data(),
                     GL_STATIC_DRAW);
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

    return true;
}

bool Rendering::Mesh::Draw() const
{
    if (!IsInitialized())
    {
        LOG_ERROR("Cannot draw mesh: it is not initialized.\n");
        return false;
    }

    const std::size_t drawCount = geometry.indices.empty() ? geometry.vertices.size() : geometry.indices.size();
    if (drawCount > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot draw mesh: vertex or index count is too large.\n");
        return false;
    }

    glBindVertexArray(vertexArray);
    if (geometry.indices.empty())
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geometry.vertices.size()));
    else
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geometry.indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return true;
}
