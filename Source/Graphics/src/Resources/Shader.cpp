#include "Shader.h"

#include "LogHandler.h"

#include <glm/gtc/type_ptr.hpp>
#include <utility>

Graphics::Shader::Shader(std::vector<ShaderSource> sources)
    : sources(std::move(sources)) { }

Graphics::Shader::Shader(Shader &&other) noexcept
    : ResourceOwner(std::move(other)), shaderProgram(std::exchange(other.shaderProgram, 0)),
      sources(std::move(other.sources)),
      memberLocations(std::move(other.memberLocations)) { }

Graphics::Shader &Graphics::Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        sources = std::move(other.sources);
        shaderProgram = std::exchange(other.shaderProgram, 0);
        memberLocations = std::move(other.memberLocations);
    }

    return *this;
}

bool Graphics::Shader::GetUniformHandle(const std::string &name, ShaderMemberHandle &outHandle)
{
    auto it = memberLocations.find(name);
    if (it != memberLocations.end())
    {
        outHandle = it->second;
        return true;
    }

    outHandle = glGetUniformLocation(shaderProgram, name.c_str());
    if (outHandle < 0) return false;

    memberLocations.emplace(name, outHandle);
    return true;
}

bool Graphics::Shader::GetAttributeHandle(const std::string &name, ShaderMemberHandle &outHandle) const
{
    outHandle = glGetAttribLocation(shaderProgram, name.c_str());
    return outHandle != -1;
}

void Graphics::Shader::ReleaseResources() noexcept
{
    if (shaderProgram != 0)
        glDeleteProgram(shaderProgram);
    shaderProgram = 0;
}

bool Graphics::Shader::InitializeResources()
{
    LOG_SCOPE;

    if (!ResourceOwner::InitializeResources())
        return false;

    if (sources.empty())
    {
        LOG_ERROR("Cannot initialize shader: no shader sources provided.\n");
        return false;
    }

    if (!CreateGLShaderProgram(shaderProgram, sources.data(), sources.size()))
        return false;

    return true;
}


bool Graphics::CompileGLShader(ResourceHandle &outShader, const ShaderSourceType type, const char *source)
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
            LOG_ERROR("Cannot create shader program: failed to compile at %s shader stage.\n",
                ShaderSourceTypeToString(static_cast<ShaderSourceType>(sources[i].type)));
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
