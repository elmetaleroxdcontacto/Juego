#include "career/manager_advice.h"

#include "ai/ai_squad_planner.h"
#include "career/career_reports.h"
#include "career/career_support.h"
#include "career/dressing_room_service.h"

#include <algorithm>

using namespace std;

namespace {

void pushUniqueLine(vector<string>& lines, const string& line) {
    if (line.empty()) return;
    if (find(lines.begin(), lines.end(), line) == lines.end()) {
        lines.push_back(line);
    }
}

int shortContractCount(const Team& team) {
    int total = 0;
    for (const auto& player : team.players) {
        if (player.contractWeeks <= 12) ++total;
    }
    return total;
}

int injuredCount(const Team& team) {
    int total = 0;
    for (const auto& player : team.players) {
        if (player.injured) ++total;
    }
    return total;
}

int heavyLoadCount(const Team& team) {
    int total = 0;
    for (const auto& player : team.players) {
        if (player.fitness < 62 || player.fatigueLoad >= 60) ++total;
    }
    return total;
}

int youthProjects(const Team& team) {
    int total = 0;
    for (const auto& player : team.players) {
        if (player.age <= 21 && player.potential >= player.skill + 8) ++total;
    }
    return total;
}

bool monthlyObjectiveUnderPressure(const Career& career) {
    if (career.boardMonthlyObjective.empty()) return false;
    if (career.boardMonthlyObjective.find("posicion") != string::npos) {
        return career.boardMonthlyProgress > 0 && career.boardMonthlyProgress > career.boardMonthlyTarget;
    }
    return career.boardMonthlyProgress < career.boardMonthlyTarget;
}

}  // namespace

