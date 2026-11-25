#include "ResourceUtils.h"

#include "StopWatch.h"
#include "LogHandler.h"
#include "ThreadPool.h"

#include "Shared.h"
#include "MeshLoader.h"
#include "IOUtils.h"

#include <stb_image.h>

#include <atomic>
#include <regex>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <cinttypes>
#include <iterator>
#include <tuple>
#include <utility>

namespace // Local helpers
{
    // Driver function to frontload generic parallel resource loading and merging logic for all resource types.
    template <typename TResource, typename TChunkResult, typename TLoadFile, typename TMergeChunk>
    bool LoadResourcesFromFiles_Parallel(const std::filesystem::path &rootDirectory,
        Graphics::LoadedResources<TResource> &outResults, const char *resourceDomain, const std::regex &filePattern,
        const bool outputNonEmpty, TLoadFile &&loadFileFunc, TMergeChunk &&mergeChunkFunc)
    {
        const std::string directory = rootDirectory.string();

        LOG_SCOPE;

        if (!std::filesystem::is_directory(rootDirectory))
        {
            LOG_ERROR("Cannot load %s: directory '%s' does not exist.\n", resourceDomain, directory.c_str());
            return false;
        }

        if (outputNonEmpty)
            LOG_WARNING("Loading %s into a non-empty list.\n", resourceDomain);

        const auto files = IOUtils::FindMatchingFiles_Parallel(rootDirectory, filePattern);
        std::atomic_size_t rejected{0};

        auto tasks = StaticThreadPool().ForEach_Parallel<TChunkResult>(
            [&](const std::filesystem::path &file, std::size_t index, TChunkResult &results)
            {
                if (!loadFileFunc(file, index, results))
                    ++rejected;
            },
            files);

        bool added = false;
        for (auto &task : tasks)
            added |= mergeChunkFunc(task.get());

        outResults.discardedCount += static_cast<UInt32>(rejected.load());
        outResults.foundCount += static_cast<UInt32>(files.size());
        return added;
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
    bool MergeResourceChunks(std::vector<T> &to, std::vector<T> &from,
        std::vector<std::string> &namesTo, std::vector<std::string> &namesFrom)
    {
        if (from.empty())
            return false;

        to.insert(to.end(),
            std::make_move_iterator(from.begin()),
            std::make_move_iterator(from.end()));
        namesTo.insert(namesTo.end(),
            std::make_move_iterator(namesFrom.begin()),
            std::make_move_iterator(namesFrom.end()));
        return true;
    }

    std::string GetRelativeName(const std::filesystem::path &file,
        const std::filesystem::path &rootDirectory, const std::filesystem::path &name)
    {
        return (file.lexically_relative(rootDirectory).parent_path() / name).generic_string();
    }
} // namespace

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
    glBindAttribLocation(shaderProgram, 4, "instance_offset");
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

bool Graphics::CreateGLShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, const std::size_t sourcesCount)
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
            LOG_ERROR("Cannot create shader program: failed to compile at %s shader stage.\n", ShaderTypeToString(static_cast<ShaderType>(sources[i].type)));
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

    ResourceHandle newVertexArray   = 0;
    ResourceHandle newVertexBuffer  = 0;
    ResourceHandle newIndexBuffer   = 0;

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
    const float aspect =        static_cast<float>(textureData.width) / static_cast<float>(textureData.height);
    const UInt32 pixelWidth =   std::roundf(std::sqrt(static_cast<float>(textureData.pixels.size()) / 4.0f * aspect));
    const UInt32 pixelHeight =  std::roundf(static_cast<float>(pixelWidth) / aspect);

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

    glBindTexture       (GL_TEXTURE_2D, outTexture);
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(textureData.wrapMode));
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(textureData.wrapMode));
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(textureData.filterMode));
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(textureData.filterMode));
    glTexImage2D        (GL_TEXTURE_2D, 0, static_cast<GLint>(textureData.format), textureData.width, textureData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.pixels.data());
    glBindTexture       (GL_TEXTURE_2D, 0);

    return true;
}
#pragma endregion

