#include "check.h"
#include "regression.h"
#include "../src/content/json.h"
#include "../src/gameplay/game.h"
#include "../src/gameplay/simulate.h"
#include "../src/runtime/options.h"
#include "../src/runtime/paths.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

using namespace sf;
namespace {
template<class F> void rejects(F f, const std::string& fragment) {
    bool caught = false;
    try { f(); } catch(const std::exception& e) { caught = true; CHECK(std::string(e.what()).find(fragment) != std::string::npos); }
    CHECK(caught);
}
void clearCombat(Game& g) {
    g.invulnerable = 1000;
    g.shots.clear();
    for (auto& e : g.enemies) {
        e.p = e.home;
        g.shots.push_back({e.p, {}, 2, 1000001, false, White});
    }
    if (g.boss.active) {
        g.boss.p = {412,135};
        g.shots.push_back({g.boss.p, {}, 2, 1000001, false, White});
    }
    tick(g, 0, {});
}
void untilFirstBoss(Game& g) {
    for (int i = 0; i < 1000 && !g.boss.active; ++i) {
        if (!g.enemies.empty()) clearCombat(g);
        else tick(g, .05f, {});
    }
    CHECK(g.boss.active);
}
}
int main(int argc, char** argv) {
    try {
        const auto arguments = commandArguments(argc, argv);
        if (arguments.size() != 1) throw std::runtime_error("Expected fixture content root");
        const auto catalog = loadCatalog(std::filesystem::u8path(arguments[0]));
        runRegressionTests(catalog);
        const int oldChecks = test::checks;
        // Strict JSON: tokens, escapes, Unicode scalars, no silent duplicate overrides.
        const auto value = json::parse("{\"n\":-12.5e2,\"s\":\"a\\n\\u017c\\ud83d\\ude80\",\"a\":[true,false,null]}", "valid.json");
        CHECK(value.get<json::Value::Object>().at("n").get<double>() == -1250);
        CHECK(value.get<json::Value::Object>().at("a").get<json::Value::Array>().size() == 3);
        CHECK(value.get<json::Value::Object>().at("s").get<std::string>() == "a\n\xc5\xbc\xf0\x9f\x9a\x80");
        CHECK(json::parse("\xef\xbb\xbf{}", "bom").is<json::Value::Object>());
        for (const std::string input : {"", "{", "[] x", "[1,]", "{\"a\":1,}", "01", "+1", "1.", "1e", "NaN", "Infinity", "1e999", "truex", "{\"a\":1,\"a\":2}", "\"\\uD800\"", "\"\\uDC00\"", "\"\\x41\"", "\"x\nx\""})
            rejects([&] { json::parse(input, "broken.json"); }, "broken.json:");
        for (const std::string& input : {std::string("\"\xc0\x80\""), std::string("\"\xed\xa0\x80\""), std::string("\"\xf4\x90\x80\x80\""), std::string("\"\xe2\x82\"")})
            rejects([&] { json::parse(input, "utf8"); }, "utf8:");
        rejects([] { json::parse(std::string(60, '[') + std::string(60, ']'), "deep"); }, "complexity");
        rejects([] { json::parse(std::string(1024*1024+1, ' '), "huge"); }, "1 MiB");
        // Command parsing, explicit root, exclusive selection, no CWD fallback.
        const auto opts = parseOptions({"--campaign", "example:two_sectors"}, "/app");
        CHECK(opts.contentRoot == std::filesystem::path("/app/content"));
        CHECK(opts.campaign == "example:two_sectors");
        CHECK(parseOptions({"--content-root","relative"},"/app").contentRoot == "relative");
        for (const auto& args : {std::vector<std::string>{"--campaign"}, {"--bogus"}, {"--campaign","a","--sector","b"}, {"--self-test","--render-test"}, {"--self-test","--self-test"}, {"--grab","--sector","base:outer_rim"}})
            rejects([&] { parseOptions(args, "/app"); }, "");
        // The new catalog contains exactly the migrated original content plus example pack.
        CHECK(catalog.waves.at("base:patrol").spawns.size() == 9);
        CHECK(catalog.waves.at("base:ambush").spawns.size() == 12);
        CHECK(catalog.waves.at("base:last_line").spawns.size() == 15);
        CHECK(catalog.bosses.at("base:null_crown").hp == 420);
        CHECK(!catalog.enemies.at("example:guard").canDive);
        CampaignRunner director;
        director.begin(catalog,"base:classic");
        CHECK(director.update(.1f,false,true) == RunAction::None); // boss signal cannot finish a wave
        CHECK(director.update(0,true,false) == RunAction::StartStep);
        CHECK(director.waiting() && director.remaining() == 1.8f);
        CHECK(director.update(.5f,true,true) == RunAction::None); // signals cannot skip a wait
        CHECK(director.update(2,false,false) == RunAction::StartStep);
        CHECK(director.step().ref == "base:ambush");
        rejects([&] { director.update(-1,false,false); }, "delta");
        rejects([&] { director.update(std::numeric_limits<float>::quiet_NaN(),false,false); }, "delta");
        rejects([&] { director.begin(catalog,"missing:campaign"); }, "Unknown campaign");
        CHECK(director.step().ref == "base:ambush"); // failed begin is atomic
        // Actual boss collision must not produce Victory while another sector remains.
        Game game; reset(game,catalog,"example:two_sectors");
        untilFirstBoss(game);
        game.hp = 47; game.lives = 2; game.spreadTimer = 8; game.player = {91,123};
        const auto scoreBefore = game.score;
        clearCombat(game);
        CHECK(game.state == State::Playing && game.campaign.transitioning());
        CHECK(!game.boss.active && game.enemies.empty() && game.shots.empty());
        CHECK(game.hp == 47 && game.lives == 2 && game.spreadTimer == 8);
        CHECK(game.score == scoreBefore + 5005);
        const auto scoreAfter = game.score;
        const auto time = game.time, wait = game.campaign.remaining();
        const auto rng = game.rng;
        controlState(game,true,false); tick(game,.05f,{{1,1},{},false,true});
        CHECK(game.state == State::Paused && game.time == time && game.rng == rng);
        CHECK(game.campaign.remaining() == wait && game.campaign.sectorIndex() == 0);
        controlState(game,true,false);
        for (int i=0; i<100 && game.campaign.transitioning(); ++i) tick(game,.05f,{});
        CHECK(game.campaign.sectorIndex() == 1 && game.campaign.sectorCount() == 2);
        CHECK(game.campaign.sector().id == "example:relay" && game.wave == 1);
        CHECK(game.state == State::Playing && game.enemies.size() == 9);
        CHECK(game.hp == 47 && game.lives == 2 && game.score == scoreAfter);
        CHECK(game.spreadTimer > 5 && game.spreadTimer < 8);
        CHECK(game.player.x == 91 && game.player.y == 123);
        // Non-diving archetype is obeyed, not just parsed.
        for(int i=0;i<300;++i) { game.invulnerable=1000; tick(game,.05f,{}); }
        CHECK(std::all_of(game.enemies.begin(),game.enemies.end(),[](const Enemy& e) { return e.definition->canDive || !e.diving; }));
        // Invalid frame time cannot poison simulation state.
        const auto finiteTime=game.time;
        tick(game,std::numeric_limits<float>::infinity(),{});
        CHECK(game.time == finiteTime);
        // Restart from GameOver preserves the selected campaign, not base:classic.
        game.state=State::GameOver; controlState(game,false,true);
        CHECK(game.selectedCampaign == "example:two_sectors" && game.campaign.sectorIndex() == 0);
        CHECK(game.campaign.sectorCount() == 2 && game.score == 0 && game.hp == 100 && game.lives == 3);
        // Preview starts and restarts the chosen sector without playing preceding sectors.
        reset(game,catalog,"base:classic","example:relay");
        CHECK(game.campaign.sectorCount() == 1 && game.campaign.sector().id == "example:relay");
        untilFirstBoss(game); CHECK(game.boss.hp == 540); clearCombat(game);
        CHECK(game.state == State::Victory);
        controlState(game,false,true);
        CHECK(game.campaign.sector().id == "example:relay" && game.enemies.size() == 9);
        // No hard-coded terminal boss, wave count or sector count.
        Catalog synthetic = catalog;
        SectorDef extra; extra.id="test:mixed";extra.name="MIXED";
        extra.steps={{StepType::Boss,"base:null_crown",0},{StepType::Wait,"",0},{StepType::Wave,"base:patrol",0},{StepType::Boss,"example:relay_core",0}};
        synthetic.sectors.emplace(extra.id,extra);
        CampaignDef route;route.id="test:route";route.sectors={"test:mixed","example:relay","base:outer_rim"};route.transitionSeconds=0;
        synthetic.campaigns.emplace(route.id,route);
        std::ostringstream trace;
        simulateCampaign(synthetic,"test:route",{},trace);
        CHECK(trace.str().find("VICTORY sectors=3") != std::string::npos);
        CHECK(trace.str().find("bosses=4") != std::string::npos);
        SectorDef waits; waits.id="test:waits";waits.name="WAITS";waits.steps={{StepType::Wait,"",0},{StepType::Wait,"",.1f}};
        synthetic.sectors.emplace(waits.id,waits);
        std::ostringstream waitTrace; simulateCampaign(synthetic,"",waits.id,waitTrace);
        CHECK(waitTrace.str().find("bosses=0") != std::string::npos);
        // A wave's difficulty does not depend on its display index.
        reset(game,catalog,"base:classic"); game.wave=999; game.invulnerable=1000;
        for (auto& e:game.enemies) { e.p=e.home; e.fireTimer=0; }
        tick(game,0,{});
        CHECK(!game.shots.empty());
        CHECK(std::abs(std::hypot(game.shots.front().v.x,game.shots.front().v.y)-85) < .001f);
        std::cout << "content/campaign checks: OK (" << test::checks-oldChecks << " new, " << test::checks << " total including simulation regression)\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
