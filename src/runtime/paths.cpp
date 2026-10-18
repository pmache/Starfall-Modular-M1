#include "paths.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#include <shellapi.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace sf {
std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::vector<wchar_t> path(32768);
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!size || size >= path.size()) throw std::runtime_error("Cannot determine executable directory");
    return std::filesystem::path(std::wstring(path.data(), size)).parent_path();
#elif defined(__linux__)
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> path(size);
    if (_NSGetExecutablePath(path.data(), &size) != 0) throw std::runtime_error("Cannot determine executable directory");
    return std::filesystem::weakly_canonical(path.data()).parent_path();
#else
#error Add executableDirectory() for this platform; do not silently use the working directory.
#endif
}
std::vector<std::string> commandArguments(int argc, char** argv) {
#ifdef _WIN32
    (void)argc; (void)argv;
    int count = 0;
    wchar_t** wide = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!wide) throw std::runtime_error("Cannot decode command line");
    std::vector<std::string> result;
    try {
        for (int i = 1; i < count; ++i) {
            const int size = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
            if (!size) throw std::runtime_error("Cannot decode command line argument");
            std::string value(static_cast<std::size_t>(size), '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, value.data(), size, nullptr, nullptr);
            value.pop_back(); result.push_back(std::move(value));
        }
    } catch (...) { LocalFree(wide); throw; }
    LocalFree(wide); return result;
#else
    return {argv + 1, argv + argc};
#endif
}
void reportError(const std::string& error, bool graphical) {
    std::fprintf(stderr, "STARFALL ERROR: %s\n", error.c_str());
    try {
        std::ofstream log(executableDirectory() / "starfall-content-error.log", std::ios::binary);
        log << error << '\n';
    } catch (...) { /* stderr still contains the complete diagnostic */ }
#ifdef _WIN32
    if (graphical) {
        const int size = MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, nullptr, 0);
        if (size > 0) {
            std::wstring message(static_cast<std::size_t>(size), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, message.data(), size);
            MessageBoxW(nullptr, message.c_str(), L"Starfall - content error", MB_ICONERROR | MB_OK);
        }
    }
#else
    (void)graphical;
#endif
}
}
