#include "competition.h"

#include "competition/league_registry.h"
#include "utils/utils.h"

#include <algorithm>
#include <vector>

using namespace std;

namespace {

CompetitionConfig makeDefaultCompetitionConfig() {
    return {
        "",
        CompetitionTableProfile::Default,
        CompetitionSeasonHandler::Generic,
        {},
        20000,
        45,
        6,
        0,
        0
    };
}

vector<CompetitionConfig> builtInCompetitionConfigs() {
    return {
        {
            "primera division",
            CompetitionTableProfile::PrimeraLike,
            CompetitionSeasonHandler::PrimeraDivision,
            {},
            60000,
            85,
            3,
            0,
            16
        },
        {
            "primera b",
            CompetitionTableProfile::PrimeraLike,
            CompetitionSeasonHandler::PrimeraB,
            {},
            45000,
            70,
            4,
            0,
            16
        },
        {
            "segunda division",
            CompetitionTableProfile::Default,
            CompetitionSeasonHandler::SegundaGroups,
            {true, 7, true, "Grupo Norte", "Grupo Sur"},
            35000,
            60,
            5,
            0,
            14
        },
        {
            "tercera division a",
            CompetitionTableProfile::TerceraA,
            CompetitionSeasonHandler::TerceraA,
            {},
            25000,
            50,
            6,
            30,
            16
        },
        {
            "tercera division b",
            CompetitionTableProfile::TerceraB,
            CompetitionSeasonHandler::TerceraB,
            {true, 14, false, "Zona Norte", "Zona Sur"},
            20000,
            45,
            6,
            30,
            28
        }
    };
}

CompetitionTableProfile parseTableProfile(const string& value) {
    const string normalized = toLower(trim(value));
    if (normalized == "primeralike" || normalized == "primera_like") return CompetitionTableProfile::PrimeraLike;
    if (normalized == "terceraa" || normalized == "tercera_a") return CompetitionTableProfile::TerceraA;
    if (normalized == "tercerab" || normalized == "tercera_b") return CompetitionTableProfile::TerceraB;
    return CompetitionTableProfile::Default;
}

CompetitionSeasonHandler parseSeasonHandler(const string& value) {
    const string normalized = toLower(trim(value));
    if (normalized == "primeradivision" || normalized == "primera_division") return CompetitionSeasonHandler::PrimeraDivision;
    if (normalized == "primerab" || normalized == "primera_b") return CompetitionSeasonHandler::PrimeraB;
    if (normalized == "segundagroups" || normalized == "segunda_groups") return CompetitionSeasonHandler::SegundaGroups;
    if (normalized == "terceraa" || normalized == "tercera_a") return CompetitionSeasonHandler::TerceraA;
    if (normalized == "tercerab" || normalized == "tercera_b") return CompetitionSeasonHandler::TerceraB;
    return CompetitionSeasonHandler::Generic;
}

bool parseBoolValue(const string& value, bool defaultValue = false) {
    const string normalized = toLower(trim(value));
    if (normalized == "1" || normalized == "true" || normalized == "si" || normalized == "yes") return true;
    if (normalized == "0" || normalized == "false" || normalized == "no") return false;
    return defaultValue;
}

CompetitionConfig parseCompetitionConfigRow(const vector<string>& cols, vector<string>& warnings) {
    CompetitionConfig config = makeDefaultCompetitionConfig();
    if (cols.size() < 13) {
        warnings.push_back("competition_rules.csv tiene una fila incompleta y se ignora.");
        return config;
    }

    config.id = toLower(trim(cols[0]));
    config.tableProfile = parseTableProfile(cols[1]);
    config.seasonHandler = parseSeasonHandler(cols[2]);
    config.groups.enabled = parseBoolValue(cols[3]);
    config.groups.groupSize = clampInt(parseAge(cols[4]), 0, 40);
    config.groups.doubleRoundRobin = parseBoolValue(cols[5], true);
    config.groups.northTitle = trim(cols[6]);
    config.groups.southTitle = trim(cols[7]);
    config.baseIncome = max(0LL, parseMarketValue(cols[8]));
    config.wageFactor = clampInt(parseAge(cols[9]), 0, 200);
    config.budgetDivisor = clampInt(parseAge(cols[10]), 1, 20);
    config.maxSquadSize = clampInt(parseAge(cols[11]), 0, 50);
    config.expectedTeamCount = clampInt(parseAge(cols[12]), 0, 80);

    if (!config.groups.enabled) {
        config.groups.groupSize = 0;
        if (config.groups.northTitle.empty()) config.groups.northTitle = "Grupo Norte";
        if (config.groups.southTitle.empty()) config.groups.southTitle = "Grupo Sur";
    }
    if (config.baseIncome <= 0) {
        warnings.push_back("Regla externa invalida para " + config.id + ": base_income debe ser positiva. Se usa fallback.");
        return makeDefaultCompetitionConfig();
    }
    if (config.id.empty()) {
        warnings.push_back("competition_rules.csv contiene una fila sin id de division y se ignora.");
        return makeDefaultCompetitionConfig();
    }
    return config;
}

void validateCompetitionRule(const CompetitionConfig& config, vector<string>& warnings) {
    if (config.groups.enabled && config.groups.groupSize <= 1) {
        warnings.push_back("Regla inconsistente en " + config.id + ": una division con grupos requiere group_size > 1.");
    }
    if (config.expectedTeamCount > 0 && config.groups.enabled &&
        config.groups.groupSize > 0 && config.expectedTeamCount % config.groups.groupSize != 0) {
        warnings.push_back("Regla inconsistente en " + config.id + ": expected_team_count no calza con el tamano de grupos.");
    }
    if (config.maxSquadSize > 0 && config.maxSquadSize < 18) {
        warnings.push_back("Regla inconsistente en " + config.id + ": max_squad_size es demasiado bajo para una temporada normal.");
    }
    if (config.wageFactor <= 0 || config.budgetDivisor <= 0) {
        warnings.push_back("Regla inconsistente en " + config.id + ": wage_factor y budget_divisor deben ser positivos.");
    }
}

vector<CompetitionConfig> mergeCompetitionConfigs(const vector<CompetitionConfig>& loaded, vector<string>& warnings) {
    vector<CompetitionConfig> merged;
    const vector<CompetitionConfig> defaults = builtInCompetitionConfigs();
    for (const CompetitionConfig& fallback : defaults) {
        auto it = find_if(loaded.begin(), loaded.end(), [&](const CompetitionConfig& current) {
            return current.id == fallback.id;
        });
        if (it != loaded.end()) {
            merged.push_back(*it);
        } else {
            warnings.push_back("No existe regla externa para " + fallback.id + ". Se usa configuracion integrada.");
            merged.push_back(fallback);
        }
    }
    return merged;
}

vector<CompetitionConfig> gCompetitionConfigs = builtInCompetitionConfigs();
vector<string> gCompetitionWarnings;
bool gCompetitionConfigsLoaded = false;

const CompetitionConfig kDefaultCompetitionConfig = makeDefaultCompetitionConfig();

void ensureCompetitionConfigsLoaded() {
    if (gCompetitionConfigsLoaded) return;
    reloadCompetitionConfigs();
}

}  // namespace

