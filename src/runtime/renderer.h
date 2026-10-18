#pragma once
#include "raylib.h"
#include "../gameplay/game.h"
namespace sf::view {
Input readInput();
void drawGame(const Game& game);
void renderFrame(Game& game, RenderTexture2D canvas);
bool exportCanvas(RenderTexture2D canvas, const char* path);
int graphicsSelfTest();
}
