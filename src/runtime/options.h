#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace sf {
struct Options {
    std::filesystem::path contentRoot;
    std::string campaign, sector;
    bool help = false, validate = false, selfTest = false, grab = false, renderTest = false, simulate = false;
};
Options parseOptions(const std::vector<std::string>& arguments, const std::filesystem::path& executableDir);
const char* usage();
}