bool reloadCompetitionConfigs() {
    gCompetitionWarnings.clear();
    gCompetitionConfigs = builtInCompetitionConfigs();
    gCompetitionConfigsLoaded = true;

    vector<string> lines;
    const string rulesPath = registeredCompetitionRulesPath();
    if (!readTextFileLines(rulesPath, lines) || lines.size() <= 1) {
        gCompetitionWarnings.push_back("No se pudo cargar " + rulesPath + ". Se usan reglas integradas.");
        return false;
    }

    vector<CompetitionConfig> loaded;
    vector<string> seenIds;
    for (size_t i = 1; i < lines.size(); ++i) {
        const string line = trim(lines[i]);
        if (line.empty()) continue;
        vector<string> cols = splitCsvLine(line);
        CompetitionConfig parsed = parseCompetitionConfigRow(cols, gCompetitionWarnings);
        if (parsed.id.empty() || parsed.baseIncome <= 0) continue;
        if (find(seenIds.begin(), seenIds.end(), parsed.id) != seenIds.end()) {
            gCompetitionWarnings.push_back("competition_rules.csv repite la division " + parsed.id + ". Se ignora la fila duplicada.");
            continue;
        }
        validateCompetitionRule(parsed, gCompetitionWarnings);
        seenIds.push_back(parsed.id);
        loaded.push_back(parsed);
    }

    if (loaded.empty()) {
        gCompetitionWarnings.push_back("competition_rules.csv no entrego reglas validas. Se usan reglas integradas.");
        return false;
    }

    gCompetitionConfigs = mergeCompetitionConfigs(loaded, gCompetitionWarnings);
    return true;
}

const vector<CompetitionConfig>& listCompetitionConfigs() {
    ensureCompetitionConfigsLoaded();
    return gCompetitionConfigs;
}

const vector<string>& competitionConfigWarnings() {
    ensureCompetitionConfigsLoaded();
    return gCompetitionWarnings;
}

const CompetitionConfig& getCompetitionConfig(const string& id) {
    ensureCompetitionConfigsLoaded();
    for (const auto& config : gCompetitionConfigs) {
        if (config.id == id) return config;
    }
    return kDefaultCompetitionConfig;
}

bool competitionUsesGroupStage(const string& id, int teamCount) {
    const CompetitionConfig& config = getCompetitionConfig(id);
    if (!config.groups.enabled) return false;
    return config.expectedTeamCount <= 0 || teamCount == config.expectedTeamCount;
}

string competitionGroupTitle(const string& id, bool north) {
    const CompetitionConfig& config = getCompetitionConfig(id);
    if (!config.groups.enabled) return north ? "Grupo Norte" : "Grupo Sur";
    return north ? config.groups.northTitle : config.groups.southTitle;
}
