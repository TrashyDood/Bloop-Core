#pragma once

#include "ResourceLoading.h"
#include "ThreadPool.h"

#include <algorithm>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <functional>

class AssetResourceManager
{
    using ResourceIndexMap = std::unordered_map<std::string, std::size_t>;

    template <typename TResource>
    struct AssetRegistry
    {
        std::vector<TResource> resources;
        std::vector<std::string> names;
        ResourceIndexMap indexMap;
        Graphics::ResourceLoadQueue<TResource> loadQueue;
    };

    AssetRegistry<Graphics::ShaderResource> shaders;
    AssetRegistry<Graphics::MeshResource> meshes;
    AssetRegistry<Graphics::Texture2DResource> textures;

    // Injects the given resources into given list and assigns them a name-to-index key
    template <typename TResource>
    static void IndexResources(const std::vector<TResource> &resources, const std::vector<std::string> &names,
        ResourceIndexMap &indexMap, const char *resourceDomain)
    {
        LOG_SCOPE;

        if (resources.size() != names.size())
            LOG_WARNING("The number of %s resources does not match the number of names.\n", resourceDomain);

        const std::size_t count = std::min(resources.size(), names.size());
        for (std::size_t i = 0; i < count; ++i)
            if (!indexMap.try_emplace(names[i], i).second)
                LOG_WARNING("Multiple %s resources share the name '%s'; name lookup resolves to the first.\n",
                    resourceDomain, names[i].c_str());
    }

    template <typename TResource>
    AssetRegistry<TResource> &GetAssetRegistry()
    {
        if constexpr (std::is_same_v<TResource, Graphics::ShaderResource>)
            return shaders;
        else if constexpr (std::is_same_v<TResource, Graphics::MeshResource>)
            return meshes;
        else
        {
            static_assert(std::is_same_v<TResource, Graphics::Texture2DResource>,
                "AssetResourceManager only supports shader, mesh, and texture resources.");
            return textures;
        }
    }

public:
    template<typename TResource>
    void EnqueueAssetLoad(const IO::Path &directory,
        std::function<void()> &&onFinishedCallback = [] {})
    {
        GetAssetRegistry<TResource>().loadQueue.emplace(
            directory, std::move(onFinishedCallback));
    }

    template <typename TResource>
    const AssetRegistry<TResource> &GetAssetRegistry() const
    {
        if constexpr (std::is_same_v<TResource, Graphics::ShaderResource>)
            return shaders;
        else if constexpr (std::is_same_v<TResource, Graphics::MeshResource>)
            return meshes;
        else
        {
            static_assert(std::is_same_v<TResource, Graphics::Texture2DResource>,
                "AssetResourceManager only supports shader, mesh, and texture resources.");
            return textures;
        }
    }

    [[nodiscard]] const std::vector<Graphics::ShaderResource> &Shaders() noexcept { return shaders.resources; }
    [[nodiscard]] const std::vector<Graphics::MeshResource> &Meshes() noexcept { return meshes.resources; }
    [[nodiscard]] const std::vector<Graphics::Texture2DResource> &Textures() const noexcept { return textures.resources; }
    [[nodiscard]] const std::vector<std::string> &MeshNames() const noexcept { return meshes.names; }
    [[nodiscard]] const std::vector<std::string> &ShaderNames() const noexcept { return shaders.names; }
    [[nodiscard]] const std::vector<std::string> &TextureNames() const noexcept { return textures.names; }

    template <typename TResource>
    void GetResourceIndexByName(Int32 &outIndex, const std::string &name) const
    {
        outIndex = -1;
        const auto &indexMap = GetAssetRegistry<TResource>().indexMap;
        const auto it = indexMap.find(name);
        if (it != indexMap.end() &&
            it->second <= static_cast<std::size_t>(std::numeric_limits<Int32>::max()))
            outIndex = static_cast<Int32>(it->second);
    }

    template <typename TResource>
    TResource *GetResourceByName(const std::string &name)
    {
        Int32 index;
        GetResourceIndexByName<TResource>(index, name);
        return GetResourceByIndex<TResource>(index);
    }

    template <typename TResource>
    TResource *GetResourceByIndex(const Int32 index)
    {
        if (index < 0)
            return nullptr;

        auto &assets = GetAssetRegistry<TResource>();
        const auto resourceIndex = static_cast<std::size_t>(index);
        if (resourceIndex >= assets.resources.size())
            return nullptr;

        return &assets.resources[resourceIndex];
    }

    // Checks if any pending resource loading tasks are finished, if they are, we initialize and index them.
    void PollPendingResources();
};

template<typename TResource>
class AssetHandle
{
    AssetResourceManager *manager = nullptr;
    std::string name;
    Int32 resourceId = -1;

public:
    [[nodiscard]] const bool IsValid() const { return resourceId > -1 && manager; }
    [[nodiscard]] const std::string &Name() const { return name; }
    [[nodiscard]] TResource *GetResource() const
    {
        return manager ? manager->GetResourceByIndex<TResource>(resourceId) : nullptr;
    }

    AssetHandle() = default;
    AssetHandle(AssetResourceManager &manager, const std::string &name) :
        manager(&manager), name(name)
    {
        manager.GetResourceIndexByName<TResource>(resourceId, name);
    }
};