#pragma region Load Resource
bool Graphics::LoadShaderSource(ShaderSource &outShaderSource, std::string &outGroup,
    const std::filesystem::path &file, const std::filesystem::path &rootDirectory)
{
    LOG_SCOPE;

    const std::string fileString = file.string();
    const ShaderType type = GetShaderFileType(file);

    LOG_INFO("Loading shader file '%s'.\n", fileString.c_str());

    if (type == ShaderType::Unknown)
    {
        LOG_SCOPE;
        LOG_WARNING("Skipping shader file with an unrecognized stage: '%s'.\n", fileString.c_str());
        return false;
    }

    std::string sourceCode;
    if (!IOUtils::ReadFileToEnd(fileString.c_str(), sourceCode))
    {
        LOG_SCOPE;
        LOG_WARNING("Failed to read shader file '%s'.\n", fileString.c_str());
        return false;
    }

    const std::filesystem::path stem = file.lexically_relative(rootDirectory).stem(),
        groupName = stem.extension().empty() ? std::filesystem::path("default") : stem.stem();

    outShaderSource = {sourceCode, type};
    outGroup = GetRelativeName(file, rootDirectory, groupName);

    return true;
}

bool Graphics::LoadMeshResource(std::vector<MeshResource> &outMeshes, std::vector<std::string> &outNames,
    const std::filesystem::path &file, const std::filesystem::path &rootDirectory)
{
    const std::string fileString = file.string();

    LOG_SCOPE;
    LOG_INFO("Loading mesh file '%s'.\n", fileString.c_str());

    std::vector<MeshVertices> vertices;
    std::vector<MeshIndices> indices;
    std::vector<std::string> names;

    if (!ReadAndParseModeFile(fileString.c_str(), names, vertices, indices))
    {
        LOG_WARNING("Failed to load one or more mesh objects in file '%s'.\n", fileString.c_str());
        return false;
    }

    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        if (names[i].empty())
            names[i] = GetRelativeName(file, rootDirectory, file.stem());

        outMeshes.emplace_back(std::move(vertices[i]), std::move(indices[i]));
        outNames.emplace_back(std::move(names[i]));
    }
    return vertices.size() > 0;
}

bool Graphics::LoadTexture2DResource(Texture2DResource &outTexture, std::string &outName,
    const std::filesystem::path &file, const std::filesystem::path &rootDirectory)
{
    const std::string fileString = file.string();

    LOG_SCOPE;
    LOG_INFO("Loading texture file '%s'.\n", fileString.c_str());

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

    outTexture = Texture2DResource(width, height, std::move(rgba));
    outName = GetRelativeName(file, rootDirectory, file.stem());
    
    return true;
}
#pragma endregion

#pragma region Load Resource Lists
bool Graphics::LoadShaderResources_Parallel(LoadedResources<ShaderResource> &outResults,
    const std::filesystem::path &rootDirectory)
{
    using ShaderGroupMap = std::map<std::string, std::vector<ShaderSource>>;

    ShaderGroupMap shaderGroups;
    const bool addedNew = LoadResourcesFromFiles_Parallel<ShaderResource, ShaderGroupMap>(
        rootDirectory, outResults, "shader", std::regex(".*\\.glsl", std::regex::icase),
        !outResults.loadedResources.empty(),
        [rootDir = rootDirectory](const std::filesystem::path &file, std::size_t, ShaderGroupMap &results)
        {
            ShaderSource shaderSource;
            std::string shaderGroup;
            if (!LoadShaderSource(shaderSource, shaderGroup, file, rootDir))
                return false;

            results[shaderGroup].push_back(shaderSource);
            return true;
        },
        [&shaderGroups](ShaderGroupMap chunkGroups)
        {
            const bool added = !chunkGroups.empty();
            MergeMaps(chunkGroups, shaderGroups);
            return added;
        });

    for (auto &[name, sources] : shaderGroups)
    {
        LOG_SCOPE;
        LOG_INFO("Found shader program '%s' with %zu %s.\n", name.c_str(), sources.size(), (sources.size() == 1 ? "stage" : "stages"));
        outResults.loadedResources.emplace_back(std::move(sources));
        outResults.resourceNames.emplace_back(std::move(name));
    }

    return addedNew;
}

