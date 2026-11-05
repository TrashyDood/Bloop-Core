#pragma once

#include "Shared/Core.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

struct Vertex
{
    Vector3f position{};
    Vector3f normal{};
    Vector2f textureCoordinates{};
    Rgba color{};
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    ResourceHandle vertexArrayHandle = 0;
    ResourceHandle vertexBufferHandle = 0;
    ResourceHandle indexBufferHandle = 0;

    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;

    bool Initialize();

private:
    void Release() noexcept;
};
