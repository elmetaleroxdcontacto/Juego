#include "career/staff_service.h"

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

}  // namespace staff_service
