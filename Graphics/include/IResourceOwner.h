#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <type_traits>
#include <vector>

namespace Graphics
{
    class ResourceOwner
    {
    protected:
        ResourceOwner() = default;
        ResourceOwner(ResourceOwner &&) = default;
        ResourceOwner &operator=(ResourceOwner &&) = default;

    public:
        ResourceOwner(const ResourceOwner &) = delete;
        ResourceOwner &operator=(const ResourceOwner &) = delete;

        virtual ~ResourceOwner() = default;

        virtual bool IsInitialized() const = 0;
        virtual void ReleaseResources() noexcept = 0;
        virtual bool InitializeResources() { ReleaseResources(); return true; }
    };

    bool InitializeResources(ResourceOwner *owners[], std::size_t count);
    bool InitializeResources(std::vector<ResourceOwner *> &owners);

    template <typename T>
    bool InitializeResources(ResourceOwner *owners[], std::size_t count)
    {
        static_assert(std::is_base_of_v<ResourceOwner, T>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        for (std::size_t i = 0; i < count; ++i)
            if (!owners[i]->InitializeResources())
                return false;

        return true;
    }

    template <typename T>
    bool InitializeResources(std::vector<T> &owners)
    {
        static_assert(std::is_base_of_v<ResourceOwner, T>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        for (T &owner : owners)
            if (!owner.InitializeResources())
                return false;

        return true;
    }
}