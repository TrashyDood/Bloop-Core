#pragma once

#include "MeshResource.h"
#include "ShaderResource.h"
#include "Shared.h"
#include "Texture2DResource.h"

#include <filesystem>
#include <vector>

namespace Graphics
{
    bool CompileShader(ResourceHandle &outShader, const ShaderType type, const char *source);
    bool LinkShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, const std::size_t shadersCount);
    bool CreateShaderProgram(
        ResourceHandle &outShaderProgram, const ShaderSource *sources, const std::size_t sourcesCount);
    bool CreateMeshBuffers(ResourceHandle &vertexArray, ResourceHandle &vertexBuffer, ResourceHandle &indexBuffer,
        const MeshVertices &vertices, const MeshIndices &indices = {});
    bool CreateTexture2D(ResourceHandle &outTexture, const TextureData &textureData);

    bool LoadShaderResources(std::vector<Graphics::ShaderResource> &outShaders, std::vector<std::string> &outNames,
        const std::filesystem::path &directory);
    bool LoadMeshResources(std::vector<Graphics::MeshResource> &outMeshes, std::vector<std::string> &outNames,
        const std::filesystem::path &directory);
    bool LoadTextureResources(std::vector<Graphics::Texture2DResource> &outTextures, std::vector<std::string> &outNames,
        const std::filesystem::path &directory);

bool LoadAndInitResources(std::vector<Graphics::ShaderResource> &outShaders,
    std::vector<Graphics::MeshResource> &outMeshes, std::vector<Graphics::Texture2DResource> &outTextures,
    std::vector<std::string> &outShaderNames, std::vector<std::string> &outMeshNames,
    std::vector<std::string> &outTextureNames, const std::filesystem::path &rootDirectory);

    template <typename T>
    bool InitializeResources(std::vector<T> &resources, std::vector<std::string> &names, std::size_t &outDiscardedCount, bool eraseOnFail = true)
    {
        static_assert(std::is_base_of_v<ResourceOwner, T>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        if (resources.size() != names.size())
            LOG_WARNING("The number of resources does not equal the number of names. \n");

        for (std::size_t resourceIndex = 0; resourceIndex < resources.size();)
        {
            if (resources[resourceIndex].InitializeResources())
            {
                ++resourceIndex;
                continue;
            }

            static_cast<ResourceOwner &>(resources[resourceIndex]).ReleaseResources();

            if (!eraseOnFail)
            {
                ++resourceIndex;
                continue;
            }

            outDiscardedCount++;
            resources.erase(resources.begin() + resourceIndex);
            if (resourceIndex < names.size())
                names.erase(names.begin() + resourceIndex);
        }

        return outDiscardedCount < 1;
    }
} // namespace Graphics