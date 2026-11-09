#pragma once

#include "Shared/Shared.h"
#include "Rendering/IResourceOwner.h"
#include "Rendering/Rendering.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace Rendering
{
    // Represents a single vertex in a mesh, containing position, normal, texture coordinates, and color information.
    struct Vertex
    {
        Vector3f position{};
        Vector3f normal{};
        Vector2f textureCoordinates{};
        Color4f color = Colors::White;

        Vertex() = default;
        Vertex(Vector3f position, Vector3f normal = {}, Vector2f textureCoordinates = {}, Color4f color = Colors::White)
            : position(position), normal(normal), textureCoordinates(textureCoordinates), color(color) {}
    };

    using MeshVertices = std::vector<Vertex>;
    using MeshIndices = std::vector<std::uint32_t>;

    // Represents a mesh in OpenGL, managing its vertex array, vertex buffer, and index buffer.
    class Mesh : public IResourceOwner
    {
        static constexpr std::size_t MaxBufferSize = static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max());

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

        static bool CreateBuffers(ResourceHandle &vertexArray, ResourceHandle &vertexBuffer, ResourceHandle &indexBuffer,
            const MeshVertices &vertices, const MeshIndices &indices = {});

      private:
        void ReleaseResources() noexcept override;
    };
}