namespace manager_advice {

vector<string> buildManagerActionLines(const Career& career, size_t limit) {
    vector<string> lines;
    if (!career.myTeam) return lines;

    const Team& team = *career.myTeam;
    const DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(team, career.currentWeek);
    const SquadNeedReport planner = ai_squad_planner::analyzeSquad(team);
    const int shortContracts = shortContractCount(team);
    const int injured = injuredCount(team);
    const int heavyLoad = heavyLoadCount(team);

    if (heavyLoad >= 4 || dressing.fatigueRiskCount >= 3) {
        pushUniqueLine(lines,
                       "Bajar la carga semanal: hay " + to_string(max(heavyLoad, dressing.fatigueRiskCount)) +
                           " jugadores llegando exigidos.");
    }
    if (dressing.lowMoraleCount >= 3 || dressing.conflictCount > 0 || team.morale < 48) {
        pushUniqueLine(lines,
                       "Gestionar vestuario: conviene reunion o charla individual antes de perder control emocional.");
    }
    if (shortContracts > 0) {
        pushUniqueLine(lines,
                       "Renovar pronto: " + to_string(shortContracts) +
                           " contrato(s) entran en la zona corta de 12 semanas.");
    }
    if (!planner.thinPositions.empty() || planner.rotationRisk >= 55) {
        const string need = planner.weakestPosition.empty() ? "MED" : planner.weakestPosition;
        pushUniqueLine(lines,
                       "Mercado activo: prioriza " + need +
                           " para bajar el riesgo de rotacion del plantel.");
    }
    if (dressing.promiseRiskCount > 0 || !career.activePromises.empty()) {
        pushUniqueLine(lines,
                       "Proteger promesas: hay compromisos deportivos que pueden tensionar el vestuario.");
    }
    if (career.boardConfidence <= 42 || monthlyObjectiveUnderPressure(career)) {
        pushUniqueLine(lines,
                       "Directiva en alerta: toca sumar progreso visible en el objetivo mensual o en la tabla.");
    }
    if (team.debt > team.sponsorWeekly * 10 || team.budget < max(150000LL, team.sponsorWeekly * 3)) {
        pushUniqueLine(lines,
                       "Caja comprometida: evita paquetes salariales altos y considera una venta de equilibrio.");
    }
    if (injured >= 3) {
        pushUniqueLine(lines,
                       "Parte medico exigente: prepara rotacion porque hay " + to_string(injured) + " lesionado(s).");
    }
    if (lines.empty()) {
        pushUniqueLine(lines, "Semana estable: puedes empujar desarrollo, scouting y preparacion del proximo rival.");
    }
    if (lines.size() > limit) lines.resize(limit);
    return lines;
}

vector<string> buildCareerStorylines(const Career& career, size_t limit) {
    vector<string> lines;
    if (!career.myTeam) return lines;

    const Team& team = *career.myTeam;
    const Team* opponent = nextOpponent(career);
    const int rank = career.currentCompetitiveRank();
    const int fieldSize = max(1, career.currentCompetitiveFieldSize());
    const int youthCount = youthProjects(team);

    if (opponent && !team.primaryRival.empty() && opponent->name == team.primaryRival) {
        pushUniqueLine(lines, "Se viene el clasico ante " + opponent->name + ", una semana con carga emocional alta.");
    }
    if (rank > 0 && rank <= max(2, fieldSize / 4)) {
        pushUniqueLine(lines, team.name + " esta en la pelea alta y cada punto empieza a pesar mas.");
    } else if (rank > fieldSize - max(2, fieldSize / 4)) {
        pushUniqueLine(lines, team.name + " entra en zona de presion competitiva y el margen de error se achica.");
    }
    if (career.boardConfidence <= 35) {
        pushUniqueLine(lines, "La directiva entra en zona critica: el proyecto necesita una reaccion inmediata.");
    } else if (career.boardConfidence >= 70) {
        pushUniqueLine(lines, "La directiva respalda el proceso y te da aire para pensar mas alla de una sola fecha.");
    }
    if (career.managerReputation >= 75) {
        pushUniqueLine(lines, "Tu reputacion ya empieza a mover mercado, vestuario y percepcion externa del club.");
    } else if (career.managerReputation <= 45) {
        pushUniqueLine(lines, "El margen del DT aun se construye: cada semana suma o resta credito.");
    }
    if (youthCount >= 3) {
        pushUniqueLine(lines,
                       "La identidad de " + team.name + " puede apoyarse en cantera: hay " +
                           to_string(youthCount) + " proyecto(s) juveniles listos para empujar.");
    }
    if (!career.history.empty()) {
        const SeasonHistoryEntry& last = career.history.back();
        pushUniqueLine(lines,
                       "Memoria reciente: el club llega desde T" + to_string(last.season) +
                           " con cierre " + to_string(last.finish) + " y nota \"" + last.note + "\".");
    }
    if (!career.activePromises.empty()) {
        pushUniqueLine(lines, "El vestuario sigue midiendo si las promesas del manager se cumplen o se rompen.");
    }
    if (!career.scoutingAssignments.empty()) {
        pushUniqueLine(lines,
                       "La red de scouting trabaja en paralelo y puede abrir una oportunidad de mercado en las proximas semanas.");
    }
    if (!career.lastMatchCenter.opponentName.empty()) {
        pushUniqueLine(lines,
                       "El ultimo " + career.lastMatchCenter.competitionLabel +
                           " dejo una lectura tactica que puede marcar la semana.");
    }
    if (lines.empty()) {
        pushUniqueLine(lines, "La temporada sigue abierta: no hay una narrativa dominante todavia.");
    }
    if (lines.size() > limit) lines.resize(limit);
    return lines;
}

string buildTransferCompetitionLabel(const Career& career,
                                     const Team& seller,
                                     const Player& player,
                                     const TransferTarget& target) {
    if (!career.myTeam) return "Mercado indefinido";
    int pressure = target.competitionScore;
    if (player.contractWeeks <= 14) pressure += 8;
    if (player.wantsToLeave) pressure -= 4;
    if (teamPrestigeScore(seller) > teamPrestigeScore(*career.myTeam)) pressure += 5;
    if (target.scoutingConfidence < 55) pressure += 3;
    if (pressure >= 30) return "Competencia alta";
    if (pressure >= 18) return "Mercado movido";
    return "Ventana manejable";
}

string buildTransferActionLabel(const Career& career,
                                const Team& seller,
                                const Player& player,
                                const TransferTarget& target) {
    (void)career;
    (void)seller;
    if (target.urgentNeed && target.affordabilityScore >= 18) return "Mover ahora";
    if (target.contractRunningOut && target.affordabilityScore >= 10) return "Entrar antes del cierre";
    if (target.availableForLoan && target.affordabilityScore < 12) return "Explorar cesion";
    if (target.scoutingConfidence < 60) return "Pedir mas scouting";
    if (player.wantsToLeave && target.affordabilityScore >= 10) return "Presionar negociacion";
    if (target.affordabilityScore < 6) return "Seguir sin ofertar";
    return "Preparar oferta";
}

string buildTransferPackageLabel(const TransferTarget& target) {
    const long long upfrontPackage = target.expectedFee + target.expectedAgentFee;
    return "Entrada " + formatMoneyValue(upfrontPackage) +
           " | salario " + formatMoneyValue(target.expectedWage);
}

}  // namespace manager_advice
