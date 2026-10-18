#include "game.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sf {
float random01(Game &g) {
    g.rng = g.rng * 1664525u + 1013904223u;
    return static_cast<float>((g.rng >> 8) & 0xFFFFFFu) / 16777215.0f;
}

float distanceSq(Vec2 a, Vec2 b) {
    const float x = a.x - b.x;
    const float y = a.y - b.y;
    return x*x + y*y;
}

bool hit(Vec2 a, float ar, Vec2 b, float br) {
    const float r = ar + br;
    return distanceSq(a, b) <= r*r;
}

Vec2 screenToCanvas(Vec2 p, Rect destination) {
    return {(p.x - destination.x)*SCREEN_W/destination.width,
            (p.y - destination.y)*SCREEN_H/destination.height};
}


Vec2 normalizeOrLeft(Vec2 v) {
    const float length = std::sqrt(v.x*v.x + v.y*v.y);
    return length > 0.001f ? Vec2{v.x/length, v.y/length} : Vec2{-1.0f, 0.0f};
}

void burst(Game &g, Vec2 p, Rgba color, int count, float speed = 55.0f) {
    for (int i = 0; i < count; ++i) {
        const float a = random01(g) * PI_F * 2.0f;
        const float s = speed * (0.35f + random01(g) * 0.65f);
        const float life = 0.25f + random01(g) * 0.4f;
        g.particles.push_back({p, {std::cos(a)*s, std::sin(a)*s}, life, life, color});
    }
}

void addEnemy(Game &g, const Spawn& spawn, int index) {
    const EnemyDef& def = g.catalog->enemies.at(spawn.enemy);
    Enemy e;
    e.definition = &def;
    e.p = {SCREEN_W + 18.0f + index*5.0f, spawn.y + ((index & 1) ? 42.0f : -42.0f)};
    e.home = {spawn.x, spawn.y};
    e.phase = index * 0.71f;
    e.fireTimer = 1.1f + random01(g) * 2.0f;
    e.hp = e.maxHp = def.hp;
    e.kind = def.sprite;
    e.carrier = spawn.carrier;
    g.enemies.push_back(e);
}

void startWave(Game &g, const WaveDef& wave) {
    g.waveDefinition = &wave;
    g.enemies.clear();
    g.boss.active = false;
    g.bossDefeated = false;
    g.betweenWaves = false;
    g.waveBanner = 2.0f;
    g.diveTimer = 2.0f;
    for (std::size_t i = 0; i < wave.spawns.size(); ++i) addEnemy(g, wave.spawns[i], static_cast<int>(i));
}

void enterCurrentStep(Game& g) {
    const Step& step = g.campaign.step();
    g.bossDefeated = false;
    g.betweenWaves = step.type == StepType::Wait;
    if (step.type == StepType::Wave) {
        g.wave = 0;
        for (std::size_t i = 0; i <= g.campaign.stepIndex(); ++i)
            if (g.campaign.sector().steps[i].type == StepType::Wave) ++g.wave;
        startWave(g, g.catalog->waves.at(step.ref));
    } else if (step.type == StepType::Boss) beginBoss(g, g.catalog->bosses.at(step.ref));
    else g.waveBanner = 0;
}

void reset(Game &g, const Catalog& catalog, const std::string& campaign, const std::string& soloSector) {
    Game next;
    next.catalog = &catalog;
    next.selectedCampaign = campaign;
    next.selectedSector = soloSector;
    next.campaign.begin(catalog, campaign, soloSector);
    for (int i = 0; i < 84; ++i)
        next.stars.push_back({{random01(next)*SCREEN_W, random01(next)*SCREEN_H}, i%3});
    enterCurrentStep(next);
    g = std::move(next);
}
void reset(Game& g) {
    if (!g.catalog) throw std::logic_error("Game has no content catalog");
    reset(g, *g.catalog, g.selectedCampaign, g.selectedSector);
}

void spawnPlayerShot(Game &g, float ySpeed) {
    g.shots.push_back({{g.player.x + 10.0f, g.player.y + ySpeed*0.025f},
                       {220.0f, ySpeed}, 2.2f, 13, false, Rgba{85, 245, 255, 255}});
}

void playerFire(Game &g) {
    spawnPlayerShot(g, 0.0f);
    if (g.spreadTimer > 0.0f) {
        spawnPlayerShot(g, -64.0f);
        spawnPlayerShot(g, 64.0f);
    }
    g.fireTimer = g.spreadTimer > 0.0f ? 0.12f : 0.18f;
}

