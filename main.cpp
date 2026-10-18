#include "src/runtime/renderer.h"
#include "src/runtime/options.h"
#include "src/runtime/paths.h"
#include "src/gameplay/simulate.h"
#include "tests/regression.h"
#include "tests/check.h"
#include "pixel_art.h"
#include "pixel_font.h"
#include <algorithm>
#include <iostream>

using namespace sf;
using namespace sf::view;

int application(const Options& options) {
    if (options.help) { std::cout << usage(); return 0; }
    const Catalog catalog = loadCatalog(options.contentRoot);
    const std::string campaign = options.campaign.empty() ? catalog.defaultCampaign : options.campaign;
    CampaignRunner selection; selection.begin(catalog, campaign, options.sector);
    if (options.validate) { std::cout << describe(catalog); return 0; }
    if (options.simulate) { simulateCampaign(catalog, campaign, options.sector, std::cout); return 0; }
    if (options.selfTest) {
        runRegressionTests(catalog);
        graphicsSelfTest();
        std::printf("starfall full self-test: OK (%d checks)\n", sf::test::checks);
        return 0;
    }
    const bool grab = options.grab, renderTest = options.renderTest;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (grab ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(SCREEN_W*2, SCREEN_H*2, "Starfall Squadron");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(SCREEN_W, SCREEN_H);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_W, SCREEN_H);
    if (canvas.id == 0) { CloseWindow(); return 1; }
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    Game game;
    reset(game, catalog, (grab ? "base:classic" : campaign), options.sector);

    if (grab) {
        game.spreadTimer = 6;
        for (int i = 0; i < 145; ++i) tick(game, 1.0f/60.0f, demoInput(game));
        BeginTextureMode(canvas);
        drawGame(game);
        EndTextureMode();
        const bool ok = exportCanvas(canvas, "starfall-preview.png");
        if (renderTest) {
            CHECK(ok);
            for (State state : {State::Paused, State::GameOver, State::Victory}) {
                game.state = state;
                const auto rng = game.rng;
                renderFrame(game, canvas);
                CHECK(game.rng == rng);
                CHECK(exportCanvas(canvas, state == State::Paused ? "paused.png" : state == State::GameOver ? "game-over.png" : "victory.png"));
            }
            reset(game); game.enemies.clear(); game.wave = 3; beginBoss(game, catalog.bosses.at("base:null_crown"));
            for (int i = 0; i < 180; ++i) updateBoss(game, 1.0f/60);
            renderFrame(game, canvas);
            CHECK(exportCanvas(canvas, "boss.png"));
            BeginTextureMode(canvas);
            ClearBackground(Color{9,15,28,255});
            ui::text("STARFALL / SPRITE WORKSHOP",14,12,10,Color{163,200,205,255});
            art::sprite(art::fighter,16,48,art::playerPalette(false),2);
            art::sprite(art::hornet,122,48,art::enemyPalette(0),2);
            art::sprite(art::manta,224,48,art::enemyPalette(1),2);
            art::sprite(art::bomber,330,48,art::enemyPalette(2),2);
            ui::text("FIGHTER",16,96,10,RAYWHITE);
            ui::text("HORNET",122,96,10,RAYWHITE);
            ui::text("MANTA",224,96,10,RAYWHITE);
            ui::text("BOMBER",330,96,10,RAYWHITE);
            art::boss({82,183},0);
            ui::text("NULL CROWN",24,242,10,RAYWHITE);
            for(int frame=0;frame<7;++frame) art::explosion({180.0f+(frame%4)*72,150.0f+(frame/4)*65},frame/14.0f,1);
            ui::text("EXPLOSION / 7 FRAMES",166,242,10,RAYWHITE);
            EndTextureMode();
            CHECK(exportCanvas(canvas,"sprite-sheet.png"));
            for (Vector2 size : {Vector2{480,270}, Vector2{960,540}, Vector2{1366,768}}) {
                SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
                for (int i = 0; i < 3; ++i) renderFrame(game, canvas);
                CHECK(GetScreenWidth() == static_cast<int>(size.x) && GetScreenHeight() == static_cast<int>(size.y));
            }
            std::printf("starfall render-test: OK (%d checks)\n", sf::test::checks);
        }
        UnloadRenderTexture(canvas);
        CloseWindow();
        return ok ? 0 : 1;
    }

    while (!WindowShouldClose()) {
        const bool altEnter = (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER);
        if (IsKeyPressed(KEY_F11) || altEnter) ToggleBorderlessWindowed();
        if (IsKeyPressed(KEY_F12)) exportCanvas(canvas, "starfall-shot.png");

        controlState(game, IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE),
            IsKeyPressed(KEY_R) || (IsKeyPressed(KEY_ENTER) && !altEnter), IsWindowFocused());
        tick(game, GetFrameTime(), readInput());
        renderFrame(game, canvas);
    }

    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}

int main(int argc, char** argv) {
    bool graphical = false;
    try {
        const auto options = parseOptions(commandArguments(argc, argv), executableDirectory());
        graphical = !options.validate && !options.selfTest && !options.simulate && !options.grab && !options.help;
        return application(options);
    } catch (const std::exception& error) {
        if (IsWindowReady()) CloseWindow();
        reportError(error.what(), graphical);
        return 2;
    }
}
