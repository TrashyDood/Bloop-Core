#include "ResourceLoading.h"

#include "LogHandler.h"
#include "ThreadPool.h"

#include "Shared.h"
#include "Graphics.h"
#include "MeshLoader.h"
#include "IOUtils.h"

#include <stb_image.h>

#include <atomic>
#include <regex>
#include <map>
#include <vector>
#include <string>
#include <cinttypes>
#include <iterator>
#include <tuple>
#include <utility>

namespace // Local helpers
{
    // Driver function to frontload generic parallel resource loading and merging logic for all resource types.
    template <typename TResource, typename TChunkResult, typename TLoadFile, typename TMergeChunk>
    void LoadResourcesFromFiles_Parallel(const IO::Path &rootDirectory, Graphics::LoadedResources<TResource> &outResults,
        const char *resourceDomain, const std::regex &filePattern, TLoadFile &&loadFileFunc, TMergeChunk &&mergeChunkFunc)
    {
        LOG_SCOPE;
        const std::string directory = rootDirectory.absolutePath.string();

        if (!std::filesystem::is_directory(rootDirectory.absolutePath))
        {
            LOG_ERROR("Cannot load %s: directory '%s' does not exist.\n", resourceDomain, directory.c_str());
            return;
        }

        if (!outResults.IsEmpty())
            LOG_WARNING("Loading new resources into a non-empty %ss resource list.\n", resourceDomain);

        const auto files = IO::FindMatchingFiles_Parallel(rootDirectory.absolutePath, filePattern);
        std::atomic_size_t failedFileCount{0}, loadedResourceCount{0}, failedResourceCount{0};

        auto tasks = StaticThreadPool().For_Parallel(files.size(),
            [&](uint16_t, const std::size_t start, const std::size_t end)
            {
                LOG_SCOPE;

                TChunkResult results{};
                for (std::size_t index = start; index < end; ++index)
                {
                    const IO::Path resourcePath = IO::Path::FromAbsolute(files[index], rootDirectory.absolutePath);
                    UInt32 fileLoadedCount = 0, fileFailedCount = 0;
                    loadFileFunc(resourcePath, results, fileLoadedCount, fileFailedCount);

                    loadedResourceCount += fileLoadedCount;
                    failedResourceCount += fileFailedCount;

                    if (fileLoadedCount > 0)
                    {
                        LOG_INFO("Successfully loaded %zu %s %s from file '%s'.\n",
                            fileLoadedCount,
                            resourceDomain,
                            fileLoadedCount > 1 ? "resources" : "resource",
                            resourcePath.absolutePath.string().c_str());
                    }
                    else
                        ++failedFileCount;
                }
                return results;
            });

        for (auto &task : tasks)
            mergeChunkFunc(task.Get());

        const UInt32 loadedCount = static_cast<UInt32>(loadedResourceCount.load());

        outResults.filesFound += static_cast<UInt32>(files.size());
        outResults.filesFailed += static_cast<UInt32>(failedFileCount.load());
        outResults.resourcesFailed += static_cast<UInt32>(failedResourceCount.load());
        outResults.resourcesLoaded += loadedCount;
    }

    template <typename TKey, typename TValue>
    void MergeMaps(std::map<TKey, TValue> &mapA, std::map<TKey, TValue> &mapB)
    {
        for (auto &[keyA, valueA] : mapA)
        {
            auto [keyB, valueB] = mapB.try_emplace(keyA, std::move(valueA));
            if (!valueB)
                keyB->second.insert(
                    keyB->second.end(),
                    std::make_move_iterator(valueA.begin()),
                    std::make_move_iterator(valueA.end()));
        }
    }

    template <typename T>
    void MergeChunks(std::vector<T> &to, std::vector<T> &from,
        std::vector<std::string> &namesTo, std::vector<std::string> &namesFrom)
    {
        if (from.empty())
            return;

        to.insert(to.end(),
            std::make_move_iterator(from.begin()),
            std::make_move_iterator(from.end()));
        namesTo.insert(namesTo.end(),
            std::make_move_iterator(namesFrom.begin()),
            std::make_move_iterator(namesFrom.end()));
    }

