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
#include <cinttypes>
#include <iterator>
#include <tuple>
#include <utility>

namespace
{
    template <typename TChunkResult, typename TLoadFile, typename TMergeChunk>
    bool LoadResourcesFromFiles_Parallel(const std::filesystem::path &rootDirectory, const char *resourceNames,
        const char *directoryName, const std::regex &filePattern, const bool outputNonEmpty, TLoadFile &&loadFile,
        TMergeChunk &&mergeChunk)
    {
        const std::string directoryString = rootDirectory.string();

        LOG_SCOPE;
        LOG_INFO("Loading %s from '%s'.\n", resourceNames, directoryString.c_str());

        if (!std::filesystem::is_directory(rootDirectory))
        {
            LOG_SCOPE;
            LOG_ERROR("%s directory '%s' does not exist.\n", directoryName, directoryString.c_str());
            return false;
        }

        if (outputNonEmpty)
        {
            LOG_SCOPE;
            LOG_WARNING("Loading %s into a non-empty list.\n", resourceNames);
        }

        const auto files = IOUtils::FindMatchingFiles_Parallel(rootDirectory, filePattern);
        std::atomic_size_t rejectedFileCount{0};
        auto processFile = [&loadFile, &rejectedFileCount](
            const std::filesystem::path &file, const std::size_t index, TChunkResult &results)
        {
            if (!loadFile(file, index, results))
                rejectedFileCount.fetch_add(1, std::memory_order_relaxed);
        };
        auto tasks = StaticThreadPool().ForEach_Parallel<16, TChunkResult>(files, std::move(processFile));

        bool addedResources = false;
        for (auto &task : tasks)
            if (mergeChunk(task.get()))
                addedResources = true;

        LOG_INFO("Rejected %zu of %zu files while loading %s.\n",
            rejectedFileCount.load(std::memory_order_relaxed), files.size(), resourceNames);

        return addedResources;
    }
} // namespace

bool Graphics::CompileShader(ResourceHandle &outShader, const ShaderType type, const char *source)
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

bool Graphics::LinkShaders(ResourceHandle &shaderProgram, const ResourceHandle *shaders, const std::size_t shadersCount)
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

bool Graphics::CreateShaderProgram(ResourceHandle &outShaderProgram, const ShaderSource *sources, const std::size_t sourcesCount)
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
        if (!CompileShader(shaders[i], sources[i].type, source->c_str()))
        {
            LOG_ERROR("Cannot create shader program: failed to compile at %s shader stage.\n", ShaderTypeToString(static_cast<ShaderType>(sources[i].type)));
            for (const ResourceHandle shader : shaders)
                if (shader != 0)
                    glDeleteShader(shader);
            return false;
        }
    }

    const bool linked = LinkShaders(outShaderProgram, shaders.data(), shaders.size());

    if (!linked)
        LOG_ERROR("Cannot create shader program: failed to link shaders.\n");

    for (const ResourceHandle shader : shaders) // Cleanup
        glDeleteShader(shader);

    return linked;
}