bool Graphics::LoadMeshResources_Parallel(LoadedResources<MeshResource> &outResults,
    const std::filesystem::path &rootDirectory)
{
    using MeshChunkResult = std::tuple<std::vector<MeshResource>, std::vector<std::string>>;

    return LoadResourcesFromFiles_Parallel<MeshResource, MeshChunkResult>(
        rootDirectory, outResults, "model", std::regex(".*\\.obj", std::regex::icase),
        !outResults.loadedResources.empty(),
        [rootDir = rootDirectory](const std::filesystem::path &file, std::size_t,
            MeshChunkResult &results)
        {
            return LoadMeshResource(std::get<0>(results),
                std::get<1>(results), file, rootDir);
        },
        [&outResults](MeshChunkResult results)
        {
            auto [meshes, names] = std::move(results);
            return MergeResourceChunks(outResults.loadedResources, meshes, outResults.resourceNames, names);
        });
}

bool Graphics::LoadTextureResources_Parallel(LoadedResources<Texture2DResource> &outResults,
    const std::filesystem::path &rootDirectory)
{
    using TextureChunkResult = std::tuple<std::vector<Texture2DResource>, std::vector<std::string>>;

    return LoadResourcesFromFiles_Parallel<Texture2DResource, TextureChunkResult>(
        rootDirectory, outResults, "texture", std::regex(".*\\.(png|jpg|jpeg)", std::regex::icase),
        !outResults.loadedResources.empty(),
        [rootDir = rootDirectory](const std::filesystem::path &file, std::size_t, TextureChunkResult &results)
        {
            std::string name;
            Texture2DResource texture;
            if (!LoadTexture2DResource(texture, name, file, rootDir))
                return false;

            std::get<0>(results).push_back(std::move(texture));
            std::get<1>(results).push_back(std::move(name));
            return true;
        },
        [&outResults](TextureChunkResult results)
        {
            auto [textures, names] = std::move(results);
            return MergeResourceChunks(outResults.loadedResources, textures, outResults.resourceNames, names);
        });
}
#pragma endregion

bool Graphics::LoadResources_Parallel(LoadedResources<Graphics::ShaderResource> &outShaders,
    LoadedResources<Graphics::MeshResource> &outMeshes, LoadedResources<Graphics::Texture2DResource> &outTextures,
    const std::filesystem::path &shaderDirectory, const std::filesystem::path &meshDirectory,
    const std::filesystem::path &textureDirectory)
{
    auto shaderTask = StaticThreadPool().SubmitTask(
        [&outShaders, &shaderDirectory]() { return LoadShaderResources_Parallel(outShaders, shaderDirectory); });
    auto meshTask = StaticThreadPool().SubmitTask(
        [&outMeshes, &meshDirectory]() { return LoadMeshResources_Parallel(outMeshes, meshDirectory); });
    auto textureTask = StaticThreadPool().SubmitTask(
        [&outTextures, &textureDirectory]() { return LoadTextureResources_Parallel(outTextures, textureDirectory); });

    bool loadSuccess = shaderTask.get();
    loadSuccess &= meshTask.get();
    loadSuccess &= textureTask.get();

    return loadSuccess;
}

