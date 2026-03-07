#include "career/career_support.h"

#include "utils.h"

#include <algorithm>

using namespace std;

string boardStatusLabel(int confidence) {
    if (confidence >= 75) return "Muy alta";
    if (confidence >= 55) return "Estable";
    if (confidence >= 35) return "En observacion";
    if (confidence >= 20) return "Bajo presion";
    return "Critica";
}

string managerStyleLabel(const Team& team) {
    if (team.tactics == "Pressing" || team.pressingIntensity >= 4) return "intenso";
    if (team.matchInstruction == "Juego directo" || team.tempo >= 4) return "vertical";
    if (team.matchInstruction == "Por bandas" || team.width >= 4) return "amplio";
    if (team.tactics == "Defensive" || team.defensiveLine <= 2) return "pragmatico";
    return "equilibrado";
}

vector<Team*> buildJobMarket(const Career& career, bool emergency) {
    vector<Team*> jobs;
    if (!career.myTeam) return jobs;
    for (const auto& team : career.allTeams) {
        if (&team == career.myTeam) continue;
        int strength = team.getAverageSkill();
        if (emergency) {
            if (strength <= career.managerReputation + 15) jobs.push_back(const_cast<Team*>(&team));
        } else if (strength <= career.managerReputation + 25 || team.division == career.myTeam->division) {
            jobs.push_back(const_cast<Team*>(&team));
        }
    }
    sort(jobs.begin(), jobs.end(), [](Team* a, Team* b) {
        if (teamPrestigeScore(*a) != teamPrestigeScore(*b)) return teamPrestigeScore(*a) > teamPrestigeScore(*b);
        if (a->getSquadValue() != b->getSquadValue()) return a->getSquadValue() > b->getSquadValue();
        return a->name < b->name;
    });
    if (jobs.size() > (emergency ? 12 : 8)) jobs.resize(emergency ? 12 : 8);
    return jobs;
}

void takeManagerJob(Career& career, Team* newClub, const string& reason) {
    if (!newClub) return;
    string oldClub = career.myTeam ? career.myTeam->name : "Sin club";
    career.myTeam = newClub;
    career.setActiveDivision(newClub->division);
    career.initializeBoardObjectives();
    career.boardConfidence = clampInt(career.boardConfidence + 10, 35, 80);
    career.addNews(career.managerName + " deja " + oldClub + " y asume en " + newClub->name + ". " + reason);
}

const Team* nextOpponent(const Career& career) {
    if (!career.myTeam) return nullptr;
    if (career.currentWeek < 1 || career.currentWeek > static_cast<int>(career.schedule.size())) return nullptr;
    for (const auto& match : career.schedule[static_cast<size_t>(career.currentWeek - 1)]) {
        if (match.first < 0 || match.second < 0 ||
            match.first >= static_cast<int>(career.activeTeams.size()) ||
            match.second >= static_cast<int>(career.activeTeams.size())) {
            continue;
        }
        Team* home = career.activeTeams[static_cast<size_t>(match.first)];
        Team* away = career.activeTeams[static_cast<size_t>(match.second)];
        if (home == career.myTeam) return away;
        if (away == career.myTeam) return home;
    }
    return nullptr;
}

int averageFitnessForLine(const Team& team, const string& line) {
    int total = 0;
    int count = 0;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != line) continue;
        total += player.fitness;
        count++;
    }
    return count > 0 ? total / count : 65;
}

int lineThreatScore(const Team& team, const string& line) {
    int total = 0;
    int count = 0;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != line) continue;
        total += player.skill + player.currentForm;
        count++;
    }
    return count > 0 ? total / count : 55;
}

string lineMap(const Team& team) {
    return "Lineas DEF " + to_string(lineThreatScore(team, "DEF")) +
           " | MED " + to_string(lineThreatScore(team, "MED")) +
           " | DEL " + to_string(lineThreatScore(team, "DEL"));
}

string buildOpponentReport(const Career& career) {
    const Team* opponent = nextOpponent(career);
    if (!career.myTeam || !opponent) return "Sin informe rival disponible.";
    int fitness = averageFitnessForLine(*opponent, "MED");
    string vulnerability = opponent->defensiveLine >= 4 ? "espacio a la espalda"
                          : opponent->width >= 4 ? "pasillos interiores"
                          : fitness < 62 ? "fatiga en el mediocampo"
                                         : "bloque ordenado";
    return opponent->name + " | estilo " + (opponent->clubStyle.empty() ? string("equilibrado") : opponent->clubStyle) +
           " | formacion " + opponent->formation +
           " | moral " + to_string(opponent->morale) +
           " | prestigio " + to_string(teamPrestigeScore(*opponent)) +
           " | alerta " + vulnerability +
           (areRivalClubs(*career.myTeam, *opponent) ? " | clasico" : "");
}
