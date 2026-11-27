#pragma once

#include "Shared.h"
#include "ResourceOwner.h"
#include "Graphics.h"

#include <glad/gl.h>

namespace Graphics
{
    // Represents a mesh in OpenGL, managing its vertex array, vertex buffer, and index buffer.
    class Mesh : public ResourceOwner
    {
        ResourceHandle vertexArray  = 0;
        ResourceHandle vertexBuffer = 0;
        ResourceHandle indexBuffer  = 0;
        MeshVertices vertices;
        MeshIndices indices;

    public:
        [[nodiscard]] const std::vector<Vertex> &GetVertices() const    { return vertices; }
        [[nodiscard]] const std::vector<UInt32> &GetIndices() const     { return indices; }
        [[nodiscard]] ResourceHandle GetVertexArray() const             { return vertexArray; }
        [[nodiscard]] ResourceHandle GetVertexBuffer() const            { return vertexBuffer; }
        [[nodiscard]] ResourceHandle GetIndexBuffer() const             { return indexBuffer; }
        [[nodiscard]] bool IsInitialized() const noexcept override      { return vertexArray != 0 && vertexBuffer != 0 && (indices.empty() || indexBuffer != 0); }

        explicit Mesh(const MeshVertices &vertices, const MeshIndices &indices = {});
        explicit Mesh(MeshVertices &&vertices, MeshIndices &&indices = {});
        ~Mesh() { ReleaseResources(); }

        Mesh(Mesh &&other) noexcept;
        Mesh &operator=(Mesh &&other) noexcept;
        Mesh(const Mesh &) = delete;
        Mesh &operator=(const Mesh &) = delete;

        bool InitializeResources() override;
        bool Draw() const;

      private:
        void ReleaseResources() noexcept override;
    };
}