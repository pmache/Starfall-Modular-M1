#include "options.h"
#include <set>
#include <stdexcept>

namespace sf {
Options parseOptions(const std::vector<std::string>& args, const std::filesystem::path& executableDir) {
    Options o; o.contentRoot = executableDir / "content";
    std::set<std::string> seen;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (!seen.insert(arg).second) throw std::runtime_error("Duplicate option: " + arg);
        if (arg == "--help" || arg == "-h") o.help = true;
        else if (arg == "--validate-content" || arg == "--list-content") o.validate = true;
        else if (arg == "--self-test") o.selfTest = true;
        else if (arg == "--grab") o.grab = true;
        else if (arg == "--render-test") o.renderTest = o.grab = true;
        else if (arg == "--simulate-campaign") o.simulate = true;
        else if (arg == "--content-root" || arg == "--campaign" || arg == "--sector") {
            if (++i == args.size() || args[i].rfind("--", 0) == 0) throw std::runtime_error("Missing value for " + arg);
            if (arg == "--content-root") o.contentRoot = std::filesystem::u8path(args[i]);
            else if (arg == "--campaign") o.campaign = args[i];
            else o.sector = args[i];
        } else throw std::runtime_error("Unknown argument: " + arg);
    }
    if (!o.campaign.empty() && !o.sector.empty()) throw std::runtime_error("Choose --campaign OR --sector, not both");
    const int modes = static_cast<int>(o.validate) + static_cast<int>(o.selfTest) + static_cast<int>(o.grab) + static_cast<int>(o.simulate);
    if (modes > 1) throw std::runtime_error("Choose only one validation/test/render mode");
    if ((o.selfTest || o.grab) && (!o.campaign.empty() || !o.sector.empty()))
        throw std::runtime_error("Regression/render fixtures use base:classic; do not combine them with a campaign/sector selection");
    return o;
}
const char* usage() {
    return "Starfall Modular M1\n"
        "  --campaign PACK:ID       Play a campaign\n"
        "  --sector PACK:ID         Play only this sector (R restarts it)\n"
        "  --content-root PATH      Override content folder (otherwise next to EXE)\n"
        "  --validate-content       Check every enabled definition; no window\n"
        "  --list-content           Validate and list campaigns and sectors\n"
        "  --simulate-campaign      Scripted collision/progression test; not a playthrough\n"
        "  --self-test              Original base:classic regression tests\n"
        "  --render-test            Original OpenGL screenshots and checks\n"
        "  --grab                   Save a preview\n";
}
}
