#include "ShaderResource.h"

#include "ResourceUtils.h"
#include "LogHandler.h"

#include <utility>

Graphics::ShaderResource::ShaderResource(std::vector<ShaderSource> sources)
    : sources(std::move(sources)) { }

Graphics::ShaderResource::ShaderResource(ShaderResource &&other) noexcept
    : ResourceOwner(std::move(other)), shaderProgram(std::exchange(other.shaderProgram, 0)),
      sources(std::move(other.sources)) { }

bool Graphics::ShaderResource::GetUniformLocation(const std::string &name, GLint &location) const
{
    location = glGetUniformLocation(shaderProgram, name.c_str());
    return location != -1;
}

bool Graphics::ShaderResource::GetAttributeLocation(const std::string &name, GLint &location) const
{
    location = glGetAttribLocation(shaderProgram, name.c_str());
    return location != -1;
}

Graphics::ShaderResource &Graphics::ShaderResource::operator=(ShaderResource &&other) noexcept
{
    if (this != &other)
    {
        ReleaseResources();
        ResourceOwner::operator=(std::move(other));
        sources = std::move(other.sources);
        shaderProgram = std::exchange(other.shaderProgram, 0);
    }

    return *this;
}

Graphics::ShaderResource::~ShaderResource()
{
    ReleaseResources();
}

bool Graphics::ShaderResource::IsInitialized() const
{
    return shaderProgram != 0;
}

void Graphics::ShaderResource::ReleaseResources() noexcept
{
    if (shaderProgram != 0)
        glDeleteProgram(shaderProgram);
    shaderProgram = 0;
}

bool Graphics::ShaderResource::InitializeResources()
{
    LOG_SCOPE;

    if (!ResourceOwner::InitializeResources())
        return false;

    if (sources.empty())
    {
        LOG_ERROR("Cannot initialize shader: no shader sources provided.\n");
        return false;
    }

    if (!CreateShaderProgram(shaderProgram, sources.data(), sources.size()))
        return false;

    return true;
}