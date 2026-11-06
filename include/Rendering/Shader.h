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
        [[nodiscard]] bool IsInitialized() const { return shaderProgram != 0; }
        [[nodiscard]] bool HasSources() const { return !sources.empty(); }

        [[nodiscard]]
        GLint GetUniformLocation(const std::string &name) const { return glGetUniformLocation(shaderProgram, name.c_str()); }
        [[nodiscard]]
        GLint GetAttributeLocation(const std::string &name) const { return glGetAttribLocation(shaderProgram, name.c_str()); }

        Shader() = default;
        explicit Shader(std::vector<ShaderSource> sources) : sources(std::move(sources)) {}

        ~Shader();

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;

        Shader(Shader &&other) noexcept;
        Shader &operator=(Shader &&other) noexcept;

        void Use() const { glUseProgram(shaderProgram); }
        virtual bool InitializeResources() override;

    private:
        virtual void ReleaseResources() noexcept override;
    };
}