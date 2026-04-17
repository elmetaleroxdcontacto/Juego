#include "engine/team_personality.h"

#include "engine/models.h"
#include "utils/utils.h"

using namespace std;

namespace {

string detectCoachLabel(const Team& team) {
    const string raw = toLower(team.headCoachStyle);
    if (raw.find("intens") != string::npos || raw.find("presion") != string::npos) return "Intensidad";
    if (raw.find("trans") != string::npos || raw.find("vertical") != string::npos) return "Transicion";
    if (raw.find("orden") != string::npos || raw.find("contencion") != string::npos) return "Orden";
    if (raw.find("ofens") != string::npos) return "Ofensivo";
    if (raw.find("control") != string::npos) return "Control";
    if (team.tactics == "Pressing") return "Intensidad";
    if (team.tactics == "Counter" || team.matchInstruction == "Juego directo") return "Transicion";
    if (team.tactics == "Defensive" || team.matchInstruction == "Bloque bajo") return "Orden";
    if (team.matchInstruction == "Por bandas") return "Ofensivo";
    return "Equilibrado";
}

string detectClubLabel(const Team& team) {
    if (!team.clubStyle.empty()) return team.clubStyle;
    if (team.tactics == "Pressing" || team.pressingIntensity >= 4) return "Presion vertical";
    if (team.matchInstruction == "Juego directo" || team.tempo >= 4) return "Transicion directa";
    if (team.matchInstruction == "Por bandas" || team.width >= 4) return "Ataque por bandas";
    if (team.tactics == "Defensive" || team.defensiveLine <= 2) return "Bloque ordenado";
    if (team.trainingFacilityLevel >= 3) return "Control de posesion";
    return "Equilibrio competitivo";
}

string detectMarketLabel(const Team& team) {
    const string raw = toLower(team.transferPolicy);
    if (raw.find("cantera") != string::npos || raw.find("valor futuro") != string::npos) {
        return "Cantera y valor futuro";
    }
    if (raw.find("vender") != string::npos || raw.find("venta") != string::npos ||
        raw.find("austera") != string::npos) {
        return "Vender antes de comprar";
    }
    if (raw.find("titulares") != string::npos || raw.find("rearme") != string::npos) {
        return "Competir por titulares hechos";
    }
    if (raw.find("oportunidades") != string::npos) return "Mercado de oportunidades";
    if (team.debt > team.sponsorWeekly * 18) return "Vender antes de comprar";
    if (team.youthFacilityLevel >= 4 || team.youthCoach >= 72) return "Cantera y valor futuro";
    if (team.clubPrestige >= 70) return "Competir por titulares hechos";
    return "Mercado de oportunidades";
}

string detectYouthLabel(const Team& team) {
    if (!team.youthIdentity.empty()) return team.youthIdentity;
    if (team.youthFacilityLevel >= 4 || team.youthCoach >= 75) return "Cantera estructurada";
    if (team.youthFacilityLevel >= 3 || team.trainingFacilityLevel >= 3) return "Desarrollo mixto";
    if (team.fanBase >= 28 && team.clubPrestige >= 60) return "Plantel de mercado";
    return "Talento local";
}

void clampProfile(TeamPersonalityProfile& profile) {
    profile.pressBias = clampInt(profile.pressBias, 0, 100);
    profile.transitionBias = clampInt(profile.transitionBias, 0, 100);
    profile.widthBias = clampInt(profile.widthBias, 0, 100);
    profile.blockBias = clampInt(profile.blockBias, 0, 100);
    profile.controlBias = clampInt(profile.controlBias, 0, 100);
    profile.riskAppetite = clampInt(profile.riskAppetite, 0, 100);
    profile.adaptability = clampInt(profile.adaptability, 0, 100);
    profile.youthTrust = clampInt(profile.youthTrust, 0, 100);
    profile.marketPatience = clampInt(profile.marketPatience, 0, 100);
    profile.starterBias = clampInt(profile.starterBias, 0, 100);
    profile.saleBias = clampInt(profile.saleBias, 0, 100);
}

}  // namespace

