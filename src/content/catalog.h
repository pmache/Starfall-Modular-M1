#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace sf {
struct Definition { std::string id, name, source; };
struct EnemyDef : Definition {
    int sprite = 0, hp = 22, score = 100;
    bool canDive = true;
};
struct Spawn { std::string enemy; float x = 0, y = 0; bool carrier = false; };
struct WaveDef : Definition {
    float diveInterval = 2, shotSpeed = 85, fireMin = 1.85f, fireMax = 4.05f;
    std::vector<Spawn> spawns;
};
struct BossDef : Definition {
    int hp = 420, score = 5000;
    float fireInterval = 0.72f, rageThreshold = 0.5f, rageMultiplier = 0.72f;
    float shotSpeed = 96, rageShotSpeed = 115, orbitAmplitude = 58, orbitSpeed = 0.72f;
};
enum class StepType { Wave, Wait, Boss };
struct Step { StepType type = StepType::Wait; std::string ref; float seconds = 0; };
struct SectorDef : Definition { std::vector<Step> steps; };
struct CampaignDef : Definition {
    std::vector<std::string> sectors;
    float transitionSeconds = 2.4f;
};
struct Catalog {
    std::filesystem::path root;
    std::string defaultCampaign;
    std::map<std::string, EnemyDef> enemies;
    std::map<std::string, WaveDef> waves;
    std::map<std::string, BossDef> bosses;
    std::map<std::string, SectorDef> sectors;
    std::map<std::string, CampaignDef> campaigns;
};
// All files are validated before a Catalog is returned. No partial load/fallback.
Catalog loadCatalog(const std::filesystem::path& root);
std::string describe(const Catalog& catalog);
}