void hostileShot(Game &g, Vec2 from, float speed, float angleOffset) {
    Vec2 d = normalizeOrLeft({g.player.x - from.x, g.player.y - from.y});
    const float a = std::atan2(d.y, d.x) + angleOffset;
    g.shots.push_back({from, {std::cos(a)*speed, std::sin(a)*speed},
                       2.5f, 16, true, Rgba{255, 99, 124, 255}});
}

void hurtPlayer(Game &g, int damage) {
    if (g.invulnerable > 0.0f || g.state != State::Playing) return;
    g.hp -= damage;
    g.invulnerable = 0.75f;
    g.shake = 0.28f;
    g.flash = 0.16f;
    burst(g, g.player, Rgba{255, 143, 72, 255}, 14, 85.0f);
    if (g.hp > 0) return;

    --g.lives;
    g.explosions.push_back({g.player, 0, 1.2f});
    if (g.lives <= 0) {
        g.hp = 0;
        g.state = State::GameOver;
        return;
    }
    g.hp = 100;
    g.player = {52.0f, SCREEN_H*0.5f};
    g.invulnerable = 2.2f;
    g.spreadTimer = 0.0f;
    // updateShots may be holding a reference into this vector. Defer erasure.
    g.clearHostileShots = true;
}

void killEnemy(Game &g, std::size_t index) {
    const Enemy e = g.enemies[index];
    g.score += e.definition->score + (e.diving ? 75 : 0);
    burst(g, e.p, e.kind == 2 ? Rgba{255, 91, 160, 255} : Rgba{255, 188, 78, 255}, 13);
    g.explosions.push_back({e.p, 0, e.kind == 2 ? 1.0f : 0.75f});
    g.shake = std::max(g.shake, 0.1f);
    if (e.carrier && !g.power.active) {
        g.power = {e.p, true, 0.0f};
    }
    g.enemies.erase(g.enemies.begin() + static_cast<std::ptrdiff_t>(index));
}

void updateStars(Game &g, float dt) {
    for (Star &s : g.stars) {
        s.p.x -= (9.0f + s.layer*13.0f)*dt;
        if (s.p.x < 0.0f) {
            s.p.x += SCREEN_W;
            s.p.y = random01(g)*SCREEN_H;
        }
    }
}

void updateParticles(Game &g, float dt) {
    for (Explosion &e : g.explosions) e.age += dt;
    g.explosions.erase(std::remove_if(g.explosions.begin(), g.explosions.end(),
        [](const Explosion &e) { return e.age >= 0.5f; }), g.explosions.end());
    for (Particle &p : g.particles) {
        p.life -= dt;
        p.p.x += p.v.x*dt;
        p.p.y += p.v.y*dt;
        p.v.x *= 1.0f - 2.2f*dt;
        p.v.y *= 1.0f - 2.2f*dt;
    }
    g.particles.erase(std::remove_if(g.particles.begin(), g.particles.end(),
                                     [](const Particle &p) { return p.life <= 0.0f; }),
                      g.particles.end());
}

void beginBoss(Game &g, const BossDef& def) {
    g.boss = Boss{};
    g.boss.definition = &def;
    g.boss.hp = g.boss.maxHp = def.hp;
    g.bossDefeated = false;
    g.boss.active = true;
    g.betweenWaves = false;
    g.waveBanner = 2.4f;
    g.power.active = false;
}

void updateBoss(Game &g, float dt) {
    Boss &b = g.boss;
    const BossDef& def = *b.definition;
    b.age += dt;
    b.p.x += (SCREEN_W - 68.0f - b.p.x) * std::min(1.0f, dt*1.8f);
    b.p.y = SCREEN_H*0.5f + std::sin(b.age*def.orbitSpeed)*def.orbitAmplitude;
    b.fireTimer -= dt;
    if (b.p.x > SCREEN_W - 48.0f || b.fireTimer > 0.0f) return;

    const float rage = b.hp < b.maxHp*def.rageThreshold ? def.rageMultiplier : 1.0f;
    b.fireTimer = def.fireInterval*rage;
    hostileShot(g, {b.p.x - 15.0f, b.p.y - 22.0f}, def.shotSpeed, -0.18f);
    hostileShot(g, {b.p.x - 15.0f, b.p.y + 22.0f}, def.shotSpeed, 0.18f);
    if (b.hp < b.maxHp*def.rageThreshold) hostileShot(g, {b.p.x - 19.0f, b.p.y}, def.rageShotSpeed);
}

