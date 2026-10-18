#include "catalog.h"
#include "../runtime/options.h"
#include "../runtime/paths.h"
#include "../gameplay/simulate.h"
#include "../../tests/regression.h"
#include <iostream>
int main(int argc, char** argv) {
    try {
        const auto options = sf::parseOptions(sf::commandArguments(argc, argv), sf::executableDirectory());
        if (options.help) { std::cout << sf::usage(); return 0; }
        if (options.grab) throw std::runtime_error("Use starfall.exe for rendering; this tool has no graphics backend");
        const auto catalog = sf::loadCatalog(options.contentRoot);
        const auto campaign = options.campaign.empty() ? catalog.defaultCampaign : options.campaign;
        sf::CampaignRunner check; check.begin(catalog, campaign, options.sector);
        if (options.selfTest) return sf::runRegressionTests(catalog);
        if (options.simulate) sf::simulateCampaign(catalog, campaign, options.sector, std::cout);
        else std::cout << sf::describe(catalog);
        return 0;
    } catch (const std::exception& e) { sf::reportError(e.what(), false); return 2; }
}
