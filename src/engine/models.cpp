#include "models.h"

#include "competition/competition.h"
#include "competition/league_registry.h"
#include "io/io.h"
#include "utils/utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

static constexpr int kCareerSaveVersion = 7;

void applyPositionStats(Player& p) {
    string norm = normalizePosition(p.position);
    if (norm == "ARQ") {
        p.attack = clampInt(p.skill - 30, 10, 70);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (norm == "DEF") {
        p.attack = clampInt(p.skill - 10, 20, 90);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (norm == "MED") {
        p.attack = clampInt(p.skill, 25, 99);
        p.defense = clampInt(p.skill, 25, 99);
    } else if (norm == "DEL") {
        p.attack = clampInt(p.skill + 10, 30, 99);
        p.defense = clampInt(p.skill - 10, 20, 90);
    }
}

string defaultRoleForPosition(const string& position) {
    string norm = normalizePosition(position);
    if (norm == "ARQ") return "Tradicional";
    if (norm == "DEF") return "Stopper";
    if (norm == "MED") return "BoxToBox";
    if (norm == "DEL") return "Poacher";
    return "Default";
}

string defaultDutyForPosition(const string& position) {
    string norm = normalizePosition(position);
    if (norm == "ARQ") return "Defensa";
    if (norm == "DEF") return "Defensa";
    if (norm == "MED") return "Apoyo";
    if (norm == "DEL") return "Ataque";
    return "Apoyo";
}

string defaultDutyForRole(const string& role, const string& position) {
    const string compact = toLower(trim(role));
    if (compact == "pivote" || compact == "stopper" || compact == "tradicional") return "Defensa";
    if (compact == "carrilero" || compact == "enganche" || compact == "poacher" || compact == "objetivo") return "Ataque";
    if (compact == "organizador" || compact == "boxtobox" || compact == "falso9" || compact == "pressing") return "Apoyo";
    return defaultDutyForPosition(position);
}

string defaultInstructionForPosition(const string& position) {
    (void)position;
    return "Libre";
}

string defaultDevelopmentPlanForPosition(const string& position) {
    string norm = normalizePosition(position);
    if (norm == "ARQ") return "Reflejos";
    if (norm == "DEF") return "Defensa";
    if (norm == "MED") return "Creatividad";
    if (norm == "DEL") return "Finalizacion";
    return "Equilibrado";
}

static string randomPreferredFoot() {
    int roll = randInt(1, 100);
    if (roll <= 68) return "Derecho";
    if (roll <= 92) return "Izquierdo";
    return "Ambos";
}

static void addUniquePosition(vector<string>& positions, const string& value) {
    string normalized = normalizePosition(value);
    if (normalized == "N/A") return;
    if (find(positions.begin(), positions.end(), normalized) == positions.end()) {
        positions.push_back(normalized);
    }
}

static vector<string> inferSecondaryPositions(const Player& p) {
    vector<string> positions;
    string primary = normalizePosition(p.position);
    if (primary == "ARQ") return positions;

    if (primary == "DEF") {
        if (p.versatility >= 48 || p.attack >= p.defense - 6 || p.stamina >= 68) addUniquePosition(positions, "MED");
    } else if (primary == "MED") {
        if (p.defense >= p.attack - 4 || p.tacticalDiscipline >= 64) addUniquePosition(positions, "DEF");
        if (p.attack >= p.defense - 2 || p.bigMatches >= 62) addUniquePosition(positions, "DEL");
    } else if (primary == "DEL") {
        if (p.versatility >= 42 || p.stamina >= 66) addUniquePosition(positions, "MED");
    }

    if (p.versatility >= 76) {
        if (primary != "DEF") addUniquePosition(positions, "DEF");
        if (primary != "MED") addUniquePosition(positions, "MED");
        if (primary != "DEL") addUniquePosition(positions, "DEL");
    }
    return positions;
}

static string inferSocialGroup(const Player& p) {
    if (p.leadership >= 72 || playerHasTrait(p, "Lider")) return "Lideres";
    if (p.age <= 20 || playerHasTrait(p, "Proyecto")) return "Juveniles";
    if (p.age >= 31 || playerHasTrait(p, "Fragil")) return "Veteranos";
    if (p.ambition >= 70 && p.professionalism >= 60) return "Competitivos";
    return "Nucleo";
}

static string inferPersonality(const Player& p) {
    if (p.leadership >= 78 && p.professionalism >= 66) return "Lider";
    if (p.professionalism >= 76 && p.discipline >= 68) return "Profesional";
    if (p.ambition >= 76 && p.discipline <= 52) return "Temperamental";
    if (p.adaptation >= 74 && p.versatility >= 62) return "Adaptable";
    if (p.consistency <= 44 || p.currentForm <= 38) return "Irregular";
    if (p.age <= 20 && p.potential >= p.skill + 10) return "Promesa";
    return "Equilibrado";
}

void ensurePlayerProfile(Player& p, bool regenerateTraits) {
    string normalizedPos = normalizePosition(p.position);
    if (normalizedPos == "N/A") normalizedPos = "MED";
    p.position = normalizedPos;
    if (p.preferredFoot.empty()) p.preferredFoot = randomPreferredFoot();
    if (p.consistency <= 0) {
        int base = 38 + p.professionalism / 3 + p.happiness / 8 + randInt(-6, 8);
        p.consistency = clampInt(base, 1, 99);
    }
    if (p.bigMatches <= 0) {
        int base = 34 + p.ambition / 3 + p.leadership / 6 + randInt(-8, 9);
        p.bigMatches = clampInt(base, 1, 99);
    }
    if (p.currentForm <= 0) {
        int base = 44 + p.skill / 5 + randInt(-10, 10);
        p.currentForm = clampInt(base, 1, 99);
    }
    if (p.tacticalDiscipline <= 0) {
        int base = 36 + p.professionalism / 2 + p.chemistry / 6 + randInt(-8, 8);
        p.tacticalDiscipline = clampInt(base, 1, 99);
    }
    if (p.discipline <= 0) {
        int base = (p.tacticalDiscipline * 2 + p.professionalism + p.consistency) / 4 + randInt(-3, 4);
        p.discipline = clampInt(base, 1, 99);
    } else {
        p.discipline = clampInt(p.discipline, 1, 99);
    }
    if (p.injuryProneness <= 0) {
        int base = 12 + p.injuryHistory * 12 + max(0, 66 - p.stamina) / 2 + max(0, p.age - 30) * 2;
        if (playerHasTrait(p, "Fragil")) base += 12;
        p.injuryProneness = clampInt(base + randInt(-3, 5), 1, 99);
    } else {
        p.injuryProneness = clampInt(p.injuryProneness, 1, 99);
    }
    if (p.versatility <= 0) {
        int base = 30 + p.stamina / 4 + p.professionalism / 5 + randInt(-8, 12);
        p.versatility = clampInt(base, 1, 99);
    }
    if (p.adaptation <= 0) {
        int base = 34 + p.versatility / 3 + p.professionalism / 4 + p.ambition / 8 + randInt(-4, 6);
        p.adaptation = clampInt(base, 1, 99);
    } else {
        p.adaptation = clampInt(p.adaptation, 1, 99);
    }
    if (p.personality.empty()) p.personality = inferPersonality(p);
    if (p.secondaryPositions.empty()) p.secondaryPositions = inferSecondaryPositions(p);
    if (p.role.empty()) p.role = defaultRoleForPosition(p.position);
    if (p.roleDuty.empty()) p.roleDuty = defaultDutyForRole(p.role, p.position);
    if (p.individualInstruction.empty()) p.individualInstruction = defaultInstructionForPosition(p.position);
    if (p.developmentPlan.empty()) p.developmentPlan = defaultDevelopmentPlanForPosition(p.position);
    if (p.promisedRole.empty()) p.promisedRole = "Sin promesa";
    if (p.promisedPosition.empty()) p.promisedPosition = p.position;
    if (p.socialGroup.empty()) p.socialGroup = inferSocialGroup(p);
    p.moraleMomentum = clampInt(p.moraleMomentum, -25, 25);
    p.fatigueLoad = clampInt(p.fatigueLoad, 0, 100);
    p.unhappinessWeeks = clampInt(p.unhappinessWeeks, 0, 52);
    if (regenerateTraits || p.traits.empty()) p.traits = generatePlayerTraits(p, p.age <= 19);
    if (p.traits.empty()) p.traits = generatePlayerTraits(p, p.age <= 19);
}

int positionFitScore(const Player& p, const string& desiredPosition) {
    string desired = normalizePosition(desiredPosition);
    string primary = normalizePosition(p.position);
    if (desired == "N/A" || desired.empty()) return 60;
    if (primary == desired) return 100 + p.versatility / 8;
    if (find(p.secondaryPositions.begin(), p.secondaryPositions.end(), desired) != p.secondaryPositions.end()) {
        return 78 + p.versatility / 4;
    }
    if (primary == "MED" && (desired == "DEF" || desired == "DEL")) return 45 + p.versatility / 5;
    if (primary == "DEF" && desired == "MED") return 42 + p.versatility / 6;
    if (primary == "DEL" && desired == "MED") return 40 + p.versatility / 6;
    if (primary == "ARQ" || desired == "ARQ") return 6;
    return 18 + p.versatility / 8;
}

string playerReliabilityLabel(const Player& p) {
    int score = (p.consistency * 2 + p.professionalism) / 3;
    if (score >= 78) return "Muy fiable";
    if (score >= 62) return "Solido";
    if (score >= 48) return "Variable";
    return "Irregular";
}

string playerFormLabel(const Player& p) {
    if (p.currentForm >= 76) return "En racha";
    if (p.currentForm >= 60) return "Bien";
    if (p.currentForm >= 45) return "Normal";
    return "Baja";
}

static int divisionPrestigeBase(const string& division) {
    string id = canonicalDivisionId(division);
    if (id.empty()) id = toLower(trim(division));
    if (id == "primera division") return 68;
    if (id == "primera b") return 56;
    if (id == "segunda division") return 48;
    if (id == "tercera division a" || id == "tercera a") return 40;
    if (id == "tercera division b" || id == "tercera b") return 32;
    return 36;
}

static string inferClubStyle(const Team& team) {
    if (team.tactics == "Pressing" || team.pressingIntensity >= 4) return "Presion vertical";
    if (team.matchInstruction == "Juego directo" || team.tempo >= 4) return "Transicion directa";
    if (team.matchInstruction == "Por bandas" || team.width >= 4) return "Ataque por bandas";
    if (team.tactics == "Defensive" || team.matchInstruction == "Bloque bajo" || team.defensiveLine <= 2) {
        return "Bloque ordenado";
    }
    if (team.trainingFacilityLevel >= 3 || team.formation.find("4-3-3") != string::npos) {
        return "Control de posesion";
    }
    return "Equilibrio competitivo";
}

static string inferYouthIdentity(const Team& team) {
    if (team.youthFacilityLevel >= 4 || team.youthCoach >= 75) return "Cantera estructurada";
    if (team.youthFacilityLevel >= 3 || team.trainingFacilityLevel >= 3) return "Desarrollo mixto";
    if (team.fanBase >= 28 && team.clubPrestige >= 60) return "Plantel de mercado";
    return "Talento local";
}

static string inferPrimaryRival(const string& clubName) {
    string id = normalizeTeamId(clubName);
    static const vector<pair<string, string>> rivals = {
        {"colocolo", "Universidad de Chile"},
        {"universidaddechile", "Colo-Colo"},
        {"universidadcatolica", "Universidad de Chile"},
        {"deportesiquique", "San Marcos de Arica"},
        {"sanmarcosdearica", "Deportes Iquique"},
        {"cobreloa", "Deportes Antofagasta"},
        {"deportesantofagasta", "Cobreloa"},
        {"santiagowanderers", "Everton"},
        {"everton", "Santiago Wanderers"},
        {"unionespanola", "Palestino"},
        {"palestino", "Union Espanola"},
        {"magallanes", "Santiago Morning"},
        {"santiagomorning", "Magallanes"},
        {"rangers", "Curico Unido"},
        {"curicounido", "Rangers"},
        {"deportesconcepcion", "Fernandez Vial"},
        {"fernandezvial", "Deportes Concepcion"}
    };
    for (const auto& rival : rivals) {
        if (rival.first == id) return rival.second;
    }
    return "";
}

static string inferCoachName(const Team& team) {
    static const vector<string> firstNames = {"Marco", "Luis", "Diego", "Jorge", "Pablo", "Ruben", "Hector", "Matias"};
    static const vector<string> lastNames = {"Soto", "Mena", "Rojas", "Valdes", "Tapia", "Pizarro", "Leal", "Munoz"};
    int hash = 0;
    for (char ch : normalizeTeamId(team.name)) hash += static_cast<unsigned char>(ch);
    return firstNames[static_cast<size_t>(hash % static_cast<int>(firstNames.size()))] + " " +
           lastNames[static_cast<size_t>((hash / 3) % static_cast<int>(lastNames.size()))];
}

static string inferStaffName(const Team& team, const string& roleKey, int salt) {
    static const vector<string> firstNames = {"Cristian", "Andres", "Felipe", "Ramon", "Mauricio", "Nicolas", "Rafael", "Claudio"};
    static const vector<string> lastNames = {"Araya", "Contreras", "Caceres", "Escobar", "Maldonado", "Astudillo", "Sepulveda", "Fuentes"};
    int hash = salt;
    const string key = normalizeTeamId(team.name) + normalizeTeamId(roleKey);
    for (char ch : key) hash += static_cast<unsigned char>(ch);
    return firstNames[static_cast<size_t>(hash % static_cast<int>(firstNames.size()))] + " " +
           lastNames[static_cast<size_t>((hash / 5) % static_cast<int>(lastNames.size()))];
}

static string inferCoachStyle(const Team& team) {
    if (team.tactics == "Pressing") return "Intensidad";
    if (team.tactics == "Defensive" || team.matchInstruction == "Bloque bajo") return "Orden";
    if (team.matchInstruction == "Juego directo") return "Transicion";
    if (team.trainingFocus == "Ataque") return "Ofensivo";
    return "Equilibrado";
}

static string inferTransferPolicy(const Team& team) {
    if (team.youthFacilityLevel >= 4 || team.youthCoach >= 72) return "Cantera y valor futuro";
    if (team.debt > team.sponsorWeekly * 18) return "Vender antes de comprar";
    if (team.clubPrestige >= 70) return "Competir por titulares hechos";
    return "Mercado de oportunidades";
}

static vector<string> inferScoutingRegions(const Team& team) {
    vector<string> regions;
    auto addRegion = [&](const string& region) {
        if (region.empty()) return;
        if (find(regions.begin(), regions.end(), region) == regions.end()) regions.push_back(region);
    };
    addRegion(team.youthRegion.empty() ? string("Metropolitana") : team.youthRegion);
    if (team.scoutingChief >= 58) addRegion("Centro");
    if (team.scoutingChief >= 64) addRegion("Sur");
    if (team.scoutingChief >= 70) addRegion("Norte");
    if (team.clubPrestige >= 68 || team.scoutingChief >= 78) addRegion("Internacional");
    return regions;
}

static int calculateTeamPrestige(const Team& team) {
    long long achievementScore = static_cast<long long>(team.achievements.size()) * 2;
    long long financialPower = clampInt(static_cast<int>(team.sponsorWeekly / 60000LL), 0, 10);
    long long facilities = (team.stadiumLevel - 1) * 3 + (team.youthFacilityLevel - 1) * 3 +
                           (team.trainingFacilityLevel - 1) * 3;
    long long crowd = team.fanBase / 3;
    long long debtPenalty = clampInt(static_cast<int>(team.debt / 250000LL), 0, 14);
    int prestige = divisionPrestigeBase(team.division) + static_cast<int>(achievementScore + financialPower + facilities + crowd) -
                   static_cast<int>(debtPenalty);
    return clampInt(prestige, 20, 95);
}

int teamPrestigeScore(const Team& team) {
    return calculateTeamPrestige(team);
}

void ensureTeamIdentity(Team& team) {
    team.clubPrestige = calculateTeamPrestige(team);
    team.clubStyle = inferClubStyle(team);
    team.youthIdentity = inferYouthIdentity(team);
    if (team.primaryRival.empty()) team.primaryRival = inferPrimaryRival(team.name);
    if (team.goalkeepingCoach <= 0) team.goalkeepingCoach = max(45, team.fitnessCoach - 2);
    if (team.performanceAnalyst <= 0) team.performanceAnalyst = max(45, team.assistantCoach - 1);
    if (team.headCoachName.empty()) team.headCoachName = inferCoachName(team);
    if (team.headCoachReputation <= 0) team.headCoachReputation = clampInt(team.clubPrestige - 6 + team.assistantCoach / 8, 25, 92);
    if (team.headCoachStyle.empty()) team.headCoachStyle = inferCoachStyle(team);
    if (team.headCoachTenureWeeks < 0) team.headCoachTenureWeeks = 0;
    if (team.jobSecurity <= 0) team.jobSecurity = 58;
    if (team.transferPolicy.empty()) team.transferPolicy = inferTransferPolicy(team);
    if (team.scoutingRegions.empty()) team.scoutingRegions = inferScoutingRegions(team);
    if (team.assistantCoachName.empty()) team.assistantCoachName = inferStaffName(team, "assistant", 11);
    if (team.fitnessCoachName.empty()) team.fitnessCoachName = inferStaffName(team, "fitness", 17);
    if (team.scoutingChiefName.empty()) team.scoutingChiefName = inferStaffName(team, "scouting", 23);
    if (team.youthCoachName.empty()) team.youthCoachName = inferStaffName(team, "youth", 29);
    if (team.medicalChiefName.empty()) team.medicalChiefName = inferStaffName(team, "medical", 31);
    if (team.goalkeepingCoachName.empty()) team.goalkeepingCoachName = inferStaffName(team, "goalkeeping", 37);
    if (team.performanceAnalystName.empty()) team.performanceAnalystName = inferStaffName(team, "analyst", 41);
}

bool areRivalClubs(const Team& a, const Team& b) {
    string aName = normalizeTeamId(a.name);
    string bName = normalizeTeamId(b.name);
    if (!a.primaryRival.empty() && normalizeTeamId(a.primaryRival) == bName) return true;
    if (!b.primaryRival.empty() && normalizeTeamId(b.primaryRival) == aName) return true;
    return false;
}

string teamExpectationLabel(const Team& team) {
    int prestige = teamPrestigeScore(team);
    if (prestige >= 74) return "Pelear arriba";
    if (prestige >= 58) return "Competir por playoffs";
    if (prestige >= 44) return "Mitad de tabla";
    return "Evitar el fondo";
}

bool playerHasTrait(const Player& p, const string& trait) {
    return find(p.traits.begin(), p.traits.end(), trait) != p.traits.end();
}

string joinStringValues(const vector<string>& values, const string& separator) {
    string out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out += separator;
        out += values[i];
    }
    return out;
}

vector<string> generatePlayerTraits(const Player& p, bool youth) {
    vector<string> traits;
    auto addTrait = [&](const string& trait) {
        if (find(traits.begin(), traits.end(), trait) == traits.end()) {
            traits.push_back(trait);
        }
    };

    string pos = normalizePosition(p.position);
    if (p.leadership >= 72 || p.professionalism >= 80) addTrait("Lider");
    if (p.professionalism >= 76 && p.ambition >= 62 && traits.size() < 3) addTrait("Competidor");
    if ((p.injuryHistory >= 2 || p.stamina <= 60 || p.age >= 31) && traits.size() < 3) addTrait("Fragil");
    if ((p.ambition >= 75 && p.professionalism <= 48) && traits.size() < 3) addTrait("Caliente");
    if (p.discipline >= 78 && traits.size() < 3) addTrait("Disciplinado");
    if (p.adaptation >= 78 && traits.size() < 3) addTrait("Adaptable");
    if (p.injuryProneness >= 72 && traits.size() < 3) addTrait("Fragil");
    if (p.bigMatches >= 74 && traits.size() < 3) addTrait("Cita grande");
    if (p.consistency <= 45 && traits.size() < 3) addTrait("Irregular");
    if (p.versatility >= 76 && traits.size() < 3) addTrait("Versatil");
    if (p.preferredFoot == "Ambos" && traits.size() < 3) addTrait("Dos perfiles");
    if (pos == "MED" && p.attack >= p.defense && traits.size() < 3) addTrait("Pase riesgoso");
    if ((pos == "MED" || pos == "DEF") && p.stamina >= 72 && traits.size() < 3) addTrait("Llega al area");
    if ((pos == "DEF" || pos == "DEL") && p.stamina >= 74 && p.professionalism >= 55 && traits.size() < 3) addTrait("Presiona");
    if ((pos == "ARQ" || pos == "DEF") && p.defense >= p.attack + 8 && traits.size() < 3) addTrait("Muralla");
    if (youth && p.potential - p.skill >= 12 && traits.size() < 3) addTrait("Proyecto");
    if (traits.empty()) addTrait("Equilibrado");
    if (traits.size() > 3) traits.resize(3);
    return traits;
}

Player makeRandomPlayer(const string& position, int skillMin, int skillMax, int ageMin, int ageMax) {
    Player p;
    const string normalized = normalizePosition(position);
    const int boundedMin = min(skillMin, skillMax);
    const int boundedMax = max(skillMin, skillMax);
    const int profileRoll = randInt(1, 100);
    const bool highCeiling = randInt(1, 100) <= 18;
    p.name = "Jugador" + to_string(randInt(1000, 9999));
    p.position = normalized;
    p.skill = randInt(boundedMin, boundedMax);
    p.age = randInt(ageMin, ageMax);
    const int potentialFloor = p.age <= 18 ? 6 : (p.age <= 21 ? 4 : 1);
    int potentialTop = p.age <= 18 ? 16 : (p.age <= 21 ? 12 : 7);
    if (highCeiling) potentialTop += 5;
    p.potential = clampInt(p.skill + randInt(potentialFloor, potentialTop), p.skill, 97);

    if (normalized == "ARQ") {
        const bool sweeperKeeper = profileRoll >= 78;
        p.attack = clampInt(p.skill - 34 + (sweeperKeeper ? 6 : 0) + randInt(-3, 3), 8, 74);
        p.defense = clampInt(p.skill + 12 + (sweeperKeeper ? 2 : 4) + randInt(-3, 4), 30, 99);
        p.stamina = clampInt(p.skill - 3 + randInt(-6, 4), 28, 92);
        p.setPieceSkill = clampInt(p.skill - 12 + randInt(-5, 6), 15, 80);
        p.role = sweeperKeeper ? "SweeperKeeper" : "Tradicional";
        p.developmentPlan = "Reflejos";
    } else if (normalized == "DEF") {
        const bool wingBack = profileRoll >= 68;
        p.attack = clampInt(p.skill - 12 + (wingBack ? 10 : 1) + randInt(-4, 4), 20, 92);
        p.defense = clampInt(p.skill + 10 + (wingBack ? -2 : 5) + randInt(-4, 4), 30, 99);
        p.stamina = clampInt(p.skill + (wingBack ? 8 : 2) + randInt(-5, 5), 35, 99);
        p.setPieceSkill = clampInt(p.skill - 4 + randInt(-6, 6), 20, 92);
        p.role = wingBack ? "Carrilero" : (profileRoll <= 24 ? "Stopper" : "BallPlaying");
        p.developmentPlan = wingBack ? "Fisico" : "Defensa";
    } else if (normalized == "MED") {
        const bool creator = profileRoll <= 38;
        const bool holder = profileRoll >= 76;
        p.attack = clampInt(p.skill + (creator ? 8 : holder ? -4 : 2) + randInt(-5, 5), 25, 99);
        p.defense = clampInt(p.skill + (creator ? -2 : holder ? 8 : 2) + randInt(-5, 5), 25, 99);
        p.stamina = clampInt(p.skill + (holder ? 4 : 2) + randInt(-6, 6), 35, 99);
        p.setPieceSkill = clampInt(p.skill + (creator ? 10 : 2) + randInt(-6, 8), 25, 99);
        p.role = creator ? "Organizador" : (holder ? "Pivote" : "BoxToBox");
        p.developmentPlan = creator ? "Creatividad" : (holder ? "Defensa" : "Fisico");
    } else {
        const bool pressingForward = profileRoll <= 26;
        const bool targetMan = profileRoll >= 78;
        p.attack = clampInt(p.skill + 12 + (pressingForward ? 1 : targetMan ? 3 : 6) + randInt(-4, 5), 30, 99);
        p.defense = clampInt(p.skill - 14 + (pressingForward ? 8 : targetMan ? 2 : 0) + randInt(-4, 4), 16, 88);
        p.stamina = clampInt(p.skill + (pressingForward ? 7 : targetMan ? 2 : 0) + randInt(-6, 5), 35, 99);
        p.setPieceSkill = clampInt(p.skill - 1 + randInt(-6, 8), 20, 96);
        p.role = pressingForward ? "Pressing" : (targetMan ? "Objetivo" : "Poacher");
        p.developmentPlan = pressingForward ? "Fisico" : "Finalizacion";
    }

    p.fitness = p.stamina;
    p.value = max(20000LL,
                  static_cast<long long>(p.skill) * 9000LL +
                      static_cast<long long>(max(0, p.potential - p.skill)) * 7000LL -
                      static_cast<long long>(max(0, p.age - 28)) * 4000LL);
    p.wage = max(1500LL,
                 static_cast<long long>(p.skill) * 140 +
                     static_cast<long long>(max(0, p.potential - p.skill)) * 30 +
                     randInt(0, 900));
    p.releaseClause = max(50000LL, p.value * (18 + randInt(0, 8)) / 10);
    p.leadership = clampInt(28 + p.age + randInt(-10, 18), 1, 99);
    p.professionalism = clampInt(34 + p.age + randInt(-8, 18) + (highCeiling ? 4 : 0), 1, 99);
    p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    p.consistency = clampInt(28 + p.age + p.professionalism / 6 + randInt(-10, 12), 1, 99);
    p.bigMatches = clampInt(24 + p.ambition / 3 + p.age / 2 + randInt(-10, 10), 1, 99);
    p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    p.currentForm = clampInt(46 + p.skill / 5 + randInt(-8, 8), 1, 99);
    p.tacticalDiscipline = clampInt(34 + p.professionalism / 2 + randInt(-8, 8), 1, 99);
    p.versatility = clampInt(24 + p.stamina / 3 + (normalized == "MED" ? 10 : 0) + randInt(-8, 10), 1, 99);
    p.discipline = clampInt((p.tacticalDiscipline * 2 + p.professionalism + p.consistency) / 4 + randInt(-3, 4), 1, 99);
    p.injuryProneness = clampInt(12 + max(0, 66 - p.stamina) / 2 + max(0, p.age - 30) * 2 + randInt(-3, 5), 1, 99);
    p.adaptation = clampInt(34 + p.versatility / 3 + p.professionalism / 4 + p.ambition / 8 + randInt(-4, 6), 1, 99);
    p.moraleMomentum = randInt(-2, 4);
    p.fatigueLoad = randInt(0, 8);
    p.unhappinessWeeks = 0;
    p.desiredStarts = (p.skill >= boundedMax - 4) ? 3 : (p.skill >= boundedMax - 8 ? 2 : 1);
    p.startsThisSeason = 0;
    p.wantsToLeave = false;
    p.onLoan = false;
    p.parentClub.clear();
    p.loanWeeksRemaining = 0;
    p.contractWeeks = randInt(52, 156);
    p.injured = false;
    p.injuryType = "";
    p.injuryWeeks = 0;
    p.injuryHistory = 0;
    p.yellowAccumulation = 0;
    p.seasonYellowCards = 0;
    p.seasonRedCards = 0;
    p.matchesSuspended = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    p.lastTrainedSeason = -1;
    p.lastTrainedWeek = -1;
    p.roleDuty = defaultDutyForRole(p.role, normalized);
    p.promisedPosition = normalized;
    ensurePlayerProfile(p, true);
    return p;
}

Team::Team(string n)
    : name(std::move(n)),
      division(""),
      tactics("Balanced"),
      formation("4-4-2"),
      budget(0),
      morale(50),
      trainingFocus("Balanceado"),
      points(0),
      goalsFor(0),
      goalsAgainst(0),
      awayGoals(0),
      wins(0),
      draws(0),
      losses(0),
      yellowCards(0),
      redCards(0),
      tiebreakerSeed(0),
      pressingIntensity(3),
      defensiveLine(3),
      tempo(3),
      width(3),
      markingStyle("Zonal"),
      preferredBench(),
      captain(""),
      penaltyTaker(""),
      freeKickTaker(""),
      cornerTaker(""),
      rotationPolicy("Balanceado"),
      assistantCoach(55),
      fitnessCoach(55),
      scoutingChief(55),
      youthCoach(55),
      medicalTeam(55),
      goalkeepingCoach(55),
      performanceAnalyst(55),
      assistantCoachName(""),
      fitnessCoachName(""),
      scoutingChiefName(""),
      youthCoachName(""),
      medicalChiefName(""),
      goalkeepingCoachName(""),
      performanceAnalystName(""),
      youthRegion("Metropolitana"),
      debt(0),
      sponsorWeekly(25000),
      stadiumLevel(1),
      youthFacilityLevel(1),
      trainingFacilityLevel(1),
      fanBase(12),
      clubPrestige(0),
      clubStyle(""),
      youthIdentity(""),
      primaryRival(""),
      matchInstruction("Equilibrado"),
      headCoachName(""),
      headCoachReputation(50),
      headCoachStyle(""),
      headCoachTenureWeeks(0),
      jobSecurity(58),
      transferPolicy(""),
      scoutingRegions() {}

void Team::addPlayer(const Player& p) {
    players.push_back(p);
}

void Team::addHeadToHeadPoints(const string& opponent, int pointsToAdd) {
    for (auto& record : headToHead) {
        if (record.opponent == opponent) {
            record.points += pointsToAdd;
            return;
        }
    }
    headToHead.push_back({opponent, pointsToAdd});
}

int Team::headToHeadPointsAgainst(const string& opponent) const {
    for (const auto& record : headToHead) {
        if (record.opponent == opponent) return record.points;
    }
    return 0;
}

void Team::resetSeasonStats() {
    points = 0;
    goalsFor = 0;
    goalsAgainst = 0;
    awayGoals = 0;
    wins = 0;
    draws = 0;
    losses = 0;
    yellowCards = 0;
    redCards = 0;
    tiebreakerSeed = randInt(0, 1000000);
    headToHead.clear();
}

const vector<DivisionInfo> kDivisions = listRegisteredDivisions();