void updateEnemies(Game &g, float dt) {
    if (!g.waveDefinition) return;
    const WaveDef& def = *g.waveDefinition;
    g.diveTimer -= dt;
    if (g.diveTimer <= 0.0f && !g.enemies.empty()) {
        std::vector<std::size_t> ready;
        for (std::size_t i = 0; i < g.enemies.size(); ++i)
            if (g.enemies[i].definition->canDive && !g.enemies[i].diving && g.enemies[i].p.x < SCREEN_W - 20.0f) ready.push_back(i);
        if (!ready.empty()) {
            const std::size_t pick = ready[static_cast<std::size_t>(random01(g)*ready.size()) % ready.size()];
            Enemy &e = g.enemies[pick];
            e.diving = true;
            e.diveTime = 0.0f;
            e.diveStart = e.p;
        }
        g.diveTimer = def.diveInterval;
    }

    for (Enemy &e : g.enemies) {
        e.phase += dt;
        e.fireTimer -= dt;
        if (e.diving) {
            e.diveTime += dt;
            const float t = e.diveTime / 2.55f;
            const float side = e.diveStart.y < SCREEN_H*0.5f ? 1.0f : -1.0f;
            e.p.x = e.diveStart.x + t*(-35.0f - e.diveStart.x);
            e.p.y = e.diveStart.y + side*std::sin(t*PI_F)*72.0f
                    + (g.player.y - e.diveStart.y)*t*0.22f;
            if (t >= 1.0f) {
                e.diving = false;
                e.p = {SCREEN_W + 18.0f, e.home.y};
            }
        } else {
            const float tx = e.home.x + std::sin(g.time*1.3f + e.phase)*4.0f;
            const float ty = e.home.y + std::cos(g.time*1.7f + e.phase)*2.5f;
            const float settle = std::min(1.0f, dt*2.7f);
            e.p.x += (tx - e.p.x)*settle;
            e.p.y += (ty - e.p.y)*settle;
        }

        if (e.fireTimer <= 0.0f && e.p.x > 220.0f && e.p.x < SCREEN_W - 12.0f) {
            hostileShot(g, {e.p.x - 8.0f, e.p.y}, def.shotSpeed);
            e.fireTimer = def.fireMin + random01(g)*(def.fireMax-def.fireMin);
        }
    }
}

void updateShots(Game &g, float dt) {
    if (g.clearHostileShots) {
        g.shots.erase(std::remove_if(g.shots.begin(), g.shots.end(),
            [](const Shot &s) { return s.hostile; }), g.shots.end());
        g.clearHostileShots = false;
    }
    for (Shot &s : g.shots) {
        s.p.x += s.v.x*dt;
        s.p.y += s.v.y*dt;
    }

    for (std::size_t si = 0; si < g.shots.size();) {
        Shot &s = g.shots[si];
        bool remove = s.p.y < -16.0f || s.p.y > SCREEN_H + 16.0f
                   || s.p.x < -16.0f || s.p.x > SCREEN_W + 16.0f;
        if (!remove && s.hostile && hit(s.p, s.radius, g.player, 7.0f)) {
            hurtPlayer(g, s.damage);
            remove = true;
        }
        if (!remove && !s.hostile) {
            if (g.boss.active && hit(s.p, s.radius, {g.boss.p.x-10, g.boss.p.y}, 42.0f)) {
                g.boss.hp -= s.damage;
                g.score += 5;
                burst(g, s.p, Rgba{111, 247, 255, 255}, 3, 25.0f);
                remove = true;
                if (g.boss.hp <= 0) {
                    g.boss.hp = 0;
                    g.score += g.boss.definition->score;
                    burst(g, g.boss.p, Rgba{255, 191, 68, 255}, 55, 120.0f);
                    g.explosions.push_back({g.boss.p, 0, 2.0f});
                    g.boss.active = false;
                    g.bossDefeated = true;
                    g.clearHostileShots = true;
                    g.shake = 0.8f;
                    g.flash = 0.4f;
                }
            } else {
                for (std::size_t ei = 0; ei < g.enemies.size(); ++ei) {
                    if (!hit(s.p, s.radius, g.enemies[ei].p, 8.0f)) continue;
                    g.enemies[ei].hp -= s.damage;
                    burst(g, s.p, Rgba{107, 239, 255, 255}, 3, 24.0f);
                    if (g.enemies[ei].hp <= 0) killEnemy(g, ei);
                    remove = true;
                    break;
                }
            }
        }
        if (remove) g.shots.erase(g.shots.begin() + static_cast<std::ptrdiff_t>(si));
        else ++si;
        if (g.clearHostileShots) {
            g.shots.erase(std::remove_if(g.shots.begin(), g.shots.end(),
                [](const Shot &shot) { return shot.hostile; }), g.shots.end());
            g.clearHostileShots = false;
            break;
        }
        if (g.state != State::Playing) break;
    }
}

