#pragma once

#include "Rendering.h"
#include "Shared.h"
#include "IResourceOwner.h"

#include <utility>
#include <vector>

namespace Rendering
{
    class ShaderResource : public IResourceOwner
    {
        ResourceHandle shaderProgram = 0;
        std::vector<ShaderSource> sources;

    public:
        [[nodiscard]] ResourceHandle GetProgramHandle() const { return shaderProgram; }
        [[nodiscard]] std::size_t GetSourceCount() const { return sources.size(); }
        [[nodiscard]] const ShaderSource &GetSource(std::size_t index) const { return sources.at(index); }
        [[nodiscard]] const std::vector<ShaderSource> &GetSources() const { return sources; }
        [[nodiscard]] bool HasSources() const { return !sources.empty(); }
        [[nodiscard]] bool IsInitialized() const override { return shaderProgram != 0; }

        [[nodiscard]]
        bool GetUniformLocation(const std::string &name, GLint &location) const;
        [[nodiscard]]
        bool TryGetAttributeLocation(const std::string &name, GLint &location) const;

        explicit ShaderResource(std::vector<ShaderSource> sources) : sources(std::move(sources)) {}

        // Transfers the program handle, leaving the source without ownership.
        ShaderResource(ShaderResource &&other) noexcept
            : shaderProgram(std::exchange(other.shaderProgram, 0)), sources(std::move(other.sources)) {}
        // Releases this program before taking ownership of the source's.
        ShaderResource &operator=(ShaderResource &&other) noexcept;

        ~ShaderResource() { ReleaseResources(); }

        ShaderResource(const ShaderResource &) = delete;
        ShaderResource &operator=(const ShaderResource &) = delete;

        void Use() const { glUseProgram(shaderProgram); }
        bool InitializeResources() override;

    private:
        void ReleaseResources() noexcept override;
    };
}