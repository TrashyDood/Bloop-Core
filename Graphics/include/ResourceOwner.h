#pragma once

#include "LogHandler.h"

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
        virtual bool InitializeResources();
    };

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
    bool InitializeResources(std::vector<T> &owners, bool eraseOnFail = true)
    {
        static_assert(std::is_base_of_v<ResourceOwner, T>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        bool allInitialized = true;
        for (auto owner = owners.begin(); owner != owners.end();)
        {
            if (owner->InitializeResources())
            {
                ++owner;
                continue;
            }

            allInitialized = false;
            static_cast<ResourceOwner &>(*owner).ReleaseResources();

            if (eraseOnFail)
                owner = owners.erase(owner);
        }

        return allInitialized;
    }
} // namespace Graphics