#pragma once

#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

inline bool AssetFileExists(const std::string& path) {
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline std::string ExecutableDirectory() {
    char buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return std::string();
    std::string path(buffer, length);
    const std::size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

inline std::string ResolveAssetPath(const char* relativePath) {
    const std::string relative(relativePath ? relativePath : "");
    const std::string executable = ExecutableDirectory();
    const std::string candidates[] = {
        executable.empty() ? std::string() : executable + "\\" + relative,
        executable.empty() ? std::string() : executable + "\\..\\..\\ConfigInicial\\" + relative,
        ".\\" + relative,
        relative
    };
    for (const std::string& candidate : candidates) {
        if (!candidate.empty() && AssetFileExists(candidate)) return candidate;
    }
    return relative;
}
