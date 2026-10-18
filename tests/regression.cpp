#include "regression.h"
#include "check.h"
#include "../src/gameplay/game.h"
#include <algorithm>
#include <cmath>
namespace sf {
int runRegressionTests(const Catalog& catalog) {
    test::checks = 0;
    CHECK(hit({0, 0}, 3.0f, {5, 0}, 2.0f));
    CHECK(!hit({0, 0}, 2.0f, {5, 0}, 2.0f));
    Game g;
    reset(g, catalog, "base:classic");
    CHECK(g.wave == 1 && g.enemies.size() == 9);
    CHECK(g.player.x < SCREEN_W*0.5f);
    CHECK(std::all_of(g.enemies.begin(), g.enemies.end(), [&g](const Enemy &e) {
        return e.home.x > g.player.x && e.p.x > SCREEN_W;
    }));
    CHECK(std::count_if(g.enemies.begin(), g.enemies.end(), [](const Enemy &e) { return e.carrier; }) == 1);
    playerFire(g);
    CHECK(!g.shots.empty() && g.shots.front().v.x > 0.0f);
    g.shots.clear();
    hostileShot(g, {400.0f, g.player.y}, 80.0f);
    CHECK(g.shots.front().v.x < 0.0f);
    g.stars = {{{100.0f, 70.0f}, 0}};
    updateStars(g, 1.0f);
    CHECK(g.stars.front().p.x < 100.0f && g.stars.front().p.y == 70.0f);
    const Vec2 mapped = screenToCanvas({580.0f, 320.0f}, {100.0f, 50.0f, 960.0f, 540.0f});
    CHECK(mapped.x == 240.0f && mapped.y == 135.0f);
    startWave(g, catalog.waves.at("base:ambush"));
    CHECK(g.enemies.size() == 12);
    startWave(g, catalog.waves.at("base:last_line"));
    CHECK(g.enemies.size() == 15);
    beginBoss(g, catalog.bosses.at("base:null_crown"));
    CHECK(g.boss.active && g.boss.hp == g.boss.maxHp && g.boss.p.x > SCREEN_W);
    const float bossEntryX = g.boss.p.x;
    updateBoss(g, 0.1f);
    CHECK(g.boss.p.x < bossEntryX && g.boss.p.x > SCREEN_W*0.5f);
    // Lethal hostile impact used to erase the vector from inside hurtPlayer,
    // invalidating updateShots' current reference/index (including its last slot).
    for (bool friendlyFirst : {false, true}) {
        reset(g);
        g.hp = 1;
        g.invulnerable = 0;
        const Shot hostile{g.player, {}, 2, 16, true, Red};
        const Shot friendly{{250, 200}, {}, 2, 13, false, White};
        if (friendlyFirst) g.shots.push_back(friendly);
        g.shots.push_back(hostile);
        if (!friendlyFirst) g.shots.push_back(friendly);
        updateShots(g, 0);
        CHECK(g.lives == 2 && g.hp == 100 && g.state == State::Playing);
        CHECK(g.shots.size() == 1 && !g.shots.front().hostile);
        CHECK(!g.clearHostileShots);
    }
    reset(g);
    g.lives = 1; g.hp = 1; g.invulnerable = 0;
    g.shots.push_back({g.player, {}, 2, 16, true, Red});
    tick(g, 0, {});
    CHECK(g.state == State::GameOver && g.hp == 0 && g.lives == 0);
    controlState(g, false, true);
    CHECK(g.state == State::Playing && g.lives == 3 && g.hp == 100);
    CHECK(g.shots.empty() && g.particles.empty() && g.explosions.empty() && g.enemies.size() == 9);
    CHECK(g.stars.size() == 84 && g.score == 0 && !g.boss.active);

    // Idle mouse input cannot undo keyboard movement; keyboard wins when both move.
    const float startX = g.player.x;
    tick(g, 0.02f, {{1, 0}, {200, 200}, true, false});
    CHECK(g.player.x > startX && g.player.x < 100);
    const Vec2 moved = g.player;
    tick(g, 0.02f, {});
    CHECK(g.player.x == moved.x && g.player.y == moved.y);
    tick(g, 0.02f, {{}, {-100, 999}, true, false});
    CHECK(g.player.x == 16 && g.player.y == SCREEN_H - 19);

    reset(g);
    controlState(g, true, false);
    CHECK(g.state == State::Paused);
    const float pausedTime = g.time, starX = g.stars[0].p.x;
    const auto pausedRng = g.rng;
    tick(g, 0.05f, {{1, 0}, {}, false, true});
    CHECK(g.time == pausedTime && g.stars[0].p.x == starX && g.rng == pausedRng);
    CHECK(g.shots.empty());
    controlState(g, true, false);
    CHECK(g.state == State::Playing);
    controlState(g, false, false, false);
    CHECK(g.state == State::Paused);
    controlState(g, false, false, true);
    CHECK(g.state == State::Paused); // focus return never resumes without consent

    for (const auto size : {Vec2{480,270}, Vec2{960,540}, Vec2{1366,768}, Vec2{1920,1080}}) {
        const Rect dest = destinationRectFor(static_cast<int>(size.x), static_cast<int>(size.y));
        CHECK(dest.width <= size.x && dest.height <= size.y);
        CHECK(std::fmod(dest.width, SCREEN_W) == 0 && dest.height*SCREEN_W == dest.width*SCREEN_H);
        const Vec2 center = screenToCanvas({size.x/2, size.y/2}, dest);
        CHECK(center.x == 240 && center.y == 135);
    }

    reset(g);
    g.spreadTimer = 1;
    playerFire(g);
    CHECK(g.shots.size() == 3);
    CHECK(g.shots[1].v.y < 0 && g.shots[2].v.y > 0);
    g.hp = 60;
    g.power = {g.player, true, 0};
    tick(g, 0, {});
    CHECK(!g.power.active && g.hp == 85 && g.spreadTimer == 9 && g.score == 250);

    // Deterministic scenario through actual projectile collision and wave timers.
    // Scripted hits test progression, not human difficulty or aiming skill.
    reset(g);
    for (int wave = 1; wave <= 3; ++wave) {
        CHECK(g.wave == wave && !g.boss.active);
        for (const Enemy &e : g.enemies) {
            const Vec2 target{e.home.x, e.home.y};
            g.shots.push_back({target, {}, 2, 100, false, White});
        }
        for (Enemy &e : g.enemies) e.p = e.home;
        tick(g, 0, {});
        CHECK(g.enemies.empty() && g.betweenWaves);
        for (int i = 0; i < 120 && g.betweenWaves; ++i) tick(g, 1.0f/60, {});
        CHECK(!g.betweenWaves);
    }
    CHECK(g.boss.active && g.state == State::Playing);
    for (int i = 0; i < 180; ++i) tick(g, 1.0f/60, {});
    CHECK(g.boss.p.x < SCREEN_W && !g.shots.empty());
    g.shots.clear();
    g.shots.push_back({g.boss.p, {}, 2, 1000, false, White});
    tick(g, 0, {});
    CHECK(g.state == State::Victory && !g.boss.active && !g.betweenWaves);
    const auto finalScore = g.score;
    for (int i = 0; i < 180; ++i) tick(g, 1.0f/60, {});
    CHECK(g.state == State::Victory && g.score == finalScore && !g.boss.active);
    CHECK(g.flash == 0 && g.shake == 0 && g.particles.empty() && g.explosions.empty());
    controlState(g, false, true);
    CHECK(g.state == State::Playing && g.score == 0 && g.wave == 1 && g.enemies.size() == 9);
    CHECK(g.time == 0 && g.spreadTimer == 0 && g.hp == 100 && g.lives == 3);
    std::printf("starfall self-test: OK (%d checks)\n", test::checks);
    return 0;
}

}