    std::string GetRelativeName(const IO::Path &file, const std::filesystem::path &name)
    {
        return (file.relativePath.parent_path() / name).generic_string();
    }

    std::string GetShaderGroupName(const std::filesystem::path &file)
    {
        const std::filesystem::path stem = file.stem();
        return stem.extension().empty() ? "default" : stem.stem().string();
    }

    Graphics::ShaderType GetShaderFileType(const std::filesystem::path &file)
    {
        using namespace Graphics;
        const ShaderType stageType =
            ShaderTypeFromString(file.stem().extension().string());

        if (stageType != ShaderType::Unknown)
            return stageType;

        return ShaderTypeFromString(
            file.stem().string());
    }
} // namespace

#pragma region Load Singular Resource
bool Graphics::LoadShaderResourceFile(ShaderSource &outShaderSource, std::string &outGroup,
    const IO::Path &file)
{
    LOG_SCOPE;
    const std::string fileString = file.absolutePath.string();
    const ShaderType type = GetShaderFileType(file.absolutePath);

    if (type == ShaderType::Unknown)
    {
        LOG_WARNING("Failed to load shader file with an unrecognized stage: '%s'.\n", fileString.c_str());
        return false;
    }

    std::string sourceCode;
    if (!IO::ReadFileToEnd(fileString.c_str(), sourceCode))
    {
        LOG_WARNING("Failed to read shader file '%s'.\n", fileString.c_str());
        return false;
    }

    const std::filesystem::path stem = file.relativePath.stem(),
        groupName = stem.extension().empty() ? std::filesystem::path("default") : stem.stem();

    outShaderSource = {sourceCode, type};
    outGroup = GetRelativeName(file, groupName);

    return true;
}

void Graphics::LoadMeshFile(std::vector<Mesh> &outMeshes, std::vector<std::string> &outNames,
    const IO::Path &file, UInt32 &outSucceededCount, UInt32 &outFailedCount)
{
    LOG_SCOPE;
    const std::string fileString = file.absolutePath.string();

    std::vector<MeshVertices> vertices;
    std::vector<MeshIndices> indices;
    std::vector<std::string> names;

    ReadAndParseModeFile(fileString.c_str(), names, vertices, indices, outSucceededCount, outFailedCount);
    if (outFailedCount > 0)
        LOG_WARNING("%u mesh objects failed to load from '%s'.\n", outFailedCount, fileString.c_str());

    for (std::size_t i = 0; i < outSucceededCount; ++i)
    {
        if (names[i].empty())
            names[i] = GetRelativeName(file, file.relativePath.stem());

        outMeshes.emplace_back(std::move(vertices[i]), std::move(indices[i]));
        outNames.emplace_back(std::move(names[i]));
    }
}

bool Graphics::LoadTexture2DResourceFile(Texture2D &outTexture, std::string &outName,
    const IO::Path &file)
{
    LOG_SCOPE;
    const std::string fileString = file.absolutePath.string();

    int width = 0, height = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> image(
        stbi_load(fileString.c_str(), &width, &height, nullptr, STBI_rgb_alpha), stbi_image_free);

    if (!image)
    {
        LOG_WARNING("Failed to load texture file '%s': '%s'.\n", fileString.c_str(), stbi_failure_reason());
        return false;
    }

    const auto byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    std::vector<UInt8> rgba(image.get(), image.get() + byteCount);

    outTexture = Texture2D(width, height, std::move(rgba));
    outName = GetRelativeName(file, file.relativePath.stem());
    
    return true;
}
#pragma endregion

