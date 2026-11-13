#pragma once

#include "Shared.h"
#include "IResourceOwner.h"
#include "Graphics.h"

#include <cstdint>

namespace Graphics
{
    // Represents a mesh in OpenGL, managing its vertex array, vertex buffer, and index buffer.
    class MeshResource : public IResourceOwner
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

        explicit MeshResource(const MeshVertices &vertices, const MeshIndices &indices = {})
            : vertices(vertices), indices(indices) {}
        explicit MeshResource(MeshVertices &&vertices, MeshIndices &&indices = {})
            : vertices(std::move(vertices)), indices(std::move(indices)) {}

        // Transfers mesh data and OpenGL handles, leaving the source empty.
        MeshResource(MeshResource &&other) noexcept;
        // Releases this mesh's handles before taking ownership of the source's.
        MeshResource &operator=(MeshResource &&other) noexcept;

        virtual ~MeshResource() { ReleaseResources(); }

        MeshResource(const MeshResource &) = delete;
        MeshResource &operator=(const MeshResource &) = delete;

        bool InitializeResources() override;
        bool Draw() const;

      private:
        void ReleaseResources() noexcept override;
    };
}