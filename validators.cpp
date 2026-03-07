#include "validators.h"

#include "competition.h"
#include "models.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

struct ValidationResult {
    string name;
    bool ok;
    string detail;
};

static void pushResult(vector<ValidationResult>& results, const string& name, bool ok, const string& detail) {
    results.push_back({name, ok, detail});
}

static string pairKey(const string& a, const string& b) {
    return (a < b) ? (a + "||" + b) : (b + "||" + a);
}

static ValidationResult validateDivisionCounts(Career& career) {
    for (const auto& div : career.divisions) {
        const CompetitionConfig& config = getCompetitionConfig(div.id);
        int count = static_cast<int>(career.getDivisionTeams(div.id).size());
        if (config.expectedTeamCount > 0 && count != config.expectedTeamCount) {
            return {"Conteo divisiones", false, div.id + ": esperado " + to_string(config.expectedTeamCount) + ", actual " + to_string(count)};
        }
    }
    return {"Conteo divisiones", true, "Conteos esperados validados."};
}

static ValidationResult validateUniqueTeams(Career& career) {
    for (const auto& div : career.divisions) {
        set<string> seen;
        for (auto* team : career.getDivisionTeams(div.id)) {
            string key = toLower(trim(team->name));
            if (!seen.insert(key).second) {
                return {"Equipos unicos", false, "Equipo duplicado en " + div.id + ": " + team->name};
            }
        }
    }
    return {"Equipos unicos", true, "No hay nombres duplicados por division."};
}

static ValidationResult validateRosterCaps(Career& career) {
    for (auto& team : career.allTeams) {
        int maxSquad = getCompetitionConfig(team.division).maxSquadSize;
        if (static_cast<int>(team.players.size()) < 11) {
            return {"Planteles", false, team.name + " tiene menos de 11 jugadores."};
        }
        if (maxSquad > 0 && static_cast<int>(team.players.size()) > maxSquad) {
            return {"Planteles", false, team.name + " excede maximo de plantel."};
        }
    }
    return {"Planteles", true, "Planteles dentro de rangos validos."};
}

static ValidationResult validateSchedules(Career& career) {
    for (const auto& div : career.divisions) {
        career.setActiveDivision(div.id);
        if (career.activeTeams.size() < 2) continue;
        if (career.schedule.empty()) {
            return {"Calendarios", false, "Calendario vacio en " + div.id};
        }

        map<string, int> pairCounts;
        map<string, int> homeAwayBalance;
        for (const auto& round : career.schedule) {
            set<int> used;
            for (const auto& match : round) {
                if (match.first < 0 || match.second < 0 ||
                    match.first >= static_cast<int>(career.activeTeams.size()) ||
                    match.second >= static_cast<int>(career.activeTeams.size())) {
                    return {"Calendarios", false, "Indice fuera de rango en " + div.id};
                }
                if (!used.insert(match.first).second || !used.insert(match.second).second) {
                    return {"Calendarios", false, "Equipo repetido en una misma fecha de " + div.id};
                }
                Team* home = career.activeTeams[match.first];
                Team* away = career.activeTeams[match.second];
                if (career.usesGroupFormat()) {
                    bool homeNorth = find(career.groupNorthIdx.begin(), career.groupNorthIdx.end(), match.first) != career.groupNorthIdx.end();
                    bool awayNorth = find(career.groupNorthIdx.begin(), career.groupNorthIdx.end(), match.second) != career.groupNorthIdx.end();
                    if (homeNorth != awayNorth) {
                        return {"Calendarios", false, "Cruce entre grupos detectado en " + div.id};
                    }
                }
                string key = pairKey(home->name, away->name);
                pairCounts[key]++;
                homeAwayBalance[home->name + "->" + away->name]++;
            }
        }

        if (!career.usesGroupFormat()) {
            for (const auto& entry : pairCounts) {
                if (entry.second != 2) {
                    return {"Calendarios", false, "Pareja sin ida/vuelta en " + div.id + ": " + entry.first};
                }
            }
        }
    }
    return {"Calendarios", true, "Fixtures validados."};
}

