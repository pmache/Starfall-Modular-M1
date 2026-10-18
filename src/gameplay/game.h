#pragma once
#include "campaign.h"
#include <cstdint>
#include <vector>

namespace sf {
struct Vec2 { float x = 0, y = 0; };
struct Rgba { unsigned char r = 0, g = 0, b = 0, a = 255; };
struct Rect { float x = 0, y = 0, width = 0, height = 0; };
inline constexpr Rgba White{255,255,255,255};
inline constexpr Rgba Red{230,41,55,255};
constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 270;
constexpr float PI_F = 3.14159265f;

enum class State { Playing, Paused, GameOver, Victory };

// Explicit input keeps simulation tests independent of the window/input backend.
struct Input {
    Vec2 move{};
    Vec2 mouseTarget{};
    bool mouseActive = false;
    bool firing = false;
};

struct Shot {
    Vec2 p{};
    Vec2 v{};
    float radius = 2.0f;
    int damage = 10;
    bool hostile = false;
    Rgba color = White;
};

struct Enemy {
    const EnemyDef* definition = nullptr;
    Vec2 p{};
    Vec2 home{};
    Vec2 diveStart{};
    float phase = 0.0f;
    float fireTimer = 1.0f;
    float diveTime = 0.0f;
    int hp = 20;
    int maxHp = 20;
    int kind = 0;
    bool diving = false;
    bool carrier = false;
};

struct Particle {
    Vec2 p{};
    Vec2 v{};
    float life = 0.0f;
    float maxLife = 0.0f;
    Rgba color = White;
};

struct Star {
    Vec2 p{};
    int layer = 0;
};

struct Explosion { Vec2 p{}; float age = 0; float size = 1; };

struct PowerUp {
    Vec2 p{};
    bool active = false;
    float age = 0.0f;
};

struct Boss {
    const BossDef* definition = nullptr;
    Vec2 p{SCREEN_W + 40.0f, SCREEN_H*0.5f};
    int hp = 420;
    int maxHp = 420;
    float age = 0.0f;
    float fireTimer = 1.0f;
    bool active = false;
};

struct Game {
    // Catalog must outlive Game; definitions are immutable during a run.
    const Catalog* catalog = nullptr;
    std::string selectedCampaign, selectedSector;
    CampaignRunner campaign;
    const WaveDef* waveDefinition = nullptr;
    bool bossDefeated = false;
    State state = State::Playing;
    Vec2 player{52.0f, SCREEN_H*0.5f};
    std::vector<Enemy> enemies;
    std::vector<Shot> shots;
    std::vector<Particle> particles;
    std::vector<Star> stars;
    std::vector<Explosion> explosions;
    PowerUp power;
    Boss boss;
    std::uint32_t rng = 0x51A7F11u;
    int hp = 100;
    int lives = 3;
    std::int64_t score = 0;
    int wave = 1;
    float time = 0.0f;
    float fireTimer = 0.0f;
    float invulnerable = 1.5f;
    float spreadTimer = 0.0f;
    float diveTimer = 2.2f;
    float phaseTimer = 0.0f;
    float waveBanner = 2.0f;
    float shake = 0.0f;
    float flash = 0.0f;
    bool betweenWaves = false;
    bool clearHostileShots = false;
};


float random01(Game& g);
bool hit(Vec2 a, float ar, Vec2 b, float br);
Vec2 screenToCanvas(Vec2 p, Rect destination);
Rect destinationRectFor(int width, int height);
void reset(Game& g, const Catalog& catalog, const std::string& campaign, const std::string& soloSector = {});
void reset(Game& g);
void startWave(Game& g, const WaveDef& wave);
void beginBoss(Game& g, const BossDef& boss);
void playerFire(Game& g);
void hostileShot(Game& g, Vec2 from, float speed, float angleOffset = 0);
void updateStars(Game& g, float dt);
void updateBoss(Game& g, float dt);
void updateShots(Game& g, float dt);
Input demoInput(const Game& g);
void controlState(Game& g, bool pause, bool restart, bool focused = true);
void tick(Game& g, float dt, const Input& input);
}