bool Graphics::LoadResources(LoadedResources<Graphics::ShaderResource> &outShaders,
    LoadedResources<Graphics::MeshResource> &outMeshes, LoadedResources<Graphics::Texture2DResource> &outTextures,
    const std::filesystem::path &rootDirectory, const ThreadCount threadCount)
{
    StopWatch<std::chrono::steady_clock> stopWatch;

    const std::filesystem::path shaderDirectory = rootDirectory / "shaders";
    const std::filesystem::path meshDirectory = rootDirectory / "models";
    const std::filesystem::path textureDirectory = rootDirectory / "textures";

    bool loadSuccess;
    if (threadCount > 1)
        loadSuccess = LoadResources_Parallel(
            outShaders, outMeshes, outTextures, shaderDirectory, meshDirectory, textureDirectory);
    else
    {
        const bool shaderLoadSuccess = LoadShaderResources_Parallel(outShaders, shaderDirectory);
        const bool meshLoadSuccess = LoadMeshResources_Parallel(outMeshes, meshDirectory);
        const bool textureLoadSuccess = LoadTextureResources_Parallel(outTextures, textureDirectory);
        loadSuccess = shaderLoadSuccess && meshLoadSuccess && textureLoadSuccess;
    }

    LOG_INFO("Loading resources took %f seconds.\n", TimeDuration(stopWatch.CurrentDuration()).count());

    if (outShaders.discardedCount > 0)
        LOG_WARNING("%u of %u shader source files failed to load and were discarded.\n", outShaders.discardedCount,
            outShaders.foundCount);
    if (outMeshes.discardedCount > 0)
        LOG_WARNING("%u of %u model files failed to load and were discarded.\n", outMeshes.discardedCount,
            outMeshes.foundCount);
    if (outTextures.discardedCount > 0)
        LOG_WARNING("%u of %u texture files failed to load and were discarded.\n", outTextures.discardedCount,
            outTextures.foundCount);

    return loadSuccess;
}

bool Graphics::LoadAndInitResources(LoadedResources<ShaderResource> &outShaders,
    LoadedResources<MeshResource> &outMeshes, LoadedResources<Texture2DResource> &outTextures,
    const std::filesystem::path &rootDirectory)
{
    LOG_SCOPE;
    LOG_INFO("Loading assets from '%s'.\n", rootDirectory.string().c_str());

    using namespace Graphics;

    {
        LOG_SCOPE;

        if (!LoadResources(outShaders, outMeshes, outTextures, rootDirectory))
        {
            LOG_ERROR("Failed to load resources.\n");
            return false;
        }

        StopWatch<std::chrono::steady_clock> stopWatch;
        stopWatch.Reset();
        
        const bool shadersInitSuccess = InitializeResources(outShaders);
        const bool meshesInitSuccess = InitializeResources(outMeshes);
        const bool texturesInitSuccess = InitializeResources(outTextures);
        LOG_INFO("Initializing resources took %f seconds.\n", TimeDuration(stopWatch.CurrentDuration()).count());
        
        if (outShaders.loadedResources.empty())
            LOG_ERROR("No initialized shader resources are available.\n");
        if (outShaders.discardedCount > 0)
            LOG_WARNING("%u of %u shader resources failed to initialize and were discarded.\n",
                outShaders.discardedCount, outShaders.foundCount);

        if (outMeshes.loadedResources.empty())
            LOG_WARNING("No initialized mesh resources are available.\n");
        if (outMeshes.discardedCount > 0)
            LOG_WARNING("%u of %u mesh resources failed to initialize and were discarded.\n",
                outMeshes.discardedCount, outMeshes.foundCount);

        if (outTextures.loadedResources.empty())
            LOG_ERROR("No initialized texture resources are available.\n");
        if (outTextures.discardedCount > 0)
            LOG_WARNING("%u of %u texture resources failed to initialize and were discarded.\n",
                outTextures.discardedCount, outTextures.foundCount);
    }

    return !outShaders.loadedResources.empty() && !outTextures.loadedResources.empty() &&
        !outMeshes.loadedResources.empty();
}