#pragma region Load Several Resources
void Graphics::LoadShaderResources_Parallel(LoadedResources<Shader> &outResults,
    const IO::Path &rootDirectory)
{
    LOG_SCOPE;

    using ShaderGroupMap = std::map<std::string, std::vector<ShaderSource>>;

    const std::size_t loadedResourceStart = outResults.loadedResources.size();
    ShaderGroupMap shaderGroups;
    LoadResourcesFromFiles_Parallel<Shader, ShaderGroupMap>(
        rootDirectory, outResults, "shader", std::regex(".*\\.glsl", std::regex::icase),
        [](const IO::Path &file, ShaderGroupMap &results, UInt32 &loadedCount, UInt32 &failedCount)
        {
            ShaderSource shaderSource;
            std::string shaderGroup;
            if (!LoadShaderResourceFile(shaderSource, shaderGroup, file))
            {
                failedCount = 1;
                return;
            }

            results[shaderGroup].push_back(shaderSource);
            loadedCount = 1;
        },
        [&shaderGroups](ShaderGroupMap chunkGroups)
        {
            MergeMaps(chunkGroups, shaderGroups);
        });

    for (auto &[name, sources] : shaderGroups)
    {
        LOG_SCOPE;
        LOG_INFO("Found shader program '%s' with %zu %s.\n", name.c_str(),
        sources.size(), (sources.size() == 1 ? "stage" : "stages"));

        outResults.loadedResources.emplace_back(std::move(sources));
        outResults.resourceNames.emplace_back(std::move(name));
    }
    outResults.uninitializedCount +=
        static_cast<UInt32>(outResults.loadedResources.size() - loadedResourceStart);
}

void Graphics::LoadMeshResources_Parallel(LoadedResources<Mesh> &outResults,
    const IO::Path &rootDirectory)
{
    LOG_SCOPE;

    using ChunkResult = std::tuple<std::vector<Mesh>, std::vector<std::string>>;
    const std::size_t loadedResourceStart = outResults.loadedResources.size();
    LoadResourcesFromFiles_Parallel<Mesh, ChunkResult>(
        rootDirectory, outResults, "model", std::regex(".*\\.obj", std::regex::icase),
        [](const IO::Path &file, ChunkResult &results, UInt32 &loadedCount, UInt32 &failedCount)
        {
            LoadMeshFile(std::get<0>(results), std::get<1>(results), file, loadedCount, failedCount);
        },
        [&outResults](ChunkResult results)
        {
            auto [meshes, names] = std::move(results);
            MergeChunks(outResults.loadedResources, meshes, outResults.resourceNames, names);
        });
    outResults.uninitializedCount +=
        static_cast<UInt32>(outResults.loadedResources.size() - loadedResourceStart);
}

void Graphics::LoadTextureResources_Parallel(LoadedResources<Texture2D> &outResults, const IO::Path &rootDirectory)
{
    LOG_SCOPE;

    using ChunkResult = std::tuple<std::vector<Texture2D>, std::vector<std::string>>;
    const std::size_t loadedResourceStart = outResults.loadedResources.size();
    LoadResourcesFromFiles_Parallel<Texture2D, ChunkResult>(
        rootDirectory, outResults, "texture", std::regex(".*\\.(png|jpg|jpeg)", std::regex::icase),
        [](const IO::Path &file, ChunkResult &results, UInt32 &loadedCount, UInt32 &failedCount)
        {
            std::string name;
            Texture2D texture;
            if (!LoadTexture2DResourceFile(texture, name, file))
            {
                failedCount = 1;
                return;
            }

            std::get<0>(results).push_back(std::move(texture));
            std::get<1>(results).push_back(std::move(name));
            loadedCount = 1;
        },
        [&outResults](ChunkResult results)
        {
            auto [textures, names] = std::move(results);
            MergeChunks(outResults.loadedResources, textures, outResults.resourceNames, names);
        });
    outResults.uninitializedCount +=
        static_cast<UInt32>(outResults.loadedResources.size() - loadedResourceStart);
}
#pragma endregion