TeamPersonalityProfile buildTeamPersonalityProfile(const Team& team) {
    TeamPersonalityProfile profile;
    profile.coachLabel = detectCoachLabel(team);
    profile.clubLabel = detectClubLabel(team);
    profile.marketLabel = detectMarketLabel(team);
    profile.youthLabel = detectYouthLabel(team);
    profile.adaptability = clampInt(42 + team.headCoachReputation / 3 + team.performanceAnalyst / 5, 35, 90);
    profile.riskAppetite = 50;
    profile.marketPatience = 50;

    if (profile.coachLabel == "Intensidad") {
        profile.pressBias += 34;
        profile.riskAppetite += 18;
        profile.adaptability += 8;
    } else if (profile.coachLabel == "Transicion") {
        profile.transitionBias += 34;
        profile.riskAppetite += 14;
        profile.adaptability += 6;
    } else if (profile.coachLabel == "Orden") {
        profile.blockBias += 34;
        profile.riskAppetite -= 16;
        profile.marketPatience += 8;
    } else if (profile.coachLabel == "Ofensivo") {
        profile.widthBias += 24;
        profile.riskAppetite += 22;
        profile.starterBias += 10;
    } else if (profile.coachLabel == "Control") {
        profile.controlBias += 28;
        profile.marketPatience += 12;
    } else {
        profile.controlBias += 8;
        profile.adaptability += 4;
    }

    if (profile.clubLabel == "Presion vertical") {
        profile.pressBias += 16;
        profile.riskAppetite += 10;
    } else if (profile.clubLabel == "Transicion directa") {
        profile.transitionBias += 16;
        profile.riskAppetite += 8;
    } else if (profile.clubLabel == "Ataque por bandas") {
        profile.widthBias += 18;
        profile.riskAppetite += 6;
    } else if (profile.clubLabel == "Bloque ordenado") {
        profile.blockBias += 18;
        profile.riskAppetite -= 10;
        profile.marketPatience += 6;
    } else if (profile.clubLabel == "Control de posesion") {
        profile.controlBias += 18;
        profile.marketPatience += 10;
    } else {
        profile.controlBias += 6;
    }

    if (profile.marketLabel == "Cantera y valor futuro") {
        profile.youthTrust += 26;
        profile.marketPatience += 18;
        profile.starterBias -= 10;
    } else if (profile.marketLabel == "Vender antes de comprar") {
        profile.saleBias += 28;
        profile.marketPatience += 12;
        profile.riskAppetite -= 18;
    } else if (profile.marketLabel == "Competir por titulares hechos") {
        profile.starterBias += 28;
        profile.riskAppetite += 12;
        profile.marketPatience -= 8;
    } else {
        profile.marketPatience += 4;
    }

    if (profile.youthLabel == "Cantera estructurada") {
        profile.youthTrust += 24;
        profile.marketPatience += 10;
    } else if (profile.youthLabel == "Desarrollo mixto") {
        profile.youthTrust += 14;
    } else if (profile.youthLabel == "Plantel de mercado") {
        profile.starterBias += 14;
        profile.youthTrust -= 8;
    } else {
        profile.youthTrust += 8;
    }

    if (team.jobSecurity <= 35) {
        profile.riskAppetite += 8;
        profile.adaptability += 6;
    }
    if (team.debt > team.sponsorWeekly * 18) {
        profile.saleBias += 18;
        profile.starterBias -= 8;
    }
    if (team.clubPrestige >= 72) {
        profile.starterBias += 10;
    }
    if (team.performanceAnalyst >= 75) {
        profile.adaptability += 6;
    }

    clampProfile(profile);
    return profile;
}

string teamPersonalityHeadline(const Team& team) {
    const TeamPersonalityProfile profile = buildTeamPersonalityProfile(team);
    return "DT " + profile.coachLabel + " | mercado " + profile.marketLabel + " | cantera " + profile.youthLabel;
}

vector<string> teamPersonalitySummaryLines(const Team& team, size_t limit) {
    const TeamPersonalityProfile profile = buildTeamPersonalityProfile(team);
    vector<string> lines;
    lines.push_back("DT " + profile.coachLabel + " | adaptabilidad " + to_string(profile.adaptability) +
                    " | riesgo " + to_string(profile.riskAppetite));
    lines.push_back("Mercado " + profile.marketLabel + " | impacto inmediato " + to_string(profile.starterBias) +
                    " | venta " + to_string(profile.saleBias));
    lines.push_back("Cantera " + profile.youthLabel + " | confianza juvenil " + to_string(profile.youthTrust) +
                    " | paciencia " + to_string(profile.marketPatience));
    lines.push_back("Juego " + profile.clubLabel + " | presion " + to_string(profile.pressBias) +
                    " | transicion " + to_string(profile.transitionBias) +
                    " | control " + to_string(profile.controlBias));
    if (lines.size() > limit) lines.resize(limit);
    return lines;
}
