#include "competition/league_registry.h"

#include "utils/utils.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
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

string normalizeDivisionToken(const string& raw) {
    string out;
    out.reserve(raw.size());
    bool lastWasSpace = true;
    for (unsigned char ch : raw) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
            lastWasSpace = false;
        } else if (!lastWasSpace) {
            out.push_back(' ');
            lastWasSpace = true;
        }
    }
    return trim(out);
}

bool matchesDivisionAlias(const DivisionInfo& division, const string& normalizedRaw) {
    if (normalizedRaw.empty()) return false;
    if (normalizeDivisionToken(division.id) == normalizedRaw) return true;
    if (normalizeDivisionToken(division.display) == normalizedRaw) return true;
    if (normalizeDivisionToken(pathFilename(division.folder)) == normalizedRaw) return true;
    return false;
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

string canonicalDivisionId(const string& raw) {
    ensureRegistryLoaded();
    const string normalized = normalizeDivisionToken(raw);
    if (normalized.empty()) return string();
    for (const auto& division : registryCache().divisions) {
        if (matchesDivisionAlias(division, normalized)) return division.id;
    }
    return normalized;
}

bool isRegisteredDivisionId(const string& raw) {
    ensureRegistryLoaded();
    const string canonical = canonicalDivisionId(raw);
    return std::any_of(registryCache().divisions.begin(), registryCache().divisions.end(), [&](const DivisionInfo& division) {
        return division.id == canonical;
    });
}

string divisionDisplayName(const string& id) {
    ensureRegistryLoaded();
    const string normalized = canonicalDivisionId(id);
    for (const auto& division : registryCache().divisions) {
        if (division.id == normalized) return division.display;
    }
    return id;
}
