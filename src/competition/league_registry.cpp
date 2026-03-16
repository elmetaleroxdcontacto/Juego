#include "competition/league_registry.h"

#include "utils/utils.h"

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

using namespace std;

namespace {

const char* kLeagueRegistryPath = "data/configs/league_registry.csv";
const char* kFallbackCompetitionRulesPath = "data/LigaChilena/competition_rules.csv";

vector<DivisionInfo> defaultDivisions() {
    return {
        {"primera division", "data/LigaChilena/primera division", "Primera Division"},
        {"primera b", "data/LigaChilena/primera b", "Primera B"},
        {"segunda division", "data/LigaChilena/segunda division", "Segunda Division"},
        {"tercera division a", "data/LigaChilena/tercera division a", "Tercera Division A"},
        {"tercera division b", "data/LigaChilena/tercera division b", "Tercera Division B"}
    };
}

struct LeagueRegistryCache {
    bool loaded = false;
    vector<DivisionInfo> divisions;
    vector<string> warnings;
    string competitionRulesPath = kFallbackCompetitionRulesPath;
};

LeagueRegistryCache& registryCache() {
    static LeagueRegistryCache cache;
    return cache;
}

void ensureRegistryLoaded() {
    if (registryCache().loaded) return;
    reloadLeagueRegistry();
}

}  // namespace

bool reloadLeagueRegistry() {
    LeagueRegistryCache& cache = registryCache();
    cache.loaded = true;
    cache.divisions = defaultDivisions();
    cache.warnings.clear();
    cache.competitionRulesPath = kFallbackCompetitionRulesPath;

    vector<string> lines;
    if (!readTextFileLines(kLeagueRegistryPath, lines) || lines.size() <= 1) {
        cache.warnings.push_back(string("No se pudo cargar ") + kLeagueRegistryPath + ". Se usa el registro integrado.");
        return false;
    }

    vector<DivisionInfo> loaded;
    set<string> seenIds;
    string configuredRulesPath;
    for (size_t i = 1; i < lines.size(); ++i) {
        const string line = trim(lines[i]);
        if (line.empty()) continue;
        const vector<string> cols = splitCsvLine(line);
        if (cols.size() < 3) {
            cache.warnings.push_back("league_registry.csv tiene una fila incompleta y se ignora.");
            continue;
        }

        DivisionInfo div;
        div.id = toLower(trim(cols[0]));
        div.folder = trim(cols[1]);
        div.display = trim(cols[2]);
        if (div.id.empty() || div.folder.empty()) {
            cache.warnings.push_back("league_registry.csv contiene una division sin id o carpeta. Se ignora.");
            continue;
        }
        if (!seenIds.insert(div.id).second) {
            cache.warnings.push_back("league_registry.csv repite la division " + div.id + ". Se ignora la fila duplicada.");
            continue;
        }
        if (div.display.empty()) div.display = trim(cols[0]);
        loaded.push_back(div);

        if (configuredRulesPath.empty() && cols.size() > 3 && !trim(cols[3]).empty()) {
            configuredRulesPath = trim(cols[3]);
        }
    }

    if (loaded.empty()) {
        cache.warnings.push_back("league_registry.csv no entrego divisiones validas. Se usa el registro integrado.");
        return false;
    }

    cache.divisions = loaded;
    if (!configuredRulesPath.empty()) cache.competitionRulesPath = configuredRulesPath;
    return true;
}

const vector<DivisionInfo>& listRegisteredDivisions() {
    ensureRegistryLoaded();
    return registryCache().divisions;
}

const vector<string>& leagueRegistryWarnings() {
    ensureRegistryLoaded();
    return registryCache().warnings;
}

string registeredCompetitionRulesPath() {
    ensureRegistryLoaded();
    return registryCache().competitionRulesPath;
}

string divisionDisplayName(const string& id) {
    ensureRegistryLoaded();
    const string normalized = toLower(trim(id));
    for (const auto& division : registryCache().divisions) {
        if (division.id == normalized) return division.display;
    }
    return id;
}
