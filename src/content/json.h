#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

// Small strict JSON reader. No runtime or build-time third-party dependency.
// Resource limits, duplicate keys and non-finite numbers are rejected.
namespace sf::json {
struct Value {
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> data;
    template<class T> bool is() const { return std::holds_alternative<T>(data); }
    template<class T> const T& get() const { return std::get<T>(data); }
};
Value parse(const std::string& input, const std::string& source);
}
