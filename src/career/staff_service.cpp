#include "career/staff_service.h"

#include "ai/ai_squad_planner.h"
#include "career/career_support.h"
#include "career/dressing_room_service.h"
#include "career/medical_service.h"
#include "finance/finance_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

staff_service::StaffProfile makeProfile(const string& role,
                                        const string& name,
                                        int rating,
                                        const string& impact,
                                        int prestige) {
    staff_service::StaffProfile profile;
    profile.role = role;
    profile.name = name.empty() ? "Sin asignar" : name;
    profile.rating = clampInt(rating, 1, 99);
    profile.weeklyCost = 1800LL + static_cast<long long>(profile.rating) * 75LL + static_cast<long long>(prestige) * 20LL;
    profile.impact = impact;
    if (profile.rating >= 78) profile.status = "Referencia";
    else if (profile.rating >= 64) profile.status = "Solido";
    else if (profile.rating >= 52) profile.status = "Desarrollo";
    else profile.status = "Debil";
    return profile;
}

string severityLabel(int urgency) {
    if (urgency >= 72) return "Urgente";
    if (urgency >= 48) return "Atencion";
    return "Estable";
}

void pushRecommendation(vector<staff_service::StaffRecommendation>& recommendations,
                        const string& staffRole,
                        int urgency,
                        const string& summary,
                        const string& suggestedAction) {
    if (summary.empty()) return;
    staff_service::StaffRecommendation item;
    item.staffRole = staffRole;
    item.urgency = clampInt(urgency, 0, 100);
    item.severity = severityLabel(item.urgency);
    item.summary = summary;
    item.suggestedAction = suggestedAction;
    recommendations.push_back(item);
}

int fatiguedPlayers(const Team& team) {
    int total = 0;
    for (const auto& player : team.players) {
        if (player.fitness < 62 || player.fatigueLoad >= 60) ++total;
    }
    return total;
}

