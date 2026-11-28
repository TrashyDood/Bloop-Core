#pragma once

#include "LogHandler.h"
#include "Shared.h"

#include <cstddef>
#include <type_traits>
#include <vector>

namespace Graphics
{
    namespace
    {
        struct NoOpEraseCallback { void operator()(std::size_t) const noexcept { } };
    }

    class ResourceOwner
    {
    protected:
        ResourceOwner() = default;
        ResourceOwner(ResourceOwner &&) = default;
        ResourceOwner &operator=(ResourceOwner &&) = default;

    public:
        ResourceOwner(const ResourceOwner &) = delete;
        ResourceOwner &operator=(const ResourceOwner &) = delete;

        virtual bool IsInitialized() const noexcept = 0;
        virtual void ReleaseResources() noexcept {}
        virtual bool InitializeResources();
    };

    // Initializes the given vector of resources.
    // Returns the number of resources that failed initialization.
    template <typename T>
    UInt32 InitializeResources(std::vector<T> &owners, const std::size_t startAt = 0)
    {
        static_assert(std::is_base_of_v<ResourceOwner, T>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        UInt32 failedCount = 0;
        for (std::size_t index = startAt; index < owners.size();)
        {
            if (owners[index].InitializeResources())
            {
                ++index;
                continue;
            }

            static_cast<ResourceOwner &>(owners[index]).ReleaseResources();
            ++index;
            ++failedCount;
        }

        return failedCount;
    }

    // Initialize multiple vectors of ResourceOwner-derived objects, returning the total number that failed.
    template <typename... TResourceTypes>
    UInt32 InitializeResources(std::vector<TResourceTypes> &...owners) { return (InitializeResources(owners) + ...); }
} // namespace Graphics