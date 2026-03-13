#include "development/training_impact_system.h"

#include "utils/utils.h"

#include <algorithm>
#include <sstream>
#include <vector>

using namespace std;

namespace {

using development::TrainingSessionPlan;

vector<TrainingSessionPlan> congestedSchedule(const string& focus) {
    return {
        {"Lun", "Recuperacion", "Bajar carga tras el ultimo partido", 1},
        {"Mar", "Tactico", "Ajustes cortos para sostener automatismos", 2},
        {"Mie", focus == "Defensa" ? "Defensa" : (focus == "Ataque" ? "Ataque" : "Preparacion partido"),
         "Sesion breve orientada al proximo rival", 2},
        {"Jue", "Recuperacion", "Reset fisico para evitar sobrecarga", 1},
        {"Vie", "Balon parado", "Pelota quieta y detalles competitivos", 1},
        {"Sab", "Activacion", "Rutina de baja carga antes del partido", 1},
    };
}

vector<TrainingSessionPlan> regularSchedule(const string& focus) {
    if (focus == "Ataque") {
        return {
            {"Lun", "Recuperacion", "Regenerativo y analisis del ultimo partido", 1},
            {"Mar", "Ataque", "Movimientos de area y finalizacion", 3},
            {"Mie", "Tecnico", "Controles, apoyos y ultimo pase", 3},
            {"Jue", "Tactico", "Patrones ofensivos y alturas", 2},
            {"Vie", "Balon parado", "Corners y faltas ofensivas", 2},
            {"Sab", "Activacion", "Afinar sensaciones sin fatigar", 1},
        };
    }
    if (focus == "Defensa") {
        return {
            {"Lun", "Recuperacion", "Reset fisico y video defensivo", 1},
            {"Mar", "Defensa", "Coberturas y duelos", 3},
            {"Mie", "Tactico", "Bloque, linea y presion", 3},
            {"Jue", "Resistencia", "Capacidad de repetir esfuerzos", 2},
            {"Vie", "Balon parado", "Defensa de corners y faltas", 2},
            {"Sab", "Activacion", "Ajustes finales de bloque", 1},
        };
    }
    if (focus == "Resistencia" || focus == "Fisico") {
        return {
            {"Lun", "Recuperacion", "Regenerativo y control medico", 1},
            {"Mar", "Resistencia", "Carga aerobica principal", 3},
            {"Mie", "Fisico", "Fuerza y potencia", 3},
            {"Jue", "Tecnico", "Trabajo con balon en fatiga controlada", 2},
            {"Vie", "Preparacion partido", "Reducir carga y afinar detalles", 1},
            {"Sab", "Activacion", "Entrada en calor corta", 1},
        };
    }
    if (focus == "Recuperacion") {
        return {
            {"Lun", "Recuperacion", "Regenerativo", 1},
            {"Mar", "Recuperacion", "Trabajo medico individual", 1},
            {"Mie", "Tactico", "Repaso liviano sin carga alta", 1},
            {"Jue", "Tecnico", "Rutina suave con balon", 1},
            {"Vie", "Preparacion partido", "Afinar once y pelotas detenidas", 1},
            {"Sab", "Activacion", "Mantenimiento", 1},
        };
    }
    if (focus == "Tactico" || focus == "Preparacion partido") {
        return {
            {"Lun", "Recuperacion", "Bajar pulsaciones y revisar video", 1},
            {"Mar", "Tactico", "Automatismos del sistema", 3},
            {"Mie", "Defensa", "Repliegue y presion coordinada", 2},
            {"Jue", "Ataque", "Progresion y llegadas", 2},
            {"Vie", "Balon parado", "Detalles competitivos", 2},
            {"Sab", "Activacion", "Recordatorio tactico final", 1},
        };
    }
    return {
        {"Lun", "Recuperacion", "Regenerativo y seguimiento medico", 1},
        {"Mar", "Tecnico", "Trabajo de control y pase", 2},
        {"Mie", "Tactico", "Principios colectivos", 2},
        {"Jue", "Resistencia", "Capacidad fisica base", 2},
        {"Vie", "Balon parado", "Pelota quieta en ambos arcos", 2},
        {"Sab", "Activacion", "Afinar sin sobrecargar", 1},
    };
}

int countSessions(const vector<TrainingSessionPlan>& schedule, const vector<string>& focuses) {
    int total = 0;
    for (const auto& session : schedule) {
        if (find(focuses.begin(), focuses.end(), session.focus) != focuses.end()) total++;
    }
    return total;
}

int averageLoad(const vector<TrainingSessionPlan>& schedule) {
    if (schedule.empty()) return 1;
    int total = 0;
    for (const auto& session : schedule) total += session.load;
    return max(1, total / static_cast<int>(schedule.size()));
}

}  // namespace

