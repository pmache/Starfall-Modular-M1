#include "simulate.h"
#include <ostream>
#include <stdexcept>
#include <tuple>

namespace sf {
void simulateCampaign(const Catalog& catalog, const std::string& campaign,
                      const std::string& soloSector, std::ostream& out) {
    Game g; reset(g, catalog, campaign, soloSector);
    std::tuple<std::size_t, std::size_t, bool> previous{9999, 9999, false};
    // Bound work from definitions rather than assuming three waves or two sectors.
    const std::size_t maxTicks = 14000000;
    std::size_t steps = 0, bosses = 0, startedSectors = 0;
    for (std::size_t frame = 0; frame < maxTicks && g.state == State::Playing; ++frame) {
        const auto marker = std::make_tuple(g.campaign.sectorIndex(), g.campaign.stepIndex(), g.campaign.transitioning());
        if (marker != previous) {
            if (!g.campaign.transitioning()) {
                if (g.campaign.stepIndex() == 0) { ++startedSectors; out << "ENTER " << g.campaign.sector().id << '\n'; }
                const auto& step = g.campaign.step(); ++steps;
                out << "STEP " << g.campaign.stepIndex() << " " << (step.type == StepType::Wait ? "wait" : step.ref) << '\n';
                if (step.type == StepType::Boss) ++bosses;
            } else out << "TRANSITION (not victory)\n";
            previous = marker;
        }
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
        tick(g, (g.campaign.waiting() ? 0.05f : 0.0f), {});
    }
    if (g.state != State::Victory) throw std::runtime_error("Simulation did not finish (tick limit or game over)");
    out << "VICTORY sectors=" << startedSectors << " steps=" << steps << " bosses=" << bosses << " score=" << g.score << '\n';
    const auto score = g.score;
    for (int i = 0; i < 10; ++i) tick(g, .05f, {});
    if (g.state != State::Victory || g.score != score) throw std::runtime_error("Unstable terminal state");
}
}
