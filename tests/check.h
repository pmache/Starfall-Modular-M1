#pragma once
#include <cstdio>
#include <stdexcept>
namespace sf::test {
inline int checks = 0;
inline void check(bool pass, const char* expression, int line) {
    ++checks;
    if (!pass) throw std::runtime_error("FAIL line " + std::to_string(line) + ": " + expression);
}
}
#define CHECK(expression) ::sf::test::check((expression), #expression, __LINE__)
