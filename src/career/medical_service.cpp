#include "career/medical_service.h"

#include "utils/utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

string workloadRecommendation(const Player& player, const Team& team) {
    if (player.injured) return "Tratamiento y reintegro progresivo";
    if (player.fatigueLoad >= 75 || player.fitness <= 56) return "Reducir carga y reservar proximo partido";
    if (player.injuryHistory >= 4 && team.medicalTeam <= 62) return "Seguimiento diario por riesgo de recaida";
    if (player.matchesSuspended > 0) return "Usar semana para recuperacion especifica";
    return "Carga controlada";
}

}  // namespace

namespace medical_service {

vector<MedicalStatus> buildMedicalStatuses(const Team& team) {
    vector<MedicalStatus> statuses;
    for (const auto& player : team.players) {
        MedicalStatus status;
        status.playerName = player.name;
        status.unavailable = player.injured || player.matchesSuspended > 0;
        status.weeksOut = player.injured ? player.injuryWeeks : 0;
        status.workloadRisk = clampInt(player.fatigueLoad + max(0, 70 - player.fitness) + max(0, player.age - 30) * 2, 0, 99);
        status.relapseRisk = clampInt(player.injuryHistory * 12 + max(0, 65 - team.medicalTeam) + (player.injured ? 18 : 0) + player.fatigueLoad / 3, 0, 99);
        status.diagnosis = player.injured ? player.injuryType : (status.workloadRisk >= 65 ? "Sobrecarga" : "Disponible");
        status.recommendation = workloadRecommendation(player, team);
        if (player.injured || status.workloadRisk >= 50 || status.relapseRisk >= 45) {
            statuses.push_back(status);
        }
    }
    sort(statuses.begin(), statuses.end(), [](const MedicalStatus& left, const MedicalStatus& right) {
        if (left.unavailable != right.unavailable) return left.unavailable > right.unavailable;
        if (left.relapseRisk != right.relapseRisk) return left.relapseRisk > right.relapseRisk;
        return left.workloadRisk > right.workloadRisk;
    });
    return statuses;
}

string buildMedicalDigest(const Team& team, size_t limit) {
    const auto statuses = buildMedicalStatuses(team);
    if (statuses.empty()) return "- Centro medico sin casos criticos.";
    ostringstream out;
    const size_t count = min(limit, statuses.size());
    for (size_t i = 0; i < count; ++i) {
        if (i) out << "\r\n";
        out << "- " << statuses[i].playerName << " | " << statuses[i].diagnosis
            << " | carga " << statuses[i].workloadRisk
            << " | recaida " << statuses[i].relapseRisk
            << " | " << statuses[i].recommendation;
        if (statuses[i].weeksOut > 0) out << " | baja " << statuses[i].weeksOut << " sem";
    }
    return out.str();
}

}  // namespace medical_service
