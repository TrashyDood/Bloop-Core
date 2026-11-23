#pragma once

#include "Shared.h"
#include "IOUtils.h"

#include "MeshResource.h"
#include "ShaderResource.h"
#include "Texture2DResource.h"

#include <vector>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <queue>
#include <functional>
#include <future>
#include <iterator>

namespace Graphics
{
    // Simply a wrapper/helper binding a vector of resources and a vector names
    // together along with some diagnostic data. Only used when loading resources.
    template<typename T>
    struct LoadedResources
    {
        std::vector<T> &loadedResources;
        std::vector<std::string> &resourceNames;
        UInt32 filesFound = 0, filesFailed = 0;
        UInt32 resourcesLoaded = 0, resourcesFailed = 0, uninitializedCount = 0;

        bool IsEmpty() const { return loadedResources.empty() && resourceNames.empty(); }
        std::size_t NewIndex() const { return loadedResources.size() - uninitializedCount; }

        void AppendFrom(LoadedResources<T> &source)
        {
            if (loadedResources.empty() && resourceNames.empty())
            {
                loadedResources = std::move(source.loadedResources);
                resourceNames = std::move(source.resourceNames);
            }
            else
            {
                loadedResources.insert(loadedResources.end(),
                    std::make_move_iterator(source.loadedResources.begin()),
                    std::make_move_iterator(source.loadedResources.end()));
                resourceNames.insert(resourceNames.end(),
                    std::make_move_iterator(source.resourceNames.begin()),
                    std::make_move_iterator(source.resourceNames.end()));
            }
        }
    };

    namespace // Local helpers
    {
        // Helper function to recursively unpack argument pairs of (LoadedResources, IO::Path) and submit them as
        // parallel tasks.
        template <typename TResource, typename... TArgs>
        void AppendResourceTasks(std::vector<ThreadTask<void>> &tasks, LoadedResources<TResource> &outResults,
            const IO::Path &directory, TArgs &&...args)
        {
            tasks.emplace_back(StaticThreadPool().SubmitTask(
                [&outResults, directory]() { return LoadResources_Parallel(outResults, directory); }));

            if constexpr (sizeof...(TArgs) > 0)
                AppendResourceTasks(tasks, std::forward<TArgs>(args)...);
        }
    }

#pragma region Load Resource File
    bool LoadShaderResourceFile(Graphics::ShaderSource &outShaderSource, std::string &outGroup, const IO::Path &file);
    void LoadMeshFile(std::vector<Graphics::MeshResource> &outMeshes, std::vector<std::string> &outNames,
        const IO::Path &file, UInt32 &outSucceededCount, UInt32 &outFailedCount);
    bool LoadTexture2DResourceFile(Graphics::Texture2DResource &outTexture, std::string &outName, const IO::Path &file);
#pragma endregion

#pragma region Load Resources
    void LoadShaderResources_Parallel(LoadedResources<Graphics::ShaderResource> &outResults, const IO::Path &directory);
    void LoadMeshResources_Parallel(LoadedResources<Graphics::MeshResource> &outResults, const IO::Path &directory);
    void LoadTextureResources_Parallel(LoadedResources<Graphics::Texture2DResource> &outResults, const IO::Path &directory);
#pragma endregion

#pragma region Load Resources Generic
    template <typename TResource>
    void LoadResources_Parallel(LoadedResources<TResource> &outResources, const IO::Path &directory)
    {
        if constexpr (std::is_same_v<TResource, Graphics::ShaderResource>)
            LoadShaderResources_Parallel(outResources, directory);
        else if constexpr (std::is_same_v<TResource, Graphics::MeshResource>)
            LoadMeshResources_Parallel(outResources, directory);
        else if constexpr (std::is_same_v<TResource, Graphics::Texture2DResource>)
            LoadTextureResources_Parallel(outResources, directory);
        else
            static_assert(std::is_same_v<TResource, Graphics::ShaderResource> ||
                std::is_same_v<TResource, Graphics::MeshResource> ||
                std::is_same_v<TResource, Graphics::Texture2DResource>,
                "LoadResources_Parallel supports shader, mesh, and texture resources.");
    }

