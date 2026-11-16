#include "LogHandler.h"
#include "Shared.h"

#include <cstdarg>
#include <cstdio>
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
        {
            if (entry.is_regular_file() &&
                std::regex_match(entry.path().filename().string(), filenamePattern))
                matches.push_back(entry.path());
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