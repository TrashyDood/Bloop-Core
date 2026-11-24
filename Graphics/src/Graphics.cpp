#include "Graphics.h"
#include "LogHandler.h"

#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#pragma region Create GL Handles
bool Graphics::CompileGLShader(ResourceHandle &outShader, const ShaderType type, const char *source)
{
    LOG_SCOPE;

    if (outShader != 0)
    {
        LOG_ERROR("Cannot compile shader: output handle is already initialized.\n");
        return false;
    }

    if (source == nullptr)
    {
        LOG_ERROR("Cannot compile shader: source is null.\n");
        return false;
    }

    outShader = glCreateShader(static_cast<GLuint>(type));
    if (outShader == 0)
    {
        LOG_ERROR("Cannot compile shader: failed to allocate shader.\n");
        return false;
    }

    glShaderSource(outShader, 1, &source, nullptr);
    glCompileShader(outShader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE)
    {
        char log[512]{};
        glGetShaderInfoLog(outShader, sizeof(log), nullptr, log);
        LOG_ERROR("Shader compiler error: %s\n", log);
        glDeleteShader(outShader);
        outShader = 0;
    }

    return compiled == GL_TRUE;
}

bool Graphics::LinkGLShaders(ResourceHandle &shaderProgram, const ResourceHandle *shaders, const std::size_t shadersCount)
{
    LOG_SCOPE;

    if (shaderProgram != 0)
    {
        LOG_ERROR("Cannot link shader program: output handle is already initialized.\n");
        return false;
    }

    if (shaders == nullptr || shadersCount == 0)
    {
        LOG_ERROR("Cannot link shader program: no shader handles provided.\n");
        return false;
    }

    for (std::size_t i = 0; i < shadersCount; ++i)
        if (shaders[i] == 0)
        {
            LOG_ERROR("Cannot link shader program: shader handle at index %zu is invalid.\n", i);
            return false;
        }

    shaderProgram = glCreateProgram();
    if (shaderProgram == 0)
    {
        LOG_ERROR("Cannot link shader program: failed to allocate program.\n");
        return false;
    }

    for (std::size_t i = 0; i < shadersCount; ++i)
        glAttachShader(shaderProgram, shaders[i]);

    glBindAttribLocation(shaderProgram, 0, "vertex_position");
    glBindAttribLocation(shaderProgram, 1, "vertex_normal");
    glBindAttribLocation(shaderProgram, 2, "vertex_uv");
    glBindAttribLocation(shaderProgram, 3, "vertex_color");
    glLinkProgram(shaderProgram);

    GLint linked = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);

    for (std::size_t i = 0; i < shadersCount; ++i)
        glDetachShader(shaderProgram, shaders[i]);

    if (linked == GL_FALSE)
    {
        char log[512]{};
        glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
        LOG_ERROR("Shader linker output: %s\n", log);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    return linked == GL_TRUE;
}

bool Graphics::CreateGLShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources,
    const std::size_t sourcesCount)
{
    LOG_SCOPE;

    if (outShaderProgram != 0)
    {
        LOG_ERROR("Cannot create shader program: output handle is already initialized.\n");
        return false;
    }

    if (sources == nullptr || sourcesCount == 0)
    {
        LOG_ERROR("Cannot create shader program: no shader sources provided.\n");
        return false;
    }

    std::vector<ResourceHandle> shaders(sourcesCount, 0);
    std::vector<std::string> loadedSources(sourcesCount);

    for (std::size_t i = 0; i < sourcesCount; ++i)
    {
        const std::string *source = &sources[i].source;
        if (!CompileGLShader(shaders[i], sources[i].type, source->c_str()))
        {
            LOG_ERROR("Cannot create shader program: failed to compile at %s shader stage.\n",
                ShaderTypeToString(static_cast<ShaderType>(sources[i].type)));
            for (const ResourceHandle shader : shaders)
                if (shader != 0)
                    glDeleteShader(shader);
            return false;
        }
    }

    const bool linked = LinkGLShaders(outShaderProgram, shaders.data(), shaders.size());

    if (!linked)
        LOG_ERROR("Cannot create shader program: failed to link shaders.\n");

    for (const ResourceHandle shader : shaders) // Cleanup
        glDeleteShader(shader);

    return linked;
}

