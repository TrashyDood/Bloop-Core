#include "MeshResource.h"

#include "Graphics.h"
#include "LogHandler.h"

#include <cstddef>
#include <limits>
#include <utility>

Graphics::MeshResource::MeshResource(const MeshVertices &vertices, const MeshIndices &indices)
    : vertices(vertices), indices(indices) { }

Graphics::MeshResource::MeshResource(MeshVertices &&vertices, MeshIndices &&indices)
    : vertices(std::move(vertices)), indices(std::move(indices)) { }

Graphics::MeshResource::MeshResource(MeshResource &&other) noexcept
    : ResourceOwner(std::move(other)),
      vertexArray(std::exchange(other.vertexArray, 0)),
      vertexBuffer(std::exchange(other.vertexBuffer, 0)),
      indexBuffer(std::exchange(other.indexBuffer, 0)),
      vertices(std::move(other.vertices)),
      indices(std::move(other.indices)) { }

Graphics::MeshResource &Graphics::MeshResource::operator=(MeshResource &&other) noexcept
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

Graphics::MeshResource::~MeshResource()
{
    ReleaseResources();
}

bool Graphics::MeshResource::IsInitialized() const
{
    return vertexArray != 0 && vertexBuffer != 0 && (indices.empty() || indexBuffer != 0);
}

void Graphics::MeshResource::ReleaseResources() noexcept
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

bool Graphics::MeshResource::InitializeResources()
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

bool Graphics::MeshResource::Draw() const
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