namespace development {

vector<TrainingSessionPlan> buildWeeklyTrainingSchedule(const Team& team, bool congestedWeek) {
    const string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    return congestedWeek ? congestedSchedule(focus) : regularSchedule(focus);
}

string formatWeeklyTrainingSchedule(const Team& team, bool congestedWeek) {
    const vector<TrainingSessionPlan> schedule = buildWeeklyTrainingSchedule(team, congestedWeek);
    ostringstream out;
    out << "Microciclo " << (congestedWeek ? "congestionado" : "regular")
        << " | plan " << (team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus);
    for (const auto& session : schedule) {
        out << "\r\n\r\n- " << session.day << ": " << session.focus << " | " << session.note;
    }
    return out.str();
}

void applyWeeklyTrainingImpact(Team& team, bool congestedWeek) {
    const string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    const vector<TrainingSessionPlan> schedule = buildWeeklyTrainingSchedule(team, congestedWeek);
    const int facilityBonus = max(0, team.trainingFacilityLevel - 1);
    const int assistantBonus = max(0, team.assistantCoach - 55) / 15;
    const int fitnessBonus = max(0, team.fitnessCoach - 55) / 15;
    const int medicalBonus = max(0, team.medicalTeam - 55) / 15;
    const int youthBonus = max(0, team.youthCoach - 55) / 15;

    const int attackSessions = countSessions(schedule, {"Ataque"});
    const int defenseSessions = countSessions(schedule, {"Defensa"});
    const int enduranceSessions = countSessions(schedule, {"Resistencia", "Fisico"});
    const int recoverySessions = countSessions(schedule, {"Recuperacion"});
    const int tacticalSessions = countSessions(schedule, {"Tactico", "Preparacion partido"});
    const int technicalSessions = countSessions(schedule, {"Tecnico"});
    const int setPieceSessions = countSessions(schedule, {"Balon parado"});
    const int scheduleLoad = averageLoad(schedule);

    for (auto& player : team.players) {
        if (focus == "Recuperacion" && recoverySessions >= 2) {
            if (player.injured) {
                player.injuryWeeks = max(0, player.injuryWeeks - (1 + max(0, team.medicalTeam - 60) / 18));
                if (player.injuryWeeks == 0) {
                    player.injured = false;
                    player.injuryType.clear();
                }
            }
            player.fatigueLoad = max(0, player.fatigueLoad - (8 + recoverySessions * 2 + fitnessBonus + medicalBonus));
            player.fitness = clampInt(player.fitness + 3 + recoverySessions + facilityBonus + fitnessBonus + medicalBonus, 15, player.stamina);
            player.moraleMomentum = clampInt(player.moraleMomentum + 1, -25, 25);
            continue;
        }

        if (player.injured) continue;

        int recovery = 1 + facilityBonus + fitnessBonus + medicalBonus / 2 + recoverySessions;
        recovery += tacticalSessions / 2;
        recovery += congestedWeek ? 1 : 0;
        recovery += (player.age <= 21) ? youthBonus / 2 : 0;
        recovery -= max(0, player.fatigueLoad - 35) / 22;
        recovery = max(1, recovery);
        player.fitness = clampInt(player.fitness + recovery, 15, player.stamina);
        player.fatigueLoad = max(0, player.fatigueLoad - max(2, recovery + recoverySessions));

        if (tacticalSessions > 0) {
            player.tacticalDiscipline = clampInt(player.tacticalDiscipline + tacticalSessions / 2, 1, 99);
            player.currentForm = clampInt(player.currentForm + min(2, tacticalSessions), 1, 99);
        }
        if (technicalSessions > 0) {
            player.currentForm = clampInt(player.currentForm + 1, 1, 99);
        }
        if (setPieceSessions > 0 && randInt(1, 100) <= clampInt(6 + setPieceSessions * 4 + assistantBonus, 8, 30)) {
            player.setPieceSkill = min(99, player.setPieceSkill + 1);
        }

        int baseGrowth = 8 + max(0, player.potential - player.skill) / 4 + max(0, 23 - player.age) +
                         max(0, player.professionalism - 55) / 10 + max(0, player.happiness - 50) / 12 +
                         facilityBonus * 2 + assistantBonus + youthBonus;
        baseGrowth += attackSessions * 3 + defenseSessions * 3 + technicalSessions * 2 + tacticalSessions * 2;
        baseGrowth += setPieceSessions;
        baseGrowth -= scheduleLoad + player.fatigueLoad / 14;
        if (congestedWeek) baseGrowth -= 2;
        baseGrowth = clampInt(baseGrowth, 5, 32);

        if (player.skill < player.potential && randInt(1, 100) <= baseGrowth) {
            player.skill = min(100, player.skill + 1);
            const string normalized = normalizePosition(player.position);
            if ((attackSessions > defenseSessions && normalized != "ARQ") || normalized == "DEL") {
                player.attack = min(100, player.attack + 1);
            }
            if ((defenseSessions >= attackSessions && normalized != "DEL") || normalized == "DEF" || normalized == "ARQ") {
                player.defense = min(100, player.defense + 1);
            }
            if (normalized == "MED" && attackSessions > 0 && defenseSessions > 0) {
                player.attack = min(100, player.attack + 1);
                player.defense = min(100, player.defense + 1);
            }
            player.moraleMomentum = clampInt(player.moraleMomentum + 1, -25, 25);
        }

        if (enduranceSessions > 0 && randInt(1, 100) <= clampInt(8 + enduranceSessions * 5 + facilityBonus * 2 + fitnessBonus * 2, 8, 30)) {
            player.stamina = min(100, player.stamina + 1);
            player.fitness = min(player.fitness, player.stamina);
        }
        if (player.age <= 23) {
            int potentialShift = 0;
            if (player.matchesPlayed >= max(1, team.points / 6) || player.startsThisSeason >= 2) potentialShift += 1;
            if (player.happiness >= 62 && player.fatigueLoad <= 30 && recoverySessions > 0) potentialShift += 1;
            if (player.happiness <= 40 || player.fatigueLoad >= 70) potentialShift -= 1;
            if (player.professionalism >= 74) potentialShift += 1;
            if (potentialShift >= 2 && randInt(1, 100) <= 10 + youthBonus * 2 + technicalSessions) {
                player.potential = min(99, player.potential + 1);
            } else if (potentialShift <= -1 && player.potential > player.skill && randInt(1, 100) <= 8) {
                player.potential = max(player.skill, player.potential - 1);
            }
        }
    }

    int moraleBoost = tacticalSessions + recoverySessions + (congestedWeek ? 1 : 0);
    if (focus == "Ataque" || focus == "Defensa") moraleBoost += 1;
    team.morale = clampInt(team.morale + moraleBoost / 2 + assistantBonus, 0, 100);
}

}  // namespace development
