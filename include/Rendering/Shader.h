#pragma once

#include "Rendering/Core.h"
#include "Shared/Core.h"

#include <utility>
#include <vector>

namespace Rendering
{
    class Shader
    {
        ResourceHandle shaderProgram = 0;
        std::vector<ShaderSource> sources;

    public:
        [[nodiscard]] ResourceHandle GetProgramHandle() const { return shaderProgram; }
        [[nodiscard]] std::size_t GetSourceCount() const { return sources.size(); }
        [[nodiscard]] const ShaderSource &GetSource(std::size_t index) const { return sources.at(index); }
        [[nodiscard]] const std::vector<ShaderSource> &GetSources() const { return sources; }
        [[nodiscard]] bool IsInitialized() const { return shaderProgram != 0; }
        [[nodiscard]] bool HasSources() const { return !sources.empty(); }

        Shader() = default;
        explicit Shader(std::vector<ShaderSource> sources) : sources(std::move(sources)) {}

        ~Shader();

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        Shader(Shader &&other) noexcept;
        Shader &operator=(Shader &&other) noexcept;

        bool InitializeResources(bool reinitialize = false);
        void Use() const;

    private:
        void ReleaseResources() noexcept;
    };
}