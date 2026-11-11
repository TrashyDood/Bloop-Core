#pragma once

#include "Shared.h"
#include "IResourceOwner.h"
#include "Rendering.h"

#include <cstdint>

namespace Rendering
{
    // Represents a mesh in OpenGL, managing its vertex array, vertex buffer, and index buffer.
    class Mesh : public IResourceOwner
    {
        ResourceHandle vertexArray = 0;
        ResourceHandle vertexBuffer = 0;
        ResourceHandle indexBuffer = 0;
        MeshVertices vertices;
        MeshIndices indices;

    public:
        [[nodiscard]] const std::vector<Vertex> &GetVertices() const { return vertices; }
        [[nodiscard]] const std::vector<std::uint32_t> &GetIndices() const { return indices; }
        [[nodiscard]] ResourceHandle GetVertexArray() const { return vertexArray; }
        [[nodiscard]] ResourceHandle GetVertexBuffer() const { return vertexBuffer; }
        [[nodiscard]] ResourceHandle GetIndexBuffer() const { return indexBuffer; }

        [[nodiscard]] bool IsInitialized() const override { return vertexArray != 0 && vertexBuffer != 0 && (indices.empty() || indexBuffer != 0); }

        Mesh() = default;
        explicit Mesh(const MeshVertices &vertices, const MeshIndices &indices = {})
            : vertices(vertices), indices(indices) {}
        explicit Mesh(MeshVertices &&vertices, MeshIndices &&indices = {})
            : vertices(std::move(vertices)), indices(std::move(indices)) {}

        virtual ~Mesh() { ReleaseResources(); }

        Mesh(const Mesh &) = delete;
        Mesh &operator=(const Mesh &) = delete;

        bool InitializeResources() override;
        bool Draw() const;

      private:
        void ReleaseResources() noexcept override;
    };
}