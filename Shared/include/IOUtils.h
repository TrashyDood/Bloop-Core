#pragma once

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

#include "ThreadPool.h"

#include <map>

namespace IO
{
    struct Path
    {
        const std::filesystem::path absolutePath,
            relativePath;

        static Path FromAbsolute(const std::filesystem::path &absolutePath);
        static Path FromAbsolute(const std::filesystem::path &absolutePath,
            const std::filesystem::path &rootDirectory);
        static Path FromRelative(const std::filesystem::path &relativePath);
        static Path FromRelative(const std::filesystem::path &relativePath,
            const std::filesystem::path &rootDirectory);

        private:
        Path() = default;
        Path(const std::filesystem::path &absolutePath, const std::filesystem::path &relativePath)
            : absolutePath(absolutePath), relativePath(relativePath) {}
    };

    std::filesystem::path &WorkingDirectory();
    std::filesystem::path GetAbsolutePath(const std::filesystem::path &relativePath);

    bool ReadFileToEnd(const char *filename, std::string &outContents);
    
    std::vector<std::filesystem::path> FindMatchingFiles(const std::filesystem::path &rootDirectory,
        const std::regex &filenamePattern);
    std::vector<std::filesystem::path> FindMatchingFiles_Parallel(const std::filesystem::path &rootDirectory,
        const std::regex &filenamePattern);

    template <typename TFunc, typename... TArgs>
    void IterateMatchingFiles_Parallel(const std::filesystem::path &rootDirectory, const std::regex &filenamePattern,
        const TFunc &func, const TArgs &...args)
    {
        static_assert(std::is_invocable_v<const TFunc &, const std::filesystem::path &, std::size_t, const TArgs &...>,
            "func must be callable as func(const path&, size_t, const Args&...)");

        const auto files = FindMatchingFiles_Parallel(rootDirectory, filenamePattern);
        if (files.empty())
            return;

        auto tasks = StaticThreadPool().For_Parallel(files.size(),
            [&](std::size_t, std::size_t start, std::size_t end)
            {
                for (std::size_t i = start; i < end; ++i)
                    func(files[i], i, args...);
            });
    }
}