static ValidationResult validateSaveLoad() {
    Career career;
    career.initializeLeague(true);
    if (career.divisions.empty()) {
        return {"Guardado/carga", false, "No hay divisiones para validar."};
    }
    career.setActiveDivision(career.divisions.front().id);
    if (career.activeTeams.empty()) {
        return {"Guardado/carga", false, "No hay equipos en la division inicial."};
    }
    career.myTeam = career.activeTeams.front();
    career.saveFile = "saves/validation_career_save.txt";
    career.currentSeason = 2;
    career.currentWeek = 3;
    career.resetSeason();
    career.myTeam->assistantCoach = 73;
    career.myTeam->fitnessCoach = 74;
    career.myTeam->scoutingChief = 75;
    career.myTeam->youthCoach = 76;
    career.myTeam->medicalTeam = 77;
    career.myTeam->youthRegion = "Sur";
    career.myTeam->matchInstruction = "Balon parado";
    career.myTeam->clubStyle = "Presion vertical";
    career.myTeam->youthIdentity = "Cantera estructurada";
    career.myTeam->primaryRival = "Rival de prueba";
    career.myTeam->clubPrestige = 81;
    career.boardMonthlyObjective = "Objetivo de prueba";
    career.boardMonthlyTarget = 5;
    career.boardMonthlyProgress = 2;
    career.boardMonthlyDeadlineWeek = 6;
    career.lastMatchAnalysis = "Analisis de prueba";
    if (!career.myTeam->players.empty()) {
        career.myTeam->preferredXI = {career.myTeam->players.front().name};
        career.myTeam->players.front().developmentPlan = "Liderazgo";
        career.myTeam->players.front().promisedRole = "Titular";
        career.myTeam->players.front().traits = {"Lider", "Competidor"};
        career.myTeam->players.front().preferredFoot = "Ambos";
        career.myTeam->players.front().secondaryPositions = {"MED", "DEL"};
        career.myTeam->players.front().consistency = 82;
        career.myTeam->players.front().bigMatches = 79;
        career.myTeam->players.front().currentForm = 74;
        career.myTeam->players.front().tacticalDiscipline = 77;
        career.myTeam->players.front().versatility = 71;
        if (career.myTeam->players.size() > 1) {
            career.myTeam->preferredBench = {career.myTeam->players[1].name};
        }
    }
    career.addNews("Prueba de guardado.");
    career.scoutingShortlist.push_back("Club Prueba|Jugador Prueba");
    career.saveCareer();

    ifstream rawSave(career.saveFile);
    string firstLine;
    if (!rawSave.is_open() || !getline(rawSave, firstLine) || firstLine.rfind("VERSION ", 0) != 0) {
        return {"Guardado/carga", false, "El archivo guardado no incluye version de save."};
    }

    Career loaded;
    loaded.saveFile = "saves/validation_career_save.txt";
    loaded.initializeLeague(true);
    if (!loaded.loadCareer()) {
        return {"Guardado/carga", false, "No pudo recargar el archivo de validacion."};
    }
    if (!loaded.myTeam || !career.myTeam || loaded.myTeam->name != career.myTeam->name) {
        return {"Guardado/carga", false, "El equipo cargado no coincide."};
    }
    if (loaded.currentSeason != career.currentSeason || loaded.currentWeek != career.currentWeek) {
        return {"Guardado/carga", false, "Temporada o semana no coinciden tras carga."};
    }
    if (loaded.myTeam->assistantCoach != career.myTeam->assistantCoach ||
        loaded.myTeam->medicalTeam != career.myTeam->medicalTeam ||
        loaded.myTeam->youthRegion != career.myTeam->youthRegion ||
        loaded.myTeam->matchInstruction != career.myTeam->matchInstruction ||
        loaded.myTeam->clubStyle != career.myTeam->clubStyle ||
        loaded.myTeam->youthIdentity != career.myTeam->youthIdentity ||
        loaded.myTeam->primaryRival != career.myTeam->primaryRival) {
        return {"Guardado/carga", false, "No se preservaron correctamente los datos de staff."};
    }
    if (!loaded.myTeam->players.empty() && !career.myTeam->players.empty()) {
        const Player& loadedPlayer = loaded.myTeam->players.front();
        const Player& savedPlayer = career.myTeam->players.front();
        if (loadedPlayer.developmentPlan != savedPlayer.developmentPlan ||
            loadedPlayer.promisedRole != savedPlayer.promisedRole ||
            loadedPlayer.traits != savedPlayer.traits ||
            loadedPlayer.preferredFoot != savedPlayer.preferredFoot ||
            loadedPlayer.secondaryPositions != savedPlayer.secondaryPositions ||
            loadedPlayer.consistency != savedPlayer.consistency ||
            loadedPlayer.bigMatches != savedPlayer.bigMatches ||
            loadedPlayer.currentForm != savedPlayer.currentForm ||
            loadedPlayer.tacticalDiscipline != savedPlayer.tacticalDiscipline ||
            loadedPlayer.versatility != savedPlayer.versatility) {
            return {"Guardado/carga", false, "No se preservaron correctamente los datos avanzados del jugador."};
        }
    }
    if (loaded.boardMonthlyObjective != career.boardMonthlyObjective ||
        loaded.lastMatchAnalysis != career.lastMatchAnalysis) {
        return {"Guardado/carga", false, "No se preservaron correctamente datos avanzados de carrera."};
    }
    if (loaded.scoutingShortlist != career.scoutingShortlist) {
        return {"Guardado/carga", false, "No se preservo correctamente la shortlist de scouting."};
    }
    return {"Guardado/carga", true, "Guardado y carga basicos verificados."};
}

