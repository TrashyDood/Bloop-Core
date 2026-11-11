#include "Mesh.h"
#include "Rendering.h"

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

    return CreateMeshBuffers(vertexArray, vertexBuffer, indexBuffer, vertices, indices);
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