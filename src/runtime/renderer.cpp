#include "renderer.h"
#include "../../pixel_art.h"
#include "../../pixel_font.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../../tests/check.h"
namespace sf::view {
namespace {
Vector2 toRay(sf::Vec2 p) { return {p.x, p.y}; }
Color toRay(sf::Rgba c) { return {c.r, c.g, c.b, c.a}; }
Rectangle destinationRect();
}
Input readInput() {
    Input input;
    input.move.x = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
    input.move.y = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) - (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
    const Vector2 mouse = GetMousePosition();
    const Vector2 delta = GetMouseDelta();
    const Rectangle destination = destinationRect();
    const bool inside = CheckCollisionPointRec(mouse, destination);
    // An idle pointer must not undo the last keyboard movement.
    input.mouseActive = inside && (delta.x != 0.0f || delta.y != 0.0f
                                   || IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    if (input.mouseActive) input.mouseTarget = sf::screenToCanvas({mouse.x, mouse.y}, {destination.x, destination.y, destination.width, destination.height});
    input.firing = (inside && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                 || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_Z);
    return input;
}

void drawShip(sf::Vec2 p, bool powered, float time) {
    art::ship(toRay(p), powered, time);
}

void drawEnemy(const Enemy &e, float time) {
    art::enemy(toRay(e.p), e.kind, e.carrier, time + e.phase);
}

void drawBoss(const Boss &b, float time) {
    art::boss(toRay(b.p), time);
}

void drawBackground(const Game &g) {
    art::backdrop(g.time);
    for (const Star &s : g.stars) {
        const Color colors[] = {{69,96,119,150}, {113,158,167,205}, {184,214,192,235}};
        const int x = static_cast<int>(s.p.x), y = static_cast<int>(s.p.y);
        DrawPixel(x, y, colors[s.layer]);
        if (s.layer == 2) DrawPixel(x-1, y, Color{57,96,118,150});
    }
}

void drawWorld(const Game &g) {
    drawBackground(g);
    // Drawing must not advance the gameplay RNG (or affect paused simulation).
    const float sx = g.shake > 0.0f ? std::sin(g.shake*127.0f)*2.5f : 0.0f;
    const float sy = g.shake > 0.0f ? std::cos(g.shake*113.0f)*2.0f : 0.0f;
    BeginMode2D(Camera2D{{sx, sy}, {0, 0}, 0.0f, 1.0f});

    for (const Shot &s : g.shots) {
        if (s.hostile) {
            const int x=static_cast<int>(s.p.x), y=static_cast<int>(s.p.y);
            DrawRectangle(x-2,y-1,5,3,Color{240,72,119,255});
            DrawLine(x-1,y-2,x+1,y-2,Color{170,41,91,255});
            DrawLine(x-1,y+2,x+1,y+2,Color{170,41,91,255});
            DrawLine(x-1,y,x+1,y,Color{255,231,156,255});
        } else {
            const int x=static_cast<int>(s.p.x), y=static_cast<int>(s.p.y);
            DrawLine(x-8,y,x-3,y,Color{31,115,151,170});
            DrawLine(x-3,y,x+2,y,Color{107,241,236,255});
            DrawPixel(x+3,y,Color{224,255,214,255});
        }
    }
    for (const Enemy &e : g.enemies) drawEnemy(e, g.time);
    if (g.boss.active) drawBoss(g.boss, g.time);

    if (g.power.active) {
        const int x = static_cast<int>(g.power.p.x);
        const int y = static_cast<int>(g.power.p.y);
        const int pulse = 8 + static_cast<int>(std::sin(g.power.age*7.0f)*2.0f);
        DrawCircleLines(x, y, static_cast<float>(pulse), Color{100, 255, 177, 220});
        DrawRectangle(x-4, y-4, 8, 8, Color{32, 114, 108, 255});
        DrawRectangle(x-1, y-4, 2, 8, Color{159, 255, 187, 255});
        DrawRectangle(x-4, y-1, 8, 2, Color{159, 255, 187, 255});
    }

    if (g.invulnerable <= 0.0f || (static_cast<int>(g.invulnerable*12.0f) & 1) == 0)
        drawShip(g.player, g.spreadTimer > 0.0f, g.time);

    for (const Particle &p : g.particles) {
        const float a = std::clamp(p.life/p.maxLife, 0.0f, 1.0f);
        DrawRectangle(static_cast<int>(p.p.x), static_cast<int>(p.p.y), 2, 2, Fade(toRay(p.color), a));
    }
    for (const Explosion &e : g.explosions) art::explosion(toRay(e.p), e.age, e.size);
    EndMode2D();
}

void textShadow(const char *text, int x, int y, int size, Color color) {
    ui::text(text, x+1, y+1, size, Color{0, 0, 0, 190});
    ui::text(text, x, y, size, color);
}

void centeredText(const char *text, int y, int size, Color color) {
    textShadow(text, (SCREEN_W - ui::measure(text, size))/2, y, size, color);
}

void drawHud(const Game &g) {
    DrawRectangle(0, 0, SCREEN_W, 21, Color{5, 8, 19, 225});
    DrawLine(0, 21, SCREEN_W, 21, Color{54, 191, 196, 150});
    textShadow("STARFALL", 8, 5, 10, Color{112, 236, 235, 255});

    char label[64];
    std::snprintf(label, sizeof(label), "SCORE %07lld", static_cast<long long>(g.score));
    textShadow(label, 112, 5, 10, RAYWHITE);
    std::snprintf(label, sizeof(label), "SEC %zu/%zu", g.campaign.sectorIndex()+1, g.campaign.sectorCount());
    textShadow(label, 251, 5, 10, Color{255, 194, 77, 255});
    std::snprintf(label, sizeof(label), "LIVES %d", g.lives);
    textShadow(label, 398, 5, 10, Color{244, 111, 139, 255});

    DrawRectangle(8, SCREEN_H-13, 104, 7, Color{17, 21, 42, 230});
    DrawRectangle(10, SCREEN_H-11, g.hp, 3, g.hp > 30 ? Color{75, 226, 165, 255} : Color{255, 82, 99, 255});
    ui::text("HULL", 8, SCREEN_H-24, 8, Color{164, 187, 203, 255});
    if (g.spreadTimer > 0.0f) {
        std::snprintf(label, sizeof(label), "TRIDENT %.1f", std::max(0.0f, g.spreadTimer));
        textShadow(label, SCREEN_W - 95, SCREEN_H-14, 9, Color{112, 255, 178, 255});
    }

    if (g.boss.active) {
        DrawRectangle(145, 27, 190, 8, Color{15, 15, 35, 230});
        const int w = static_cast<int>(186.0f*g.boss.hp/g.boss.maxHp);
        DrawRectangle(147, 29, w, 4, Color{241, 73, 121, 255});
        centeredText(g.boss.definition->name.c_str(), 37, 8, Color{255, 154, 178, 255});
    }

    if (g.waveBanner > 0.0f) {
        const bool boss = g.boss.active;
        centeredText(boss ? g.boss.definition->name.c_str() : TextFormat("WAVE %d", g.wave), 104, 15,
                     boss ? Color{255, 92, 125, 255} : Color{255, 207, 87, 255});
        centeredText(boss ? "BREAK THE CORE" : g.campaign.sector().name.c_str(), 128, 9, Color{157, 207, 219, 255});
    } else if (g.betweenWaves) {
        centeredText(g.campaign.transitioning() ? "NEXT SECTOR" : "FORMATION CLEAR", 114, 15, Color{103, 255, 183, 255});
    }
}

void drawOverlay(const Game &g) {
    if (g.state == State::Playing) return;
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{3, 5, 16, 190});
    DrawRectangle(116, 83, 248, 104, Color{11, 15, 35, 245});
    DrawRectangleLines(116, 83, 248, 104, Color{76, 209, 209, 230});
    if (g.state == State::Paused) {
        centeredText("PAUSED", 100, 24, Color{255, 207, 84, 255});
        centeredText("P / ESC  RESUME", 143, 10, Color{180, 210, 220, 255});
    } else if (g.state == State::GameOver) {
        centeredText("SIGNAL LOST", 99, 22, Color{255, 82, 118, 255});
        centeredText("R / ENTER  RESTART", 143, 10, Color{190, 210, 220, 255});
    } else {
        centeredText("CAMPAIGN COMPLETE", 99, 20, Color{105, 255, 181, 255});
        centeredText(TextFormat("FINAL SCORE  %07lld", static_cast<long long>(g.score)), 130, 10, Color{255, 207, 84, 255});
        centeredText("R / ENTER  FLY AGAIN", 151, 10, Color{190, 210, 220, 255});
    }
}

void drawGame(const Game &g) {
    drawWorld(g);
    drawHud(g);
    drawOverlay(g);
    for (int y = 1; y < SCREEN_H; y += 3) DrawLine(0, y, SCREEN_W, y, Color{0, 0, 0, 22});
    DrawRectangleLines(1, 1, SCREEN_W-2, SCREEN_H-2, Color{67, 213, 214, 80});
    if (g.flash > 0.0f) DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(WHITE, g.flash*1.6f));
}

