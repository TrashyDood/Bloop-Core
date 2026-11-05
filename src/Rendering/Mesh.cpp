#include "Rendering/Mesh.h"

#include <cstddef>
#include <limits>
#include <utility>

Mesh::~Mesh()
{
    Release();
}

Mesh::Mesh(Mesh &&other) noexcept
    : vertices(std::move(other.vertices)),
      indices(std::move(other.indices)),
      vertexArrayHandle(std::exchange(other.vertexArrayHandle, 0)),
      vertexBufferHandle(std::exchange(other.vertexBufferHandle, 0)),
      indexBufferHandle(std::exchange(other.indexBufferHandle, 0)) { }

Mesh &Mesh::operator=(Mesh &&other) noexcept
{
    if (this != &other)
    {
        Release();
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
        vertexArrayHandle = std::exchange(other.vertexArrayHandle, 0);
        vertexBufferHandle = std::exchange(other.vertexBufferHandle, 0);
        indexBufferHandle = std::exchange(other.indexBufferHandle, 0);
    }

    return *this;
}

void Mesh::Release() noexcept
{
    if (vertexArrayHandle != 0)
        glDeleteVertexArrays(1, &vertexArrayHandle);
    if (indexBufferHandle != 0)
        glDeleteBuffers(1, &indexBufferHandle);
    if (vertexBufferHandle != 0)
        glDeleteBuffers(1, &vertexBufferHandle);

    vertexArrayHandle = 0;
    vertexBufferHandle = 0;
    indexBufferHandle = 0;
}

bool Mesh::Initialize()
{
    if (vertexArrayHandle != 0 || vertexBufferHandle != 0 || indexBufferHandle != 0)
    {
        LOG_ERROR("Cannot initialize mesh: OpenGL handles are already set.\n");
        return false;
    }

    if (vertices.empty())
    {
        LOG_ERROR("Cannot initialize mesh: no vertices provided.\n");
        return false;
    }

    const std::size_t maxBufferSize =
        static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());
    if (vertices.size() > maxBufferSize / sizeof(Vertex) ||
        indices.size() > maxBufferSize / sizeof(std::uint32_t) ||
        sizeof(Vertex) > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot initialize mesh: vertex or index data is too large.\n");
        return false;
    }

    for (std::size_t i = 0; i < indices.size(); ++i)
    {
        if (indices[i] >= vertices.size())
        {
            LOG_ERROR("Cannot initialize mesh: index %u at position %zu is outside the vertex range.\n",
                      indices[i], i);
            return false;
        }
    }

    ResourceHandle newVertexArrayHandle = 0;
    ResourceHandle newVertexBufferHandle = 0;
    ResourceHandle newIndexBufferHandle = 0;

    glGenVertexArrays(1, &newVertexArrayHandle);
    glGenBuffers(1, &newVertexBufferHandle);
    if (!indices.empty())
        glGenBuffers(1, &newIndexBufferHandle);

    if (newVertexArrayHandle == 0 || newVertexBufferHandle == 0 ||
        (!indices.empty() && newIndexBufferHandle == 0))
    {
        LOG_ERROR("Cannot initialize mesh: failed to create OpenGL buffers.\n");
        if (newIndexBufferHandle != 0)
            glDeleteBuffers(1, &newIndexBufferHandle);
        if (newVertexBufferHandle != 0)
            glDeleteBuffers(1, &newVertexBufferHandle);
        if (newVertexArrayHandle != 0)
            glDeleteVertexArrays(1, &newVertexArrayHandle);
        return false;
    }

    glBindVertexArray(newVertexArrayHandle);

    glBindBuffer(GL_ARRAY_BUFFER, newVertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newIndexBufferHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
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

    vertexArrayHandle = newVertexArrayHandle;
    vertexBufferHandle = newVertexBufferHandle;
    indexBufferHandle = newIndexBufferHandle;
    return true;
}