int expiringContracts(const Team& team) {
    int total = 0;
    for (const auto& player : team.players) {
        if (player.contractWeeks > 0 && player.contractWeeks <= 12) ++total;
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

}  // namespace

namespace staff_service {

vector<StaffProfile> buildStaffProfiles(const Team& team) {
    const int prestige = max(1, teamPrestigeScore(team));
    return {
        makeProfile("Asistente", team.assistantCoachName, team.assistantCoach, "Orden tactico, charlas y cohesion competitiva", prestige),
        makeProfile("Preparador", team.fitnessCoachName, team.fitnessCoach, "Carga fisica, recuperacion y resistencia", prestige),
        makeProfile("Scouting", team.scoutingChiefName, team.scoutingChief, "Cobertura regional, shortlist y precision de informes", prestige),
        makeProfile("Juveniles", team.youthCoachName, team.youthCoach, "Intake, potencial y desarrollo de cantera", prestige),
        makeProfile("Medico", team.medicalChiefName, team.medicalTeam, "Prevencion, recaidas y tiempo de recuperacion", prestige),
        makeProfile("Arqueros", team.goalkeepingCoachName, team.goalkeepingCoach, "Rendimiento y forma de los porteros", prestige),
        makeProfile("Analista", team.performanceAnalystName, team.performanceAnalyst, "Lectura rival, microciclo y detalle del partido", prestige),
    };
}

string weakestStaffRole(const Team& team) {
    const auto profiles = buildStaffProfiles(team);
    auto it = min_element(profiles.begin(), profiles.end(), [](const StaffProfile& left, const StaffProfile& right) {
        if (left.rating != right.rating) return left.rating < right.rating;
        return left.role < right.role;
    });
    return it == profiles.end() ? string("Asistente") : it->role;
}

string buildStaffDigest(const Team& team, size_t limit) {
    const auto profiles = buildStaffProfiles(team);
    ostringstream out;
    const size_t count = min(limit, profiles.size());
    for (size_t i = 0; i < count; ++i) {
        if (i) out << "\r\n";
        out << "- " << profiles[i].role << ": " << profiles[i].name
            << " | nivel " << profiles[i].rating
            << " | " << profiles[i].status;
    }
    return out.str();
}

vector<StaffRecommendation> buildStaffRecommendations(const Career& career, size_t limit) {
    vector<StaffRecommendation> recommendations;
    if (!career.myTeam) return recommendations;

    const Team& team = *career.myTeam;
    const DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(team, career.currentWeek);
    const vector<medical_service::MedicalStatus> medical = medical_service::buildMedicalStatuses(team);
    const WeeklyFinanceReport finance = finance_system::projectWeeklyReport(team);
    const SquadNeedReport planner = ai_squad_planner::analyzeSquad(team);
    const int fatigueCount = fatiguedPlayers(team);
    const int contractCount = expiringContracts(team);
    const int youthCount = youthProjects(team);
    const int injuryCount = static_cast<int>(count_if(team.players.begin(), team.players.end(), [](const Player& player) {
        return player.injured;
    }));
    const int highRelapseCount = static_cast<int>(count_if(medical.begin(), medical.end(), [](const medical_service::MedicalStatus& status) {
        return status.relapseRisk >= 60 || status.workloadRisk >= 65;
    }));
    const string urgentPosition = planner.weakestPosition.empty() ? string("MED") : planner.weakestPosition;

    pushRecommendation(
        recommendations,
        "Asistente",
        dressing.socialTension * 9 + dressing.promiseRiskCount * 8 + max(0, 45 - career.boardConfidence),
        dressing.socialTension >= 5 || dressing.promiseRiskCount > 0
            ? "Detecta un vestuario sensible: " + dressing.summary + "."
            : "Ve un grupo controlado para sostener el plan semanal.",
        dressing.socialTension >= 5 || dressing.promiseRiskCount > 0 ? "Convocar reunion o charla individual."
                                                                     : "Mantener coherencia tactica y rotacion.");

    pushRecommendation(
        recommendations,
        "Preparador fisico",
        fatigueCount * 14 + max(0, 62 - team.morale) / 2 + (team.trainingFocus == "Recuperacion" ? -8 : 0),
        fatigueCount >= 4
            ? "Marca " + to_string(fatigueCount) + " jugadores exigidos por carga y fisico."
            : "La carga del grupo esta dentro de rangos razonables.",
        fatigueCount >= 4 ? "Bajar intensidad o rotar antes del proximo partido." : "Mantener microciclo actual.");

    pushRecommendation(
        recommendations,
        "Medico",
        injuryCount * 16 + highRelapseCount * 12,
        injuryCount > 0
            ? "Reporta " + to_string(injuryCount) + " lesionado(s) y " + to_string(highRelapseCount) +
                  " caso(s) con riesgo de recaida."
            : "No detecta bajas largas ni recaidas inmediatas.",
        injuryCount > 0 || highRelapseCount > 0 ? "Proteger cargas y revisar instrucciones individuales."
                                                : "Seguir con prevencion y control semanal.");

    pushRecommendation(
        recommendations,
        "Scouting",
        (career.scoutingAssignments.empty() ? 18 : 0) + (career.scoutingShortlist.empty() ? 16 : 0) +
            max(0, planner.rotationRisk - 45),
        career.scoutingAssignments.empty() && career.scoutingShortlist.empty()
            ? "Pide abrir radar nuevo para la posicion " + urgentPosition + "."
            : "Mantiene " + to_string(career.scoutingAssignments.size()) + " asignacion(es) y " +
                  to_string(career.scoutingShortlist.size()) + " objetivo(s) en seguimiento.",
        career.scoutingAssignments.empty() ? "Activar una asignacion y bajar incertidumbre de mercado."
                                           : "Actualizar shortlist y cerrar informes de mayor confianza.");

    pushRecommendation(
        recommendations,
        "Juveniles",
        max(0, 3 - youthCount) * 16 + max(0, 55 - team.youthCoach) / 2,
        youthCount >= 2
            ? "Ve " + to_string(youthCount) + " proyecto(s) juveniles empujando la plantilla."
            : "Avisa que la camada visible aun es corta para sostener rotacion y valor futuro.",
        youthCount >= 2 ? "Dar minutos selectivos y proteger el desarrollo."
                        : "Invertir en cantera o revisar la region de captacion.");

    pushRecommendation(
        recommendations,
        "Analista",
        max(0, 55 - team.performanceAnalyst) + max(0, 40 - career.boardConfidence) / 2,
        career.lastMatchCenter.opponentName.empty()
            ? "Todavia no hay un ultimo rival claro cargado para lectura fina."
            : "Resume rival y ultimo partido: " + buildOpponentReport(career),
        career.lastMatchCenter.opponentName.empty() ? "Preparar contexto rival y patrones de partido."
                                                    : "Ajustar instruccion segun lectura rival.");

    pushRecommendation(
        recommendations,
        "Direccion deportiva",
        (finance.transferBuffer < 0 ? 24 : 0) + contractCount * 8 + max(0, static_cast<int>(team.debt / 90000LL)),
        contractCount > 0
            ? "Advierte " + to_string(contractCount) + " contrato(s) entrando en zona critica."
            : "No ve urgencias contractuales inmediatas.",
        finance.transferBuffer < 0 ? "Priorizar renovaciones clave y una venta de equilibrio."
                                   : "Ordenar renovaciones antes de cargar salarios nuevos.");

    sort(recommendations.begin(), recommendations.end(), [](const StaffRecommendation& left, const StaffRecommendation& right) {
        if (left.urgency != right.urgency) return left.urgency > right.urgency;
        return left.staffRole < right.staffRole;
    });
    if (recommendations.size() > limit) recommendations.resize(limit);
    return recommendations;
}

vector<string> buildWeeklyStaffBriefingLines(const Career& career, size_t limit) {
    vector<string> lines;
    for (const auto& recommendation : buildStaffRecommendations(career, limit)) {
        lines.push_back(recommendation.staffRole + " | " + recommendation.severity + " | " +
                        recommendation.summary + " | Accion: " + recommendation.suggestedAction);
    }
    if (lines.empty()) lines.push_back("Staff | Estable | No hay alertas nuevas del cuerpo tecnico.");
    return lines;
}

string buildStaffRecommendationDigest(const Career& career, size_t limit) {
    ostringstream out;
    out << "Mesa del staff\r\n";
    const auto recommendations = buildStaffRecommendations(career, limit);
    if (recommendations.empty()) {
        out << "- No hay recomendaciones activas del staff.\r\n";
        return out.str();
    }
    for (const auto& recommendation : recommendations) {
        out << "- " << recommendation.staffRole << " [" << recommendation.severity << "] "
            << recommendation.summary << " Accion: " << recommendation.suggestedAction << "\r\n";
    }
    return out.str();
}

}  // namespace staff_service