    template <typename... TArgs>
    void LoadResources_Parallel(TArgs &&...arguments)
    {
        static_assert(sizeof...(TArgs) > 0 && sizeof...(TArgs) % 2 == 0,
            "LoadResources_Parallel expects (LoadedResources, IO::Path) argument pairs.");

        std::vector<ThreadTask<void>> tasks;
        tasks.reserve(sizeof...(TArgs) / 2);
        AppendResourceTasks(tasks, std::forward<TArgs>(arguments)...);

        for (auto &task : tasks)
            task.Get();
    }
#pragma endregion

#pragma region Init Resources
    template <typename TResources>
    void InitializeResources(LoadedResources<TResources> &outResources, bool eraseOnFail = true)
    {
        static_assert(std::is_base_of_v<ResourceOwner, TResources>,
            "InitializeResources requires a vector of ResourceOwner-derived objects.");

        auto &resources = outResources.loadedResources;
        auto &names = outResources.resourceNames;

        if (outResources.uninitializedCount < 1)
            return;

        if (InitializeResources(resources, outResources.NewIndex()) > 0 && eraseOnFail)
        {
            for (std::size_t i = outResources.NewIndex(); i < resources.size();)
            {
                if (!resources[i].IsInitialized())
                {
                    resources.erase(resources.begin() + i);
                    names.erase(names.begin() + i);
                }
                else
                    ++i;
            }
        }

        outResources.uninitializedCount = 0;
    }

    template <typename... TResources>
    void InitializeResources(bool eraseOnFail, LoadedResources<TResources> &...outResources)
    {
        static_assert(sizeof...(TResources) > 0,
            "InitializeResources requires at least one resource list.");

        (InitializeResources(outResources, eraseOnFail), ...);
    }
#pragma endregion

    template <typename TResource>
    struct ResourceLoadQueueEntry;
    template <typename TResource>
    using ResourceLoadQueue = std::queue<ResourceLoadQueueEntry<TResource>>;

    // A wrapper for an async task loading assets in the background.
    template <typename TResource>
    struct ResourceLoadQueueEntry
    {
        std::vector<TResource> resources;
        std::vector<std::string> names;
        std::function<void()> onFinishedCallback;
        LoadedResources<TResource> results;
        ThreadTask<void> task;

        static void Enqueue(ResourceLoadQueue<TResource> &queue,
            const IO::Path &directory, std::function<void()> &&onFinishedCallback)
        {
            queue.emplace(std::move(directory), std::move(onFinishedCallback));
        }

        ResourceLoadQueueEntry(const IO::Path directory, std::function<void()> &&onFinishedCallback)
            : onFinishedCallback(std::move(onFinishedCallback)), results{resources, names},
              task(StaticThreadPool().SubmitTask([this, directory] { LoadResources_Parallel(results, directory); })) { }

        void Initialize()
        {
            task.Get();
            InitializeResources(results, true);
        }
    };

    template <typename TResource, typename TIndexMap>
    void PollResourceLoadQueue(ResourceLoadQueue<TResource> &queue, std::vector<TResource> &outResources,
        std::vector<std::string> &outNames, TIndexMap &indexMap, const char *resourceDomain)
    {
        LoadedResources<TResource> loadedResources{outResources, outNames};
        while (!queue.empty() && queue.front().task.Status() == std::future_status::ready)
        {
            auto &entry = queue.front();
            entry.Initialize();

            const std::size_t firstNewIndex = outResources.size();
            loadedResources.AppendFrom(entry.results);

            for (std::size_t i = firstNewIndex; i < outResources.size(); ++i)
                if (!indexMap.try_emplace(outNames[i], i).second)
                    LOG_WARNING("Multiple %s resources share the name '%s'; name lookup resolves to the first.\n",
                        resourceDomain, outNames[i].c_str());

            entry.onFinishedCallback();
            queue.pop();
        }
    }
} // namespace Graphics