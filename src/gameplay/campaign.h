#pragma once
#include "../content/catalog.h"
#include <cstddef>

namespace sf {
enum class RunAction { None, StartStep, BeginTransition, StartSector, Victory };
// Owns progress only; does not know about sprites, input, projectiles or windows.
class CampaignRunner {
    const Catalog* catalog_ = nullptr;
    std::vector<std::string> route_;
    std::size_t sector_ = 0, step_ = 0;
    float remaining_ = 0, transitionSeconds_ = 0;
    bool transition_ = false, finished_ = false;
    void enterStep();
public:
    void begin(const Catalog& catalog, const std::string& campaign, const std::string& soloSector = {});
    RunAction update(float dt, bool enemiesCleared, bool bossDefeated);
    const SectorDef& sector() const;
    const Step& step() const;
    std::size_t sectorIndex() const { return sector_; }
    std::size_t sectorCount() const { return route_.size(); }
    std::size_t stepIndex() const { return step_; }
    float remaining() const { return remaining_; }
    bool transitioning() const { return transition_; }
    bool finished() const { return finished_; }
    bool waiting() const { return !finished_ && (transition_ || step().type == StepType::Wait); }
};
}
