#pragma once

#include "Rendering/Rendering.h"
#include "Shared/Shared.h"
#include "Rendering/IResourceOwner.h"

#include <utility>
#include <vector>

namespace Rendering
{
    class Shader : public IResourceOwner
    {
        ResourceHandle shaderProgram = 0;
        std::vector<ShaderSource> sources;

    public:
        [[nodiscard]] ResourceHandle GetProgramHandle() const { return shaderProgram; }
        [[nodiscard]] std::size_t GetSourceCount() const { return sources.size(); }
        [[nodiscard]] const ShaderSource &GetSource(std::size_t index) const { return sources.at(index); }
        [[nodiscard]] const std::vector<ShaderSource> &GetSources() const { return sources; }
        [[nodiscard]] bool HasSources() const { return !sources.empty(); }

        [[nodiscard]]
        bool TryGetUniformLocation(const std::string &name, GLint &location) const;
        [[nodiscard]]
        bool TryGetAttributeLocation(const std::string &name, GLint &location) const;

        [[nodiscard]] bool IsInitialized() const override { return shaderProgram != 0; }

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        Shader() = default;

        explicit Shader(std::vector<ShaderSource> sources) : sources(std::move(sources)) {}
        ~Shader() { ReleaseResources(); }

        Shader(Shader &&other) noexcept
            : shaderProgram(std::exchange(other.shaderProgram, 0)), sources(std::move(other.sources)) {}
        Shader &operator=(Shader &&other) noexcept;

        void Use() const { glUseProgram(shaderProgram); }
        bool InitializeResources() override;

    private:
        void ReleaseResources() noexcept override;
    };
}