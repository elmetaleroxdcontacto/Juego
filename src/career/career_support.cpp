#include "career/career_support.h"

#include "engine/team_personality.h"
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
    career.syncActiveHumanManager();
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
    const TeamPersonalityProfile profile = buildTeamPersonalityProfile(*opponent);
    int midfieldFitness = averageFitnessForLine(*opponent, "MED");
    int defenseThreat = lineThreatScore(*opponent, "DEF");
    int midfieldThreat = lineThreatScore(*opponent, "MED");
    int attackThreat = lineThreatScore(*opponent, "DEL");
    string vulnerability = opponent->defensiveLine >= 4 ? "espacio a la espalda"
                          : opponent->width >= 4 ? "pasillos interiores"
                          : midfieldFitness < 62 ? "fatiga en el mediocampo"
                                                 : "bloque ordenado";
    string offensiveShape = opponent->matchInstruction == "Juego directo" ? "busca ruptura rapida"
                             : opponent->width >= 4 ? "carga bandas y centros"
                             : opponent->tempo >= 4 ? "acelera cada recuperacion"
                                                    : "circula con paciencia";
    string mainThreat = attackThreat >= midfieldThreat + 4 ? "delantera"
                        : midfieldThreat >= defenseThreat + 4 ? "mediocampo"
                                                              : "estructura equilibrada";
    return opponent->name + " | estilo " + (opponent->clubStyle.empty() ? string("equilibrado") : opponent->clubStyle) +
           " | DT " + profile.coachLabel +
           " | mercado " + profile.marketLabel +
           " | formacion " + opponent->formation +
           " | moral " + to_string(opponent->morale) +
           " | prestigio " + to_string(teamPrestigeScore(*opponent)) +
           " | amenaza " + mainThreat +
           " | plan " + offensiveShape +
           " | lineas " + lineMap(*opponent) +
           " | alerta " + vulnerability +
           (areRivalClubs(*career.myTeam, *opponent) ? " | clasico" : "");
}

vector<string> buildNextOpponentPlanLines(const Career& career, size_t limit) {
    vector<string> lines;
    const Team* opponent = nextOpponent(career);
    if (!career.myTeam || !opponent || limit == 0) return lines;

    const Team& team = *career.myTeam;
    const TeamPersonalityProfile profile = buildTeamPersonalityProfile(*opponent);
    const int defenseThreat = lineThreatScore(*opponent, "DEF");
    const int midfieldThreat = lineThreatScore(*opponent, "MED");
    const int attackThreat = lineThreatScore(*opponent, "DEL");
    const int midfieldFitness = averageFitnessForLine(*opponent, "MED");
    const int myHeavyLegs = count_if(team.players.begin(), team.players.end(), [](const Player& player) {
        return player.fitness < 62 || player.fatigueLoad >= 60;
    });

    auto push = [&](const string& line) {
        if (line.empty() || lines.size() >= limit) return;
        if (find(lines.begin(), lines.end(), line) == lines.end()) lines.push_back(line);
    };

    string mainThreat = "estructura equilibrada";
    if (attackThreat >= midfieldThreat + 4 && attackThreat >= defenseThreat + 4) {
        mainThreat = "delantera";
    } else if (midfieldThreat >= attackThreat + 4 && midfieldThreat >= defenseThreat + 4) {
        mainThreat = "mediocampo";
    } else if (defenseThreat >= attackThreat + 4 && defenseThreat >= midfieldThreat + 4) {
        mainThreat = "bloque defensivo y balon parado";
    }

    string vulnerability = "no regalar transiciones y moverlos de lado a lado";
    if (opponent->defensiveLine >= 4) {
        vulnerability = "atacar el espacio a la espalda de la defensa";
    } else if (opponent->width >= 4) {
        vulnerability = "cerrar pasillos interiores y salir por dentro";
    } else if (midfieldFitness < 62) {
        vulnerability = "subir ritmo contra un mediocampo con fatiga";
    } else if (opponent->pressingIntensity >= 4) {
        vulnerability = "superar primera presion con pases simples";
    } else if (opponent->defensiveLine <= 2) {
        vulnerability = "abrir la cancha y cargar centros o segunda jugada";
    }

    string suggestedInstruction = "Equilibrado";
    if (opponent->defensiveLine >= 4) {
        suggestedInstruction = "Juego directo";
    } else if (opponent->width <= 2 || opponent->defensiveLine <= 2) {
        suggestedInstruction = "Por bandas";
    } else if (opponent->pressingIntensity >= 4 || opponent->tempo >= 4) {
        suggestedInstruction = "Pausar juego";
    } else if (midfieldFitness < 62) {
        suggestedInstruction = "Contra-presion";
    }

    push("Rival | " + opponent->name +
         " | formacion " + opponent->formation +
         " | DT " + profile.coachLabel +
         " | moral " + to_string(opponent->morale));
    push("Amenaza | " + mainThreat +
         " | DEF " + to_string(defenseThreat) +
         " MED " + to_string(midfieldThreat) +
         " DEL " + to_string(attackThreat));
    push("Vulnerabilidad | " + vulnerability + ".");
    push("Plan sugerido | instruccion " + suggestedInstruction +
         " | entrenamiento " + (myHeavyLegs >= 4 ? string("Recuperacion") : string("Preparacion partido")) + ".");

    if (areRivalClubs(team, *opponent)) {
        push("Riesgo emocional | clasico: cuida disciplina, promesas y liderazgo del vestuario.");
    } else if (opponent->morale >= 72) {
        push("Riesgo competitivo | rival con moral alta: evita inicio lento y controla los primeros 15 minutos.");
    } else if (opponent->morale <= 42) {
        push("Oportunidad | rival con moral baja: presiona temprano para instalar dudas.");
    } else if (myHeavyLegs >= 4) {
        push("Riesgo fisico | " + to_string(myHeavyLegs) +
             " jugadores llegan exigidos: rota antes de apostar por presion alta.");
    } else {
        push("Decision clave | revisa XI, banca y balon parado antes de simular la semana.");
    }

    return lines;
}