bool Graphics::CreateGLMeshBuffers(ResourceHandle &outVertexArray, ResourceHandle &outVertexBuffer,
    ResourceHandle &outIndexBuffer, const MeshVertices &vertices, const MeshIndices &indices)
{
    LOG_SCOPE;

    if (outVertexArray != 0 || outVertexBuffer != 0 || outIndexBuffer != 0)
    {
        LOG_ERROR("Cannot create mesh buffers: output handles are already initialized.\n");
        return false;
    }

    outVertexArray = 0;
    outVertexBuffer = 0;
    outIndexBuffer = 0;

    if (vertices.size() > MaxMeshBufferSize / sizeof(Vertex) ||
        indices.size() > MaxMeshBufferSize / sizeof(UInt32) ||
        sizeof(Vertex) > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
    {
        LOG_ERROR("Cannot create mesh buffers: vertex data, index data, or vertex stride exceeds supported limits.\n");
        return false;
    }

    for (std::size_t i = 0; i < indices.size(); ++i)
        if (indices[i] >= vertices.size())
        {
            LOG_ERROR("Cannot create mesh buffers: index %" PRIu32 " at position %zu is outside the vertex range.\n",
                indices[i], i);
            return false;
        }

    ResourceHandle newVertexArray = 0;
    ResourceHandle newVertexBuffer = 0;
    ResourceHandle newIndexBuffer = 0;

    if (!indices.empty())
        glGenBuffers(1, &newIndexBuffer);

    glGenVertexArrays(1, &newVertexArray);
    glGenBuffers(1, &newVertexBuffer);

    if (newVertexArray == 0 || newVertexBuffer == 0 || (!indices.empty() && newIndexBuffer == 0))
    {
        LOG_ERROR("Cannot create mesh buffers: failed to allocate the required vertex array or buffer objects.\n");
        if (newVertexArray != 0)
            glDeleteVertexArrays(1, &newVertexArray);
        if (newIndexBuffer != 0)
            glDeleteBuffers(1, &newIndexBuffer);
        if (newVertexBuffer != 0)
            glDeleteBuffers(1, &newVertexBuffer);
        return false;
    }

    glBindVertexArray(newVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, newVertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(UInt32)),
            indices.data(), GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void *>(offsetof(Vertex, UV)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void *>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    outVertexArray = newVertexArray;
    outVertexBuffer = newVertexBuffer;
    outIndexBuffer = newIndexBuffer;
    return true;
}

bool Graphics::CreateGLTexture2D(ResourceHandle &outTexture, const TextureData &textureData)
{
    LOG_SCOPE;

    if (outTexture != 0)
    {
        LOG_ERROR("Cannot create texture: output texture handle is already initialized.\n");
        return false;
    }

    if (textureData.pixels.empty())
    {
        LOG_ERROR("Cannot create texture: pixel data is empty.\n");
        return false;
    }

    // Calculate the pixel width and height based on the aspect ratio and pixel count
    const float aspect = static_cast<float>(textureData.width) / static_cast<float>(textureData.height);
    const UInt32 pixelWidth =
        std::roundf(std::sqrt(static_cast<float>(textureData.pixels.size()) / 4.0f * aspect));
    const UInt32 pixelHeight = std::roundf(static_cast<float>(pixelWidth) / aspect);

    if (aspect <= 0 || pixelWidth < 1 || pixelHeight < 1)
    {
        LOG_ERROR("Cannot create texture: invalid dimensions.\n");
        return false;
    }

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (pixelWidth > maxTextureSize || pixelHeight > maxTextureSize)
    {
        LOG_ERROR("Cannot create texture: dimensions exceed the texture size limit.\n");
        return false;
    }

    glGenTextures(1, &outTexture);
    if (outTexture == 0)
    {
        LOG_ERROR("Cannot create texture: failed to allocate texture.\n");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, outTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(textureData.wrapMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(textureData.wrapMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(textureData.filterMode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(textureData.filterMode));
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(textureData.format), textureData.width, textureData.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, textureData.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}
#pragma endregion

void Graphics::CheckGLErrors()
{
    LOG_SCOPE;

    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        LOG_ERROR("OpenGL error: 0x%X\n", error);
}


Matrix4f Graphics::GetMatrix_Perspective(const float fieldOfView,
    const float aspectRatio, const float nearPlane, const float farPlane)
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

Matrix4f Graphics::GetMatrix_Orthographic(const float orthographicSize,
    const float aspectRatio, const float nearPlane, const float farPlane)
{
    const float halfHeight = orthographicSize * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}