bool Graphics::CreateMeshBuffers(ResourceHandle &outVertexArray, ResourceHandle &outVertexBuffer,
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

bool Graphics::CreateTexture2D(ResourceHandle &outTexture, const TextureData &textureData)
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


bool Graphics::LoadShaderResources(std::vector<ShaderResource> &outShaders,
    std::vector<std::string> &outNames, const std::filesystem::path &rootDirectory)
{
    using namespace Graphics;
    using ShaderGroupMap = std::map<std::string, std::vector<ShaderSource>>;

    ShaderGroupMap shaderGroups;
    const bool addedNew = LoadResourcesFromFiles_Parallel<ShaderGroupMap>(
        rootDirectory, "shaders", "Shader", std::regex(".*\\.glsl", std::regex::icase), !outShaders.empty(),
        [root = rootDirectory](const std::filesystem::path &file, std::size_t,
            ShaderGroupMap &results)
        {
            LOG_SCOPE;

            const std::string fileString = file.string();
            const Graphics::ShaderType type = GetShaderFileType(file);

            LOG_INFO("Loading shader file '%s'.\n", fileString.c_str());

            if (type == Graphics::ShaderType::Unknown)
            {
                LOG_SCOPE;
                LOG_WARNING("Skipping shader file with an unrecognized stage: '%s'.\n", fileString.c_str());
                return false;
            }

            std::string source;
            if (!IOUtils::ReadFileToEnd(fileString.c_str(), source))
            {
                LOG_SCOPE;
                LOG_WARNING("Failed to read shader file '%s'.\n", fileString.c_str());
                return false;
            }

            const std::filesystem::path stem = file.lexically_relative(root).stem();
            const std::filesystem::path groupName = stem.extension().empty() ? std::filesystem::path("default") : stem.stem();
            const std::string name = IOUtils::GetRelativeName(file, root, groupName);

            results[name].push_back({std::move(source), type});
            return true;
        },
        [&shaderGroups](ShaderGroupMap chunkGroups) // Merge shader groups
        {
            const bool added = !chunkGroups.empty();
            for (auto &[groupName, sources] : chunkGroups)
            {
                auto [group, inserted] = shaderGroups.try_emplace(groupName, std::move(sources));
                if (!inserted)
                    group->second.insert(group->second.end(),
                        std::make_move_iterator(sources.begin()),
                        std::make_move_iterator(sources.end()));
            }
            return added;
        });

    for (auto &[name, sources] : shaderGroups)
    {
        LOG_SCOPE;
        LOG_INFO("Found shader program '%s' with %zu stage(s).\n", name.c_str(), sources.size());
        outShaders.emplace_back(std::move(sources));
        outNames.emplace_back(std::move(name));
    }

    return addedNew;
}

bool Graphics::LoadMeshResources(std::vector<MeshResource> &outMeshes,
    std::vector<std::string> &outNames, const std::filesystem::path &rootDirectory)
{
    using namespace Graphics;
    using MeshChunkResult = std::tuple<std::vector<MeshResource>, std::vector<std::string>>;

    return LoadResourcesFromFiles_Parallel<MeshChunkResult>(
        rootDirectory, "meshes", "Model", std::regex(".*\\.obj", std::regex::icase), !outMeshes.empty(),
        [root = rootDirectory](const std::filesystem::path &file, std::size_t,
            MeshChunkResult &results)
        {
            LOG_SCOPE;

            const std::string fileString = file.string();

            LOG_INFO("Loading mesh file '%s'.\n", fileString.c_str());

            std::vector<MeshVertices> vertices;
            std::vector<MeshIndices> indices;
            std::vector<std::string> names;

            if (!LoadMeshFile(fileString.c_str(), names, vertices, indices))
            {
                LOG_WARNING("Failed to load one or more mesh objects in file '%s'.\n", fileString.c_str());
                return false;
            }

            for (std::size_t i = 0; i < vertices.size(); ++i)
            {
                if (names[i].empty())
                    names[i] = IOUtils::GetRelativeName(file, root, file.stem());

                std::get<0>(results).emplace_back(std::move(vertices[i]), std::move(indices[i]));
                std::get<1>(results).emplace_back(std::move(names[i]));
            }
            return true;
        },
        [&outMeshes, &outNames](MeshChunkResult results)
        {
            auto [meshes, names] = std::move(results);
            if (meshes.empty())
                return false;

            outMeshes.insert(outMeshes.end(),
                std::make_move_iterator(meshes.begin()),
                std::make_move_iterator(meshes.end()));
            outNames.insert(outNames.end(),
                std::make_move_iterator(names.begin()),
                std::make_move_iterator(names.end()));
            return true;
        });
}

bool Graphics::LoadTextureResources(std::vector<Texture2DResource> &outTextures,
    std::vector<std::string> &outNames, const std::filesystem::path &rootDirectory)
{
    using namespace Graphics;
    using TextureChunkResult = std::tuple<std::vector<Texture2DResource>, std::vector<std::string>>;

    return LoadResourcesFromFiles_Parallel<TextureChunkResult>(
        rootDirectory, "textures", "Texture", std::regex(".*\\.(png|jpg|jpeg)", std::regex::icase),
        !outTextures.empty(),
        [root = rootDirectory](const std::filesystem::path &file, std::size_t,
            TextureChunkResult &results)
        {
            LOG_SCOPE;

            const std::string fileString = file.string();

            LOG_INFO("Loading texture file '%s'.\n", fileString.c_str());
            
            int width = 0, height = 0;
            std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> image(
                stbi_load(fileString.c_str(), &width, &height, nullptr, STBI_rgb_alpha), stbi_image_free);

            if (!image)
            {
                LOG_WARNING("Failed to load texture file '%s': '%s'.\n", fileString.c_str(), stbi_failure_reason());
                return false;
            }

            const std::string name = IOUtils::GetRelativeName(file, root, file.stem());
            const auto byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
            std::vector<UInt8> rgba(image.get(), image.get() + byteCount);

            std::get<0>(results).emplace_back(width, height, std::move(rgba));
            std::get<1>(results).emplace_back(std::move(name));
            return true;
        },
        [&outTextures, &outNames](TextureChunkResult results)
        {
            auto [textures, names] = std::move(results);
            if (textures.empty())
                return false;

            outTextures.insert(outTextures.end(),
                std::make_move_iterator(textures.begin()),
                std::make_move_iterator(textures.end()));
            outNames.insert(outNames.end(),
                std::make_move_iterator(names.begin()),
                std::make_move_iterator(names.end()));
            return true;
        });
}

bool Graphics::LoadAndInitResources(std::vector<Graphics::ShaderResource> &outShaders,
    std::vector<Graphics::MeshResource> &outMeshes, std::vector<Graphics::Texture2DResource> &outTextures,
    std::vector<std::string> &outShaderNames, std::vector<std::string> &outMeshNames,
    std::vector<std::string> &outTextureNames, const std::filesystem::path &rootDirectory)
{
    LOG_SCOPE;
    LOG_INFO("Loading assets from '%s'.\n", rootDirectory.string().c_str());

    using namespace Graphics;

    {
        LOG_SCOPE;

        StopWatch<std::chrono::steady_clock> stopWatch;

        LOG_INFO("Loading resources... \n");
        stopWatch.Reset();
        LoadShaderResources(outShaders, outShaderNames, SHADERS_DIR);
        LoadMeshResources(outMeshes, outMeshNames, MODELS_DIR);
        LoadTextureResources(outTextures, outTextureNames, TEXTURES_DIR);
        LOG_INFO("Loading resources took %f seconds.\n", TimeDuration(stopWatch.CurrentDuration()).count());

        LOG_INFO("Initializing resources... \n");
        stopWatch.Reset();
        std::size_t shaderCount = outShaders.size(),
            meshCount = outMeshes.size(),
            textureCount = outTextures.size(),
            failedShaders,
            failedMeshes,
            failedTextures;

        const bool shadersInitialized = InitializeResources(outShaders, outShaderNames, failedShaders);
        const bool meshesInitialized = InitializeResources(outMeshes, outMeshNames, failedMeshes);
        const bool texturesInitialized = InitializeResources(outTextures, outTextureNames, failedTextures);
        LOG_INFO("Initializing resources took %f seconds.\n", TimeDuration(stopWatch.CurrentDuration()).count());
        
        if (outShaders.empty())
            LOG_ERROR("No initialized shader resources are available.\n");
        if (!shadersInitialized && !outShaders.empty())
            LOG_WARNING("%zu of %zu shader resources failed to initialize and were discarded.\n", failedShaders, shaderCount);

        if (outMeshes.empty())
            LOG_WARNING("No initialized mesh resources are available.\n");
        if (!meshesInitialized && !outMeshes.empty())
            LOG_WARNING("%zu of %zu mesh resources failed to initialize and were discarded.\n", failedMeshes, meshCount);

        if (outTextures.empty())
            LOG_ERROR("No initialized texture resources are available.\n");
        if (!texturesInitialized && !outTextures.empty())
            LOG_WARNING("%zu of %zu texture resources failed to initialize and were discarded.\n", failedTextures, textureCount);
    }

    return !outShaders.empty() && !outTextures.empty() && !outMeshes.empty();
}
