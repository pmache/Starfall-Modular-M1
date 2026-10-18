#pragma once
#include "game.h"
#include <iosfwd>
namespace sf {
// Deterministic hits go through the REAL projectile collision code. This checks
// campaign reachability, not player skill, difficulty, rendering or play time.
void simulateCampaign(const Catalog& catalog, const std::string& campaign,
                      const std::string& soloSector, std::ostream& out);
}
