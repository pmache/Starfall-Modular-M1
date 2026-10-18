#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace sf {
std::filesystem::path executableDirectory();
std::vector<std::string> commandArguments(int argc, char** argv);
void reportError(const std::string& error, bool graphical);
}
