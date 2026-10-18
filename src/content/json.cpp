#include "json.h"
#include <cmath>
#include <locale>
#include <sstream>
#include <utility>

namespace sf::json {
namespace {
class Reader {
    const std::string& text;
    const std::string& source;
    std::size_t at = 0;
    std::size_t nodes = 0;
    [[noreturn]] void fail(const std::string& message) const {
        std::size_t line = 1, column = 1;
        for (std::size_t i = 0; i < at && i < text.size(); ++i) {
            if (text[i] == '\n') { ++line; column = 1; } else ++column;
        }
        throw std::runtime_error(source + ":" + std::to_string(line) + ":" +
            std::to_string(column) + ": " + message);
    }
    char peek() const { return at < text.size() ? text[at] : '\0'; }
    void white() { while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r') ++at; }
    bool take(char c) { if (peek() != c) return false; ++at; return true; }
    void expect(char c) { if (!take(c)) fail(std::string("expected '") + c + "'"); }
    void literal(const char* token) { for (; *token; ++token) expect(*token); }
    unsigned hex4() {
        unsigned result = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = peek();
            unsigned n;
            if (c >= '0' && c <= '9') n = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') n = static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') n = static_cast<unsigned>(c - 'A' + 10);
            else fail("invalid Unicode escape");
            ++at; result = result * 16 + n;
        }
        return result;
    }
    void utf8(std::string& out, unsigned cp) {
        if (cp <= 0x7f) out.push_back(static_cast<char>(cp));
        else if (cp <= 0x7ff) {
            out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 63)));
        } else if (cp <= 0xffff) {
            out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63)));
            out.push_back(static_cast<char>(0x80 | (cp & 63)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 63)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63)));
            out.push_back(static_cast<char>(0x80 | (cp & 63)));
        }
    }
    std::string string() {
        expect('"');
        std::string out;
        while (at < text.size()) {
            const unsigned char c = static_cast<unsigned char>(text[at++]);
            if (c == '"') return out;
            if (c < 32) fail("control character in string");
            if (c >= 128) {
                unsigned cp = 0; int count = 0; unsigned minimum = 0;
                if (c >= 0xc2 && c <= 0xdf) { cp = c & 31; count = 1; minimum = 0x80; }
                else if (c >= 0xe0 && c <= 0xef) { cp = c & 15; count = 2; minimum = 0x800; }
                else if (c >= 0xf0 && c <= 0xf4) { cp = c & 7; count = 3; minimum = 0x10000; }
                else fail("invalid UTF-8 lead byte");
                for (int i = 0; i < count; ++i) {
                    if (at >= text.size()) fail("truncated UTF-8");
                    const unsigned char next = static_cast<unsigned char>(text[at++]);
                    if ((next & 0xc0) != 0x80) fail("invalid UTF-8 continuation");
                    cp = (cp << 6) | (next & 63);
                }
                if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) fail("invalid Unicode scalar");
                utf8(out, cp); continue;
            }
            if (c != '\\') { out.push_back(static_cast<char>(c)); continue; }
            if (at == text.size()) fail("unterminated escape");
            switch (text[at++]) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                unsigned cp = hex4();
                if (cp >= 0xd800 && cp <= 0xdbff) {
                    expect('\\'); expect('u');
                    const unsigned low = hex4();
                    if (low < 0xdc00 || low > 0xdfff) fail("missing low surrogate");
                    cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                } else if (cp >= 0xdc00 && cp <= 0xdfff) fail("unexpected low surrogate");
                utf8(out, cp); break;
            }
            default: fail("invalid string escape");
            }
        }
        fail("unterminated string");
    }
    Value number() {
        const std::size_t start = at;
        take('-');
        if (!take('0')) {
            if (peek() < '1' || peek() > '9') fail("expected number");
            while (peek() >= '0' && peek() <= '9') ++at;
        }
        if (take('.')) {
            if (peek() < '0' || peek() > '9') fail("missing fractional digits");
            while (peek() >= '0' && peek() <= '9') ++at;
        }
        if (take('e') || take('E')) {
            if (!take('+')) take('-');
            if (peek() < '0' || peek() > '9') fail("missing exponent digits");
            while (peek() >= '0' && peek() <= '9') ++at;
        }
        double n = 0;
        std::istringstream stream(text.substr(start, at - start));
        stream.imbue(std::locale::classic());
        if (!(stream >> n) || !std::isfinite(n)) fail("number is outside finite range");
        return {n};
    }
    Value value(int depth) {
        if (depth > 48 || ++nodes > 100000) fail("JSON complexity limit exceeded");
        white();
        switch (peek()) {
        case '"': return {string()};
        case 'n': literal("null"); return {nullptr};
        case 't': literal("true"); return {true};
        case 'f': literal("false"); return {false};
        case '[': {
            ++at; white(); Value::Array array;
            if (take(']')) return {std::move(array)};
            do { array.push_back(value(depth + 1)); white(); if (take(']')) return {std::move(array)}; expect(','); } while (true);
        }
        case '{': {
            ++at; white(); Value::Object object;
            if (take('}')) return {std::move(object)};
            do {
                white(); const auto key = string(); white(); expect(':');
                auto item = value(depth + 1);
                if (!object.emplace(key, std::move(item)).second) fail("duplicate key '" + key + "'");
                white(); if (take('}')) return {std::move(object)}; expect(',');
            } while (true);
        }
        default: return number();
        }
    }
public:
    Reader(const std::string& input, const std::string& file) : text(input), source(file) {}
    Value read() {
        if (text.size() > 1024 * 1024) fail("JSON file exceeds 1 MiB");
        if (text.size() >= 3 && text.compare(0, 3, "\xef\xbb\xbf") == 0) at = 3;
        Value result = value(0); white();
        if (at != text.size()) fail("unexpected trailing data");
        return result;
    }
};
}
Value parse(const std::string& input, const std::string& source) { return Reader(input, source).read(); }
}
