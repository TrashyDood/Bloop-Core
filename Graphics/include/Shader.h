#pragma once

#include "Graphics.h"
#include "Shared.h"
#include "ResourceOwner.h"

#include <vector>

namespace Graphics
{
    class Shader : public ResourceOwner
    {
        ResourceHandle shaderProgram = 0;
        std::vector<ShaderSource> sources;

    public:
        [[nodiscard]] ResourceHandle GetProgramHandle() const { return shaderProgram; }
        [[nodiscard]] std::size_t GetSourceCount() const { return sources.size(); }
        [[nodiscard]] const ShaderSource &GetSource(std::size_t index) const { return sources.at(index); }
        [[nodiscard]] const std::vector<ShaderSource> &GetSources() const { return sources; }
        [[nodiscard]] bool HasSources() const { return !sources.empty(); }
        [[nodiscard]] bool IsInitialized() const override;

        [[nodiscard]]
        bool GetUniformLocation(const std::string &name, GLint &location) const;
        [[nodiscard]]
        bool GetAttributeLocation(const std::string &name, GLint &location) const;

        explicit Shader(std::vector<ShaderSource> sources);

        Shader(Shader &&other) noexcept;
        Shader &operator=(Shader &&other) noexcept;
        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;
        
        ~Shader() override;

        void Use() const { glUseProgram(shaderProgram); }
        bool InitializeResources() override;

    private:
        void ReleaseResources() noexcept override;
    };
}