static ValidationResult validateTableSorting() {
    Team a("A");
    Team b("B");
    Team c("C");
    a.points = 10; a.goalsFor = 8; a.goalsAgainst = 4; a.wins = 3;
    b.points = 10; b.goalsFor = 7; b.goalsAgainst = 4; b.wins = 2;
    c.points = 9; c.goalsFor = 9; c.goalsAgainst = 5; c.wins = 2;
    a.division = "primera b";
    b.division = "primera b";
    c.division = "primera b";
    LeagueTable table;
    table.ruleId = "primera b";
    table.addTeam(&b);
    table.addTeam(&c);
    table.addTeam(&a);
    table.sortTable();
    if (table.teams.size() != 3 || table.teams[0] != &a || table.teams[1] != &b || table.teams[2] != &c) {
        return {"Desempates", false, "El orden de tabla esperado no coincide."};
    }
    return {"Desempates", true, "Orden de tabla base validado."};
}

ValidationSuiteSummary buildValidationSuiteSummary() {
    vector<ValidationResult> results;
    Career career;
    career.initializeLeague(true);

    results.push_back(validateDivisionCounts(career));
    results.push_back(validateUniqueTeams(career));
    results.push_back(validateRosterCaps(career));
    results.push_back(validateSchedules(career));
    results.push_back(validateSaveLoad());
    results.push_back(validateTableSorting());

    int failures = 0;
    ValidationSuiteSummary summary;
    summary.lines.push_back("");
    summary.lines.push_back("=== Suite de Validacion ===");
    for (const auto& result : results) {
        if (!result.ok) failures++;
        summary.lines.push_back(string(result.ok ? "[OK] " : "[FAIL] ") + result.name + ": " + result.detail);
    }
    summary.ok = (failures == 0);
    summary.lines.push_back("");
    summary.lines.push_back("Resultado: " + (summary.ok ? string("sin fallas") : to_string(failures) + " falla(s)"));
    return summary;
}

int runValidationSuite(bool verbose) {
    ValidationSuiteSummary summary = buildValidationSuiteSummary();
    if (verbose) {
        for (const auto& line : summary.lines) {
            cout << line << endl;
        }
    }
    return summary.ok ? 0 : 1;
}
