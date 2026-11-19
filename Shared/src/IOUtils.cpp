#include "IOUtils.h"

#include "LogHandler.h"
#include "ThreadPool.h"

#include <fstream>
#include <iterator>
#include <utility>

namespace IOUtils
{
    std::vector<std::filesystem::path> FindMatchingFiles(const std::filesystem::path &folder,
        const std::regex &filenamePattern)
    {
        std::vector<std::filesystem::path> matches;

        for (const auto &entry : std::filesystem::recursive_directory_iterator(folder))
            if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), filenamePattern))
                matches.push_back(entry.path());

        return matches;
    }

    std::vector<std::filesystem::path> FindMatchingFiles_Parallel(const std::filesystem::path &folder,
        const std::regex &filenamePattern)
    {
        std::vector<std::filesystem::path> files;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(folder))
            if (entry.is_regular_file())
                files.push_back(entry.path());

        std::size_t chunkCount;
        auto tasks = StaticThreadPool().For_Parallel(
            [&files, &filenamePattern](std::size_t, std::size_t start, std::size_t end)
            {
                std::vector<std::filesystem::path> matches;
                for (std::size_t i = start; i < end; ++i)
                    if (std::regex_match(files[i].filename().string(), filenamePattern))
                        matches.push_back(files[i]);

                return matches;
            }, files.size());

        std::vector<std::filesystem::path> matches;
        for (auto &task : tasks)
        {
            auto chunkMatches = task.get();
            matches.insert(matches.end(), std::make_move_iterator(chunkMatches.begin()),
                std::make_move_iterator(chunkMatches.end()));
        }

        return matches;
    }

    bool ReadFileToEnd(const char *filename, std::string &outContents)
    {
        if (filename == nullptr)
        {
            LOG_ERROR("Cannot read file: filename is null.\n");
            return false;
        }

        const char *sourcePath = filename;
        std::ifstream file(sourcePath);
        if (!file)
        {
            LOG_ERROR("Failed to open file: %s\n", sourcePath);
            return false;
        }

        std::string contents{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
        if (file.bad())
        {
            LOG_ERROR("Cannot read file: %s\n", sourcePath);
            return false;
        }

        outContents = std::move(contents);
        return true;
    }
}
