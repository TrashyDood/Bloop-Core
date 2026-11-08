#pragma once

#include "Shared/Shared.h"
#include "Rendering/IResourceOwner.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
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

    // Represents the geometry of a mesh, including its vertices and indices.
    struct MeshGeometry
    {
        std::vector<Vertex> vertices{};
        std::vector<std::uint32_t> indices{};
    };

    // Represents a mesh in OpenGL, managing its vertex array, vertex buffer, and index buffer.
    class Mesh : public IResourceOwner
    {
        ResourceHandle vertexArray = 0;
        ResourceHandle vertexBuffer = 0;
        ResourceHandle indexBuffer = 0;

        MeshGeometry geometry{};

    public:
        [[nodiscard]] const MeshGeometry &GetGeometry() const { return geometry; }
        [[nodiscard]] const std::vector<Vertex> &GetVertices() const { return geometry.vertices; }
        [[nodiscard]] const std::vector<std::uint32_t> &GetIndices() const { return geometry.indices; }
        [[nodiscard]] std::size_t GetVertexCount() const { return geometry.vertices.size(); }
        [[nodiscard]] std::size_t GetIndexCount() const { return geometry.indices.size(); }
        [[nodiscard]] ResourceHandle GetVertexArray() const { return vertexArray; }
        [[nodiscard]] ResourceHandle GetVertexBuffer() const { return vertexBuffer; }
        [[nodiscard]] ResourceHandle GetIndexBuffer() const { return indexBuffer; }
        [[nodiscard]] bool IsInitialized() const { return vertexArray != 0 && vertexBuffer != 0 && (geometry.indices.empty() || indexBuffer != 0); }

        Mesh() = default;
        explicit Mesh(MeshGeometry geometry) : geometry(std::move(geometry)) {}

        ~Mesh();

        Mesh(const Mesh &) = delete;
        Mesh &operator=(const Mesh &) = delete;

        Mesh(Mesh &&other) noexcept;
        Mesh &operator=(Mesh &&other) noexcept;

        virtual bool InitializeResources() override;
        bool Draw() const;

    private:
        virtual void ReleaseResources() noexcept override;
    };
}
