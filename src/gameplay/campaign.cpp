#include "campaign.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sf {
void CampaignRunner::begin(const Catalog& catalog, const std::string& campaign, const std::string& soloSector) {
    // Build a replacement first: invalid selections never damage a live run.
    CampaignRunner next;
    next.catalog_ = &catalog;
    if (!soloSector.empty()) {
        if (!catalog.sectors.count(soloSector)) throw std::runtime_error("Unknown sector: " + soloSector);
        next.route_ = {soloSector};
    } else {
        const auto found = catalog.campaigns.find(campaign);
        if (found == catalog.campaigns.end()) throw std::runtime_error("Unknown campaign: " + campaign);
        next.route_ = found->second.sectors;
        next.transitionSeconds_ = found->second.transitionSeconds;
    }
    next.enterStep();
    *this = std::move(next);
}
const SectorDef& CampaignRunner::sector() const {
    if (!catalog_ || route_.empty()) throw std::logic_error("CampaignRunner is not initialized");
    return catalog_->sectors.at(route_.at(sector_));
}
const Step& CampaignRunner::step() const { return sector().steps.at(step_); }
void CampaignRunner::enterStep() { remaining_ = step().type == StepType::Wait ? step().seconds : 0; }
RunAction CampaignRunner::update(float dt, bool enemiesCleared, bool bossDefeated) {
    if (!catalog_) throw std::logic_error("CampaignRunner is not initialized");
    if (finished_) return RunAction::None;
    if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("Invalid campaign delta time");
    if (transition_) {
        remaining_ = std::max(0.0f, remaining_ - dt);
        if (remaining_ > 0) return RunAction::None;
        transition_ = false; ++sector_; step_ = 0; enterStep();
        return RunAction::StartSector;
    }
    bool done = false;
    switch (step().type) {
    case StepType::Wave: done = enemiesCleared; break;
    case StepType::Boss: done = bossDefeated; break;
    case StepType::Wait: remaining_ = std::max(0.0f, remaining_ - dt); done = remaining_ <= 0; break;
    }
    if (!done) return RunAction::None;
    if (step_ + 1 < sector().steps.size()) { ++step_; enterStep(); return RunAction::StartStep; }
    if (sector_ + 1 == route_.size()) { finished_ = true; return RunAction::Victory; }
    transition_ = true; remaining_ = transitionSeconds_;
    return RunAction::BeginTransition;
}
}
