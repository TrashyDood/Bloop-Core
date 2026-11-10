#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <vector>

namespace Rendering
{
    class IResourceOwner
    {
    protected:
        IResourceOwner() = default;
        IResourceOwner(IResourceOwner &&) = default;
        IResourceOwner &operator=(IResourceOwner &&) = default;

    public:
        IResourceOwner(const IResourceOwner &) = delete;
        IResourceOwner &operator=(const IResourceOwner &) = delete;

        virtual ~IResourceOwner() = default;

        virtual bool IsInitialized() const = 0;
        virtual void ReleaseResources() noexcept = 0;
        virtual bool InitializeResources() { ReleaseResources(); return true; }
    };

    bool InitializeResources(IResourceOwner *owners[], std::size_t count);
    inline bool InitializeResources(std::vector<IResourceOwner *> owners) { return InitializeResources(owners.data(), owners.size()); }
}