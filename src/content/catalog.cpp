#include "catalog.h"
#include "json.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>

namespace sf {
namespace {
using J = json::Value;
[[noreturn]] void fail(const std::string& path, const std::string& message) {
    throw std::runtime_error(path + ": " + message);
}
const J::Object& object(const J& j, const std::string& path) {
    if (!j.is<J::Object>()) fail(path, "expected object");
    return j.get<J::Object>();
}
const J* field(const J& j, const std::string key, const std::string& path) {
    const auto& o = object(j, path);
    const auto it = o.find(key);
    if (it == o.end()) fail(path + "/" + key, "required field is missing");
    return &it->second;
}
bool has(const J& j, const std::string& key) { return j.get<J::Object>().count(key) != 0; }
void keys(const J& j, const std::string& path, std::initializer_list<const char*> allowed) {
    for (const auto& item : object(j, path)) {
        if (std::none_of(allowed.begin(), allowed.end(), [&](const char* k) { return item.first == k; }))
            fail(path + "/" + item.first, "unknown field (possible typo)");
    }
}
std::string str(const J& j, const std::string& path) {
    if (!j.is<std::string>()) fail(path, "expected string");
    return j.get<std::string>();
}
std::string text(const J& j, const std::string key, const std::string& path) {
    return str(*field(j, key, path), path + "/" + key);
}
float number(const J& j, const std::string key, const std::string& path, double low, double high) {
    const J& v = *field(j, key, path);
    if (!v.is<double>()) fail(path + "/" + key, "expected number, not string/bool");
    const double n = v.get<double>();
    if (n < low || n > high || !std::isfinite(n))
        fail(path + "/" + key, "expected number in [" + std::to_string(low) + ", " + std::to_string(high) + "]");
    return static_cast<float>(n);
}
int integer(const J& j, const std::string key, const std::string& path, int low, int high) {
    const auto& v = *field(j, key, path);
    number(j, key, path, static_cast<float>(low), static_cast<float>(high));
    const double n = v.get<double>();
    if (std::floor(n) != n) fail(path + "/" + key, "expected integer");
    return static_cast<int>(n);
}
bool boolean(const J& j, const std::string key, const std::string& path, bool fallback) {
    if (!has(j, key)) return fallback;
    const auto& v = *field(j, key, path);
    if (!v.is<bool>()) fail(path + "/" + key, "expected boolean");
    return v.get<bool>();
}
const J::Array* array(const J& j, const std::string key, const std::string& path, std::size_t low, std::size_t high) {
    const auto& v = *field(j, key, path);
    if (!v.is<J::Array>()) fail(path + "/" + key, "expected array");
    const auto& a = v.get<J::Array>();
    if (a.size() < low || a.size() > high) fail(path + "/" + key, "array size outside supported range");
    return &a;
}
bool slug(const std::string& s) {
    return !s.empty() && s.size() <= 48 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    });
}
void id(const std::string& s, const std::string& path) {
    const auto colon = s.find(':');
    if (colon == std::string::npos || !slug(s.substr(0, colon)) || !slug(s.substr(colon + 1)))
        fail(path, "expected ID such as base:patrol (lowercase ASCII, digits, _ or -)");
}
std::string ref(const J& j, const std::string key, const std::string& path) {
    auto s = text(j, key, path); id(s, path + "/" + key); return s;
}
void schema(const J& j, const std::string& path) { integer(j, "schema_version", path, 1, 1); }
void nameCheck(const std::string& s, const std::string& path) {
    // The existing bitmap font deliberately has no Unicode glyphs. Reject silent '?' rendering.
    if (s.empty() || s.size() > 32 || !std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == ' ' || c == ':' || c == '/' || c == '-' || c == '.' || c == '?';
    })) fail(path, "name must be 1..32 characters supported by the bitmap font: A-Z, 0-9, space, : / - . ?");
}
void base(Definition& d, const J& j, const std::string& path, const std::string& pack) {
    schema(j, path);
    d.id = ref(j, "id", path);
    if (d.id.substr(0, d.id.find(':')) != pack) fail(path + "/id", "ID must use this pack's namespace: " + pack);
    d.name = text(j, "name", path); nameCheck(d.name, path + "/name");
    d.source = path;
}
void notLink(const std::filesystem::path& p) {
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(p))) fail(p.u8string(), "symlinks inside content packs are not supported");
}
J read(const std::filesystem::path& path, std::uintmax_t& total) {
    notLink(path);
    const auto size = std::filesystem::file_size(path);
    if (size > 1024 * 1024 || (total += size) > 16 * 1024 * 1024) fail(path.u8string(), "content size limit exceeded");
    std::ifstream input(path, std::ios::binary);
    if (!input) fail(path.u8string(), "cannot open file");
    std::string text(static_cast<std::size_t>(size), '\0');
    input.read(text.data(), static_cast<std::streamsize>(size));
    if (!input || input.peek() != std::char_traits<char>::eof()) fail(path.u8string(), "file changed during read or could not be read completely");
    return json::parse(text, path.u8string());
}
template<class T> void insert(std::map<std::string, T>& map, T value) {
    const auto found = map.find(value.id);
    if (found != map.end()) fail(value.source + "/id", "duplicate ID " + value.id + "; first defined in " + found->second.source);
    const std::string key = value.id; map.emplace(key, std::move(value));
}
template<class T> void exists(const std::map<std::string, T>& map, const std::string& ref, const std::string& path) {
    if (!map.count(ref)) fail(path, "unresolved reference '" + ref + "'");
}
}
Catalog loadCatalog(const std::filesystem::path& inputRoot) {
    Catalog c;
    c.root = std::filesystem::absolute(inputRoot).lexically_normal();
    std::uintmax_t total = 0;
    const auto configFile = c.root / "game.json";
    const std::string configPath = configFile.u8string() + "#";
    const auto config = read(configFile, total);
    keys(config, configPath, {"schema_version", "default_campaign", "packs"});
    schema(config, configPath);
    c.defaultCampaign = ref(config, "default_campaign", configPath);
    const auto& packs = *array(config, "packs", configPath, 1, 32);
    std::set<std::string> seen;
    std::size_t fileCount = 0;
    for (const auto& value : packs) {
        const std::string pack = str(value, configPath + "/packs");
        if (!slug(pack) || !seen.insert(pack).second) fail(configPath + "/packs", "invalid or duplicate pack folder: " + pack);
        const auto folder = c.root / pack; notLink(folder);
        const auto manifest = folder / "pack.json";
        const std::string mp = manifest.u8string() + "#";
        const auto m = read(manifest, total);
        keys(m, mp, {"schema_version", "id", "name"}); schema(m, mp);
        if (text(m, "id", mp) != pack) fail(mp + "/id", "pack ID must match its folder name");
        nameCheck(text(m, "name", mp), mp + "/name");
        for (const std::string category : {"enemies", "waves", "bosses", "sectors", "campaigns"}) {
            const auto dir = folder / category;
            if (!std::filesystem::exists(dir)) continue;
            notLink(dir);
            std::vector<std::filesystem::path> paths;
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.path().extension() != ".json") continue;
                notLink(entry.path());
                if (!entry.is_regular_file()) fail(entry.path().u8string(), "expected JSON file, not directory");
                if (++fileCount > 4096) fail(dir.u8string(), "too many definitions");
                paths.push_back(entry.path());
            }
            std::sort(paths.begin(), paths.end());
            for (const auto& path : paths) {
                const auto j = read(path, total);
                const std::string p = path.u8string() + "#";
                if (category == "enemies") {
                    keys(j, p, {"schema_version", "id", "name", "sprite", "hp", "score", "can_dive"});
                    EnemyDef d; base(d, j, p, pack);
                    const std::string sprite = text(j, "sprite", p);
                    if (sprite == "hornet") d.sprite = 0;
                    else if (sprite == "manta") d.sprite = 1;
                    else if (sprite == "bomber") d.sprite = 2;
                    else fail(p + "/sprite", "supported sprites: hornet, manta, bomber");
                    d.hp = integer(j, "hp", p, 1, 100000);
                    d.score = integer(j, "score", p, 0, 1000000);
                    d.canDive = boolean(j, "can_dive", p, true);
                    insert(c.enemies, std::move(d));
                } else if (category == "waves") {
                    keys(j, p, {"schema_version", "id", "name", "dive_interval", "shot_speed", "fire_interval_min", "fire_interval_max", "spawns"});
                    WaveDef d; base(d, j, p, pack);
                    d.diveInterval = number(j, "dive_interval", p, 0.1, 120);
                    d.shotSpeed = number(j, "shot_speed", p, 1, 1000);
                    d.fireMin = number(j, "fire_interval_min", p, 0.1, 120);
                    d.fireMax = number(j, "fire_interval_max", p, 0.1, 120);
                    if (d.fireMax < d.fireMin) fail(p + "/fire_interval_max", "must be >= fire_interval_min");
                    const auto& spawns = *array(j, "spawns", p, 1, 128);
                    for (std::size_t i = 0; i < spawns.size(); ++i) {
                        const auto& s = spawns[i]; const auto sp = p + "/spawns/" + std::to_string(i);
                        keys(s, sp, {"enemy", "x", "y", "carrier"});
                        d.spawns.push_back({ref(s, "enemy", sp), number(s, "x", sp, 230, 455),
                            number(s, "y", sp, 35, 242), boolean(s, "carrier", sp, false)});
                    }
                    insert(c.waves, std::move(d));
                } else if (category == "bosses") {
                    keys(j, p, {"schema_version", "id", "name", "behavior", "hp", "score", "fire_interval", "rage_threshold", "rage_interval_multiplier", "shot_speed", "rage_shot_speed", "orbit_amplitude", "orbit_speed"});
                    BossDef d; base(d, j, p, pack);
                    if (text(j, "behavior", p) != "null_crown") fail(p + "/behavior", "supported behavior: null_crown");
                    d.hp = integer(j, "hp", p, 1, 1000000); d.score = integer(j, "score", p, 0, 1000000);
                    d.fireInterval = number(j, "fire_interval", p, 0.1, 120);
                    d.rageThreshold = number(j, "rage_threshold", p, 0, 1);
                    d.rageMultiplier = number(j, "rage_interval_multiplier", p, 0.1, 1);
                    d.shotSpeed = number(j, "shot_speed", p, 1, 1000);
                    d.rageShotSpeed = number(j, "rage_shot_speed", p, 1, 1000);
                    d.orbitAmplitude = number(j, "orbit_amplitude", p, 0, 70);
                    d.orbitSpeed = number(j, "orbit_speed", p, 0, 10);
                    insert(c.bosses, std::move(d));
                } else if (category == "sectors") {
                    keys(j, p, {"schema_version", "id", "name", "steps"});
                    SectorDef d; base(d, j, p, pack);
                    const auto& steps = *array(j, "steps", p, 1, 512);
                    for (std::size_t i = 0; i < steps.size(); ++i) {
                        const auto& s = steps[i]; const auto sp = p + "/steps/" + std::to_string(i);
                        object(s, sp);
                        const auto type = text(s, "type", sp);
                        Step step;
                        if (type == "wait") {
                            keys(s, sp, {"type", "seconds"}); step.type = StepType::Wait;
                            step.seconds = number(s, "seconds", sp, 0, 600);
                        } else if (type == "wave" || type == "boss") {
                            keys(s, sp, {"type", "ref", "complete_when"});
                            step.type = type == "wave" ? StepType::Wave : StepType::Boss;
                            step.ref = ref(s, "ref", sp);
                            if (text(s, "complete_when", sp) != (type == "wave" ? "enemies_cleared" : "boss_defeated"))
                                fail(sp + "/complete_when", "completion condition does not match step type");
                        } else fail(sp + "/type", "supported steps: wave, wait, boss");
                        d.steps.push_back(std::move(step));
                    }
                    insert(c.sectors, std::move(d));
                } else {
                    keys(j, p, {"schema_version", "id", "name", "sectors", "sector_transition_seconds"});
                    CampaignDef d; base(d, j, p, pack);
                    d.transitionSeconds = number(j, "sector_transition_seconds", p, 0, 60);
                    const auto& sectors = *array(j, "sectors", p, 1, 1024);
                    for (std::size_t i = 0; i < sectors.size(); ++i) {
                        auto s = str(sectors[i], p + "/sectors/" + std::to_string(i));
                        id(s, p + "/sectors/" + std::to_string(i)); d.sectors.push_back(std::move(s));
                    }
                    insert(c.campaigns, std::move(d));
                }
            }
        }
    }
    for (const auto& item : c.waves) {
        const auto& d = item.second;
        for (std::size_t i = 0; i < d.spawns.size(); ++i)
            exists(c.enemies, d.spawns[i].enemy, d.source + "/spawns/" + std::to_string(i) + "/enemy");
    }
    for (const auto& item : c.sectors) {
        const auto& d = item.second;
        for (std::size_t i = 0; i < d.steps.size(); ++i) {
            const auto& s = d.steps[i]; const auto p = d.source + "/steps/" + std::to_string(i) + "/ref";
            if (s.type == StepType::Wave) exists(c.waves, s.ref, p);
            if (s.type == StepType::Boss) exists(c.bosses, s.ref, p);
        }
    }
    for (const auto& item : c.campaigns) {
        const auto& d = item.second;
        for (std::size_t i = 0; i < d.sectors.size(); ++i)
            exists(c.sectors, d.sectors[i], d.source + "/sectors/" + std::to_string(i));
    }
    exists(c.campaigns, c.defaultCampaign, configPath + "/default_campaign");
    return c;
}
std::string describe(const Catalog& c) {
    std::ostringstream out;
    out << "content validation: OK\nDefault campaign: " << c.defaultCampaign << "\n";
    for (const auto& item : c.campaigns) out << "CAMPAIGN " << item.first << " | " << item.second.name << " | " << item.second.sectors.size() << " sectors\n";
    for (const auto& item : c.sectors) out << "SECTOR   " << item.first << " | " << item.second.steps.size() << " steps\n";
    out << "Definitions: " << c.enemies.size() << " enemies, " << c.waves.size() << " waves, " << c.bosses.size() << " bosses\n";
    return out.str();
}
}