namespace {
Rectangle destinationRect() {
    const auto d = sf::destinationRectFor(GetScreenWidth(), GetScreenHeight());
    return {d.x, d.y, d.width, d.height};
}
}

bool exportCanvas(RenderTexture2D canvas, const char *path) {
    Image image = LoadImageFromTexture(canvas.texture);
    ImageFlipVertical(&image);
    const bool ok = ExportImage(image, path);
    UnloadImage(image);
    return ok;
}

void renderFrame(Game &g, RenderTexture2D canvas) {
    BeginTextureMode(canvas);
    drawGame(g);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(Color{2, 3, 8, 255});
    DrawTexturePro(canvas.texture, {0, 0, static_cast<float>(SCREEN_W), -static_cast<float>(SCREEN_H)},
                   destinationRect(), {0, 0}, 0.0f, WHITE);
    EndDrawing();
}


int graphicsSelfTest() {
    CHECK(ui::measure("",10) == 0 && ui::measure("A",10) == 5);
    CHECK(ui::measure("STARFALL",10) == 47 && ui::measure("PAUSED",24) == 105);
    CHECK(ui::glyph('a').character == 'A' && ui::glyph('@').character == '?');
    CHECK(std::all_of(std::begin(ui::glyphs),std::end(ui::glyphs),[](const ui::Glyph &g) {
        return std::all_of(std::begin(g.rows),std::end(g.rows),[](std::uint8_t row) { return row < 32; });
    }));
    CHECK(art::validStencil(art::fighter));
    CHECK(art::validStencil(art::hornet));
    CHECK(art::validStencil(art::manta));
    CHECK(art::validStencil(art::bomber));
    CHECK(art::validStencil(art::crown));
    return 0;
}
}
