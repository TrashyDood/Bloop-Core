#pragma once

#include "MeshResource.h"
#include "ShaderResource.h"
#include "Shared.h"
#include "Texture2DResource.h"

#include <filesystem>
#include <vector>

namespace Graphics
{
    template<typename T>
    struct LoadedResources
    {
        std::vector<T> &loadedResources;
        std::vector<std::string> &resourceNames;
        UInt32 foundCount = 0,
            discardedCount = 0;
        bool areInitialized = false;
    };

    bool CompileGLShader(ResourceHandle &outShader, const ShaderType type, const char *source);
    bool LinkGLShaders(ResourceHandle &outShaderProgram, const ResourceHandle *shaders, const std::size_t shadersCount);
    bool CreateGLShaderProgram(
        ResourceHandle &outShaderProgram, const ShaderSource *sources, const std::size_t sourcesCount);
    bool CreateGLMeshBuffers(ResourceHandle &vertexArray, ResourceHandle &vertexBuffer, ResourceHandle &indexBuffer,
        const MeshVertices &vertices, const MeshIndices &indices = {});
    bool CreateGLTexture2D(ResourceHandle &outTexture, const TextureData &textureData);

    inline bool LoadShaderSource(Graphics::ShaderSource &outShaderSource, std::string &outGroup,
        const std::filesystem::path &file, const std::filesystem::path &rootDirectory);
    bool LoadMeshResource(std::vector<Graphics::MeshResource> &outMeshes, std::vector<std::string> &outNames,
        const std::filesystem::path &file, const std::filesystem::path &rootDirectory);
    bool LoadTexture2DResource(Graphics::Texture2DResource &outTexture, std::string &outName,
        const std::filesystem::path &file, const std::filesystem::path &rootDirectory);

    bool LoadShaderResources_Parallel(LoadedResources<Graphics::ShaderResource> &outResults,
        const std::filesystem::path &directory);
    bool LoadMeshResources_Parallel(LoadedResources<Graphics::MeshResource> &outResults,
        const std::filesystem::path &directory);
    bool LoadTextureResources_Parallel(LoadedResources<Graphics::Texture2DResource> &outResults,
        const std::filesystem::path &directory);

    bool LoadResources_Parallel(LoadedResources<Graphics::ShaderResource> &outShaders,
        LoadedResources<Graphics::MeshResource> &outMeshes, LoadedResources<Graphics::Texture2DResource> &outTextures,
        const std::filesystem::path &shaderDirectory, const std::filesystem::path &meshDirectory,
        const std::filesystem::path &textureDirectory);
    bool LoadResources(LoadedResources<Graphics::ShaderResource> &outShaders,
        LoadedResources<Graphics::MeshResource> &outMeshes, LoadedResources<Graphics::Texture2DResource> &outTextures,
        const std::filesystem::path &rootDirectory, const ThreadCount threadCount = 1);
    bool LoadAndInitResources(LoadedResources<Graphics::ShaderResource> &outShaders,
        LoadedResources<Graphics::MeshResource> &outMeshes, LoadedResources<Graphics::Texture2DResource> &outTextures,
        const std::filesystem::path &rootDirectory);

    template <typename T>
    bool InitializeResources(LoadedResources<T> &outResults, bool eraseOnFail = true)
    {
        static_assert(std::is_base_of_v<ResourceOwner, T>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        auto &resources = outResults.loadedResources;
        auto &names = outResults.resourceNames;
        outResults.foundCount = static_cast<UInt32>(resources.size());
        outResults.discardedCount = 0;

        if (resources.size() != names.size())
            LOG_WARNING("The number of target resources does not equal the number target of names. \n");

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

            outResults.discardedCount++;
            resources.erase(resources.begin() + resourceIndex);
            if (resourceIndex < names.size())
                names.erase(names.begin() + resourceIndex);
        }

        outResults.areInitialized = true;
        return outResults.discardedCount < 1;
    }
} // namespace Graphics