Input demoInput(const Game &g) {
    return {{std::sin(g.time*0.47f)*0.2f, std::sin(g.time*1.15f)}, {}, false, true};
}

void updatePlaying(Game &g, float dt, const Input &input) {
    if (g.state != State::Playing) return;
    g.time += dt;
    g.fireTimer -= dt;
    g.invulnerable -= dt;
    g.spreadTimer -= dt;
    g.waveBanner -= dt;
    g.shake -= dt;
    g.flash -= dt;
    updateStars(g, dt);
    updateParticles(g, dt);

    Vec2 move = input.move;
    const float len = std::sqrt(move.x*move.x + move.y*move.y);
    if (len > 0.0f) { move.x /= len; move.y /= len; }
    if (input.mouseActive && len == 0.0f) g.player = input.mouseTarget;
    else {
        g.player.x += move.x*125.0f*dt;
        g.player.y += move.y*125.0f*dt;
    }
    g.player.x = std::clamp(g.player.x, 16.0f, 205.0f);
    g.player.y = std::clamp(g.player.y, 30.0f, SCREEN_H - 19.0f);
    if (input.firing && g.fireTimer <= 0.0f) playerFire(g);

    if (g.boss.active) updateBoss(g, dt);
    else if (!g.betweenWaves) updateEnemies(g, dt);
    updateShots(g, dt);
    if (g.state != State::Playing) return;

    for (const Enemy &e : g.enemies) {
        if (hit(e.p, 8.0f, g.player, 7.0f)) {
            hurtPlayer(g, 34);
            break;
        }
    }
    if (g.state != State::Playing) return;

    if (g.power.active) {
        g.power.age += dt;
        g.power.p.x -= 31.0f*dt;
        g.power.p.y += std::sin(g.power.age*5.0f)*12.0f*dt;
        if (hit(g.power.p, 7.0f, g.player, 8.0f)) {
            g.power.active = false;
            g.spreadTimer = 9.0f;
            g.hp = std::min(100, g.hp + 25);
            g.score += 250;
            burst(g, g.player, Rgba{108, 255, 174, 255}, 22, 75.0f);
        } else if (g.power.p.x < -12.0f) {
            g.power.active = false;
        }
    }

    switch (g.campaign.update(dt, g.enemies.empty(), g.bossDefeated)) {
    case RunAction::StartStep:
        enterCurrentStep(g);
        break;
    case RunAction::BeginTransition:
        g.shots.clear();
        g.power.active = false;
        g.betweenWaves = true;
        g.waveBanner = 0;
        g.invulnerable = std::max(g.invulnerable, 1.5f);
        break;
    case RunAction::StartSector:
        // Carry hull, lives, score, weapon timer and player position forward.
        g.shots.clear();
        g.wave = 0;
        g.invulnerable = std::max(g.invulnerable, 1.5f);
        enterCurrentStep(g);
        break;
    case RunAction::Victory:
        g.state = State::Victory;
        g.shots.clear();
        g.betweenWaves = false;
        g.bossDefeated = false;
        break;
    case RunAction::None: break;
    }
}

Rect destinationRectFor(int width, int height) {
    int scale = std::min(width/SCREEN_W, height/SCREEN_H);
    if (scale < 1) scale = 1;
    const float w = static_cast<float>(SCREEN_W*scale);
    const float h = static_cast<float>(SCREEN_H*scale);
    return {(width-w)*0.5f, (height-h)*0.5f, w, h};
}

void controlState(Game &g, bool pause, bool restart, bool focused) {
    if (!focused && g.state == State::Playing) g.state = State::Paused;
    else if (focused && pause) {
        if (g.state == State::Playing) g.state = State::Paused;
        else if (g.state == State::Paused) g.state = State::Playing;
    }
    if (focused && restart && (g.state == State::Victory || g.state == State::GameOver)) reset(g);
}

void tick(Game &g, float dt, const Input &input) {
    if (!std::isfinite(dt)) return;
    dt = std::clamp(dt, 0.0f, 0.05f);
    if (g.state == State::Playing) updatePlaying(g, dt, input);
    else if (g.state != State::Paused) {
        updateStars(g, dt);
        updateParticles(g, dt);
        g.shake = std::max(0.0f, g.shake - dt);
        g.flash = std::max(0.0f, g.flash - dt);
    }
}


}
