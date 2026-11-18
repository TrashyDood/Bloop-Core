#pragma once

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

namespace IOUtils
{
    bool ReadFileToEnd(const char *filename, std::string &outContents);
    std::vector<std::filesystem::path> FindMatchingFiles(const std::filesystem::path &folder,
        const std::regex &filenamePattern);
    std::vector<std::filesystem::path> FindMatchingFiles_Parallel(const std::filesystem::path &folder,
        const std::regex &filenamePattern);
    inline std::filesystem::path CurrentDirectory() { return std::filesystem::current_path(); }

    std::string GetRelativeName(const std::filesystem::path &file,
        const std::filesystem::path &root, const std::filesystem::path &name);
}
