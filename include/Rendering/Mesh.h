#pragma once

#include "Shared/Core.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <utility>
#include <vector>
#include <tuple>

namespace Rendering
{
    struct Vertex
    {
        Vector3f position{};
        Vector3f normal{};
        Vector2f textureCoordinates{};
        Color4 color = Colors::White;

        Vertex() = default;
        Vertex(Vector3f position, Vector3f normal = {}, Vector2f textureCoordinates = {}, Color4 color = Colors::White)
            : position(position), normal(normal), textureCoordinates(textureCoordinates), color(color) {}
    };

    class Mesh
    {
        ResourceHandle vertexArray = 0;
        ResourceHandle vertexBuffer = 0;
        ResourceHandle indexBuffer = 0;

        std::vector<Vertex> vertices{};
        std::vector<std::uint32_t> indices{};

    public:
        [[nodiscard]] const std::vector<Vertex> &GetVertices() const { return vertices; }
        [[nodiscard]] const std::vector<std::uint32_t> &GetIndices() const { return indices; }
        [[nodiscard]] std::size_t GetVertexCount() const { return vertices.size(); }
        [[nodiscard]] std::size_t GetIndexCount() const { return indices.size(); }
        [[nodiscard]] ResourceHandle GetVertexArray() const { return vertexArray; }
        [[nodiscard]] ResourceHandle GetVertexBuffer() const { return vertexBuffer; }
        [[nodiscard]] ResourceHandle GetIndexBuffer() const { return indexBuffer; }
        [[nodiscard]] bool IsInitialized() const { return vertexArray != 0 && vertexBuffer != 0 && (indices.empty() || indexBuffer != 0); }

        Mesh() = default;
        explicit Mesh(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices = {})
            : vertices(std::move(vertices)), indices(std::move(indices)) {}

        ~Mesh();

        Mesh(const Mesh &) = delete;
        Mesh &operator=(const Mesh &) = delete;

        Mesh(Mesh &&other) noexcept;
        Mesh &operator=(Mesh &&other) noexcept;

        bool InitializeResources(bool reinitialize = false);
        bool Draw() const;

    private:
        void ReleaseResources() noexcept;
    };
}
