#include "app_services.h"

#include "career/career_support.h"
#include "competition.h"
#include "transfers/negotiation_system.h"
#include "ui.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <streambuf>

using namespace std;

namespace {

struct StdoutCapture {
    streambuf* original = nullptr;
    ostringstream buffer;

    StdoutCapture() : original(cout.rdbuf(buffer.rdbuf())) {}
    ~StdoutCapture() {
        cout.rdbuf(original);
    }

    string str() const {
        return buffer.str();
    }
};

vector<string> splitOutputLines(const string& text) {
    vector<string> lines;
    istringstream stream(text);
    string line;
    while (getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

vector<string>* g_collectedMessages = nullptr;

void collectUiMessage(const string& message) {
    if (g_collectedMessages && !message.empty()) g_collectedMessages->push_back(message);
}

IncomingOfferDecision autoOfferDecision(const Career& career,
                                        const Player& player,
                                        long long offer,
                                        long long maxOffer) {
    IncomingOfferDecision decision;
    size_t squadSize = career.myTeam ? career.myTeam->players.size() : 0;
    if (player.wantsToLeave && offer >= player.value) {
        decision.action = 1;
        return decision;
    }
    if (squadSize > 20 && offer >= max(player.value * 12 / 10, maxOffer * 90 / 100)) {
        decision.action = 1;
        return decision;
    }
    decision.action = (offer >= maxOffer) ? 1 : 3;
    return decision;
}

bool autoRenewDecision(const Career&,
                       const Team& team,
                       const Player& player,
                       long long demandedWage,
                       int,
                       long long) {
    if (team.budget < demandedWage * 6) return false;
    if (player.wantsToLeave && player.happiness < 45) return false;
    return player.skill >= team.getAverageSkill() - 5 || team.players.size() <= 18;
}

int autoManagerJobDecision(const Career&, const vector<Team*>& jobs) {
    return jobs.empty() ? -1 : 0;
}

ServiceResult finalizeCapturedResult(bool ok, const vector<string>& messages, const string& capturedStdout) {
    ServiceResult result;
    result.ok = ok;
    result.messages = messages;
    vector<string> legacyLines = splitOutputLines(capturedStdout);
    result.messages.insert(result.messages.end(), legacyLines.begin(), legacyLines.end());
    return result;
}

string formatMoney(long long value) {
    bool negative = value < 0;
    unsigned long long absValue = static_cast<unsigned long long>(negative ? -value : value);
    string digits = to_string(absValue);
    string out;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return string(negative ? "-$" : "$") + out;
}

string nextDevelopmentPlan(const string& current) {
    static const vector<string> plans = {"Equilibrado", "Fisico", "Defensa", "Creatividad", "Finalizacion", "Liderazgo"};
    for (size_t i = 0; i < plans.size(); ++i) {
        if (plans[i] == current) return plans[(i + 1) % plans.size()];
    }
    return plans.front();
}

string nextMatchInstruction(const string& current) {
    static const vector<string> instructions = {"Equilibrado", "Laterales altos", "Bloque bajo", "Balon parado",
                                                "Presion final", "Por bandas", "Juego directo",
                                                "Contra-presion", "Pausar juego"};
    for (size_t i = 0; i < instructions.size(); ++i) {
        if (instructions[i] == current) return instructions[(i + 1) % instructions.size()];
    }
    return instructions.front();
}

string opponentSummary(const Career& career) {
    const Team* opponent = nextOpponent(career);
    if (!career.myTeam || !opponent) return "Sin informe rival disponible.";
    int avgFitness = 0;
    int injured = 0;
    int defenders = 0;
    int forwards = 0;
    for (const auto& player : opponent->players) {
        avgFitness += player.fitness;
        if (player.injured) injured++;
        string pos = normalizePosition(player.position);
        if (pos == "DEF") defenders++;
        if (pos == "DEL") forwards++;
    }
    if (!opponent->players.empty()) avgFitness /= static_cast<int>(opponent->players.size());
    string style = opponent->clubStyle.empty()
                       ? (opponent->tactics == "Pressing" ? "Presion vertical"
                          : opponent->tactics == "Defensive" ? "Bloque ordenado"
                                                             : "Equilibrio competitivo")
                       : opponent->clubStyle;
    string vulnerability = (avgFitness < 62) ? "carga fisica baja"
                          : (opponent->defensiveLine >= 4 ? "espacio a la espalda"
                             : (opponent->width >= 4 ? "espacios por dentro" : "ritmo controlado"));
    return opponent->name + " | estilo " + style +
           " | prestigio " + to_string(teamPrestigeScore(*opponent)) +
           " | lesionados " + to_string(injured) +
           " | fitness medio " + to_string(avgFitness) +
           " | formacion " + opponent->formation +
           " | fragilidad sugerida: " + vulnerability +
           (areRivalClubs(*career.myTeam, *opponent) ? " | clasico" : "");
}

int playerIndexByName(const Team& team, const string& name) {
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

void eraseNamedSelection(vector<string>& values, const string& name) {
    values.erase(remove(values.begin(), values.end(), name), values.end());
}

void detachPlayerFromSelections(Team& team, const string& playerName) {
    eraseNamedSelection(team.preferredXI, playerName);
    eraseNamedSelection(team.preferredBench, playerName);
    if (team.captain == playerName) team.captain.clear();
    if (team.penaltyTaker == playerName) team.penaltyTaker.clear();
    if (team.freeKickTaker == playerName) team.freeKickTaker.clear();
    if (team.cornerTaker == playerName) team.cornerTaker.clear();
}

void applyDepartureShock(Team& team, const Player& player) {
    if (player.skill < team.getAverageSkill()) return;
    team.morale = clampInt(team.morale - 3, 0, 100);
    for (auto& mate : team.players) {
        if (mate.name == player.name) continue;
        mate.chemistry = clampInt(mate.chemistry - 1, 1, 99);
        if (player.leadership >= 70) mate.happiness = clampInt(mate.happiness - 1, 1, 99);
    }
}

LeagueTable buildTableFromTeams(const vector<Team*>& teams, const string& title, const string& ruleId) {
    LeagueTable table;
    table.title = title;
    table.ruleId = ruleId;
    for (Team* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

int groupForTeam(const Career& career, const Team* team) {
    for (int index : career.groupNorthIdx) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size()) &&
            career.activeTeams[static_cast<size_t>(index)] == team) {
            return 0;
        }
    }
    for (int index : career.groupSouthIdx) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size()) &&
            career.activeTeams[static_cast<size_t>(index)] == team) {
            return 1;
        }
    }
    return -1;
}

LeagueTable relevantCompetitionTable(const Career& career) {
    if (!career.myTeam) return career.leagueTable;
    if (!career.usesGroupFormat() || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }
    vector<Team*> teams;
    const vector<int>* indices = nullptr;
    int group = groupForTeam(career, career.myTeam);
    if (group == 0) indices = &career.groupNorthIdx;
    else if (group == 1) indices = &career.groupSouthIdx;
    if (!indices) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }
    for (int index : *indices) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size())) {
            teams.push_back(career.activeTeams[static_cast<size_t>(index)]);
        }
    }
    string title = competitionGroupTitle(career.activeDivision, group == 0);
    return buildTableFromTeams(teams, title, career.activeDivision);
}

long long weeklyWage(const Team& team) {
    long long wages = 0;
    for (const auto& player : team.players) wages += player.wage;
    return wages * getCompetitionConfig(team.division).wageFactor / 100;
}

string detectScoutingNeed(const Team& team) {
    vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    string best = "MED";
    int bestScore = 1000000;
    for (const auto& pos : positions) {
        int count = 0;
        int totalSkill = 0;
        for (const auto& player : team.players) {
            if (normalizePosition(player.position) != pos) continue;
            count++;
            totalSkill += player.skill;
        }
        int average = count > 0 ? totalSkill / count : 0;
        int score = count * 20 + average;
        if (score < bestScore) {
            bestScore = score;
            best = pos;
        }
    }
    return best;
}

long long upgradeCost(const Team& team, ClubUpgrade upgrade) {
    switch (upgrade) {
        case ClubUpgrade::Stadium: return 60000LL * (team.stadiumLevel + 1);
        case ClubUpgrade::Youth: return 50000LL * (team.youthFacilityLevel + 1);
        case ClubUpgrade::Training: return 55000LL * (team.trainingFacilityLevel + 1);
        case ClubUpgrade::Scouting: return 36000LL + static_cast<long long>(team.scoutingChief) * 1300LL;
        case ClubUpgrade::Medical: return 33000LL + static_cast<long long>(team.medicalTeam) * 1200LL;
    }
    return 0;
}

string upgradeLabel(ClubUpgrade upgrade) {
    switch (upgrade) {
        case ClubUpgrade::Stadium: return "estadio";
        case ClubUpgrade::Youth: return "cantera";
        case ClubUpgrade::Training: return "entrenamiento";
        case ClubUpgrade::Scouting: return "scouting";
        case ClubUpgrade::Medical: return "medico";
    }
    return "club";
}

ServiceResult failure(const string& message) {
    ServiceResult result;
    result.ok = false;
    result.messages.push_back(message);
    return result;
}

}  // namespace

ServiceResult startCareerService(Career& career,
                                 const string& divisionId,
                                 const string& teamName,
                                 const string& managerName) {
    ServiceResult result;
    career.initializeLeague(true);
    if (career.divisions.empty()) {
        result.messages.push_back("No se encontraron divisiones disponibles.");
        return result;
    }
    career.setActiveDivision(divisionId);
    if (career.activeTeams.empty()) {
        result.messages.push_back("La division seleccionada no tiene equipos.");
        return result;
    }
    Team* selectedTeam = career.activeTeams.front();
    for (Team* team : career.activeTeams) {
        if (team && team->name == teamName) {
            selectedTeam = team;
            break;
        }
    }
    career.myTeam = selectedTeam;
    career.managerName = managerName.empty() ? "Manager" : managerName;
    career.managerReputation = 50;
    career.newsFeed.clear();
    career.scoutInbox.clear();
    career.scoutingShortlist.clear();
    career.history.clear();
    career.pendingTransfers.clear();
    career.achievements.clear();
    career.currentSeason = 1;
    career.currentWeek = 1;
    career.resetSeason();
    result.ok = true;
    result.messages.push_back("Nueva carrera iniciada con " + career.myTeam->name + ".");
    result.messages.insert(result.messages.end(), career.loadWarnings.begin(), career.loadWarnings.end());
    return result;
}

ServiceResult loadCareerService(Career& career) {
    ServiceResult result;
    career.initializeLeague(true);
    result.ok = career.loadCareer();
    result.messages.push_back(result.ok
                                  ? "Carrera cargada: " + (career.myTeam ? career.myTeam->name : string("Sin club")) + "."
                                  : "No se encontro una carrera guardada.");
    result.messages.insert(result.messages.end(), career.loadWarnings.begin(), career.loadWarnings.end());
    return result;
}

ServiceResult saveCareerService(Career& career) {
    ServiceResult result;
    result.ok = career.saveCareer();
    result.messages.push_back(result.ok
                                  ? "Carrera guardada en " + career.saveFile + "."
                                  : "No se pudo guardar la carrera en " + career.saveFile + ".");
    return result;
}

ServiceResult simulateCareerWeekService(Career& career) {
    vector<string> messages;
    g_collectedMessages = &messages;
    setUiMessageCallback(collectUiMessage);
    setIncomingOfferDecisionCallback(autoOfferDecision);
    setContractRenewalDecisionCallback(autoRenewDecision);
    setManagerJobSelectionCallback(autoManagerJobDecision);
    StdoutCapture capture;
    simulateCareerWeek(career);
    setManagerJobSelectionCallback(nullptr);
    setContractRenewalDecisionCallback(nullptr);
    setIncomingOfferDecisionCallback(nullptr);
    setUiMessageCallback(nullptr);
    g_collectedMessages = nullptr;
    ServiceResult result = finalizeCapturedResult(true, messages, capture.str());
    if (result.messages.empty()) result.messages.push_back("Semana simulada.");
    result.ok = true;
    return result;
}

ServiceResult scoutPlayersService(Career& career, const string& region, const string& focusPos) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    long long scoutCost = max(3000LL, 9000LL - team.scoutingChief * 50LL);
    if (team.budget < scoutCost) return failure("Presupuesto insuficiente para ojeo.");
    string resolvedRegion = region.empty() ? "Todas" : region;
    string resolvedPos = normalizePosition(focusPos);
    if (resolvedPos == "N/A" || resolvedPos.empty()) resolvedPos = detectScoutingNeed(team);
    vector<pair<Team*, int>> reports;
    for (auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (resolvedRegion != "Todas" && club.youthRegion != resolvedRegion) continue;
        for (size_t i = 0; i < club.players.size(); ++i) {
            const Player& player = club.players[i];
            if (player.onLoan) continue;
            if (positionFitScore(player, resolvedPos) < 70) continue;
            reports.push_back({&club, static_cast<int>(i)});
        }
    }
    if (reports.empty()) return failure("No se encontraron jugadores para ese informe.");
    sort(reports.begin(), reports.end(), [&](const auto& left, const auto& right) {
        const Player& a = left.first->players[static_cast<size_t>(left.second)];
        const Player& b = right.first->players[static_cast<size_t>(right.second)];
        int fitA = a.potential + a.professionalism / 2 + a.currentForm / 2 + positionFitScore(a, resolvedPos);
        int fitB = b.potential + b.professionalism / 2 + b.currentForm / 2 + positionFitScore(b, resolvedPos);
        if (fitA != fitB) return fitA > fitB;
        return a.skill > b.skill;
    });
    if (reports.size() > 5) reports.resize(5);
    int error = clampInt(14 - team.scoutingChief / 10, 2, 10);
    team.budget -= scoutCost;
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Scouting completado en " + resolvedRegion + " con foco " + resolvedPos +
                              ". Costo " + formatMoney(scoutCost) + ".");
    for (const auto& report : reports) {
        Team* club = report.first;
        const Player& player = club->players[static_cast<size_t>(report.second)];
        int estSkillLo = clampInt(player.skill - error, 1, 99);
        int estSkillHi = clampInt(player.skill + error, 1, 99);
        int estPotLo = clampInt(player.potential - error, player.skill, 99);
        int estPotHi = clampInt(player.potential + error, player.skill, 99);
        string note = player.name + " | " + club->name + " | " + club->youthRegion + " | Hab " +
                      to_string(estSkillLo) + "-" + to_string(estSkillHi) + " | Pot " +
                      to_string(estPotLo) + "-" + to_string(estPotHi) +
                      " | Pie " + player.preferredFoot +
                      " | Sec " + (player.secondaryPositions.empty() ? string("-") : joinStringValues(player.secondaryPositions, "/")) +
                      " | Forma " + playerFormLabel(player) +
                      " | Fiabilidad " + playerReliabilityLabel(player) +
                      " | Rasgos " + joinStringValues(player.traits, ", ") +
                      " | Perfil " + personalityLabel(player);
        result.messages.push_back("- " + note);
        career.scoutInbox.push_back(note);
    }
    if (career.scoutInbox.size() > 40) {
        career.scoutInbox.erase(career.scoutInbox.begin(),
                                career.scoutInbox.begin() + static_cast<long long>(career.scoutInbox.size() - 40));
    }
    career.addNews("El scouting completa un informe en la region " + resolvedRegion + " para " + team.name + ".");
    return result;
}

ServiceResult upgradeClubService(Career& career, ClubUpgrade upgrade) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    ensureTeamIdentity(team);
    long long cost = upgradeCost(team, upgrade);
    if (team.budget < cost) return failure("Presupuesto insuficiente para mejorar " + upgradeLabel(upgrade) + ".");
    team.budget -= cost;
    string message;
    switch (upgrade) {
        case ClubUpgrade::Stadium:
            team.stadiumLevel++;
            team.fanBase += 3;
            team.sponsorWeekly += 5000;
            message = team.name + " amplia su estadio.";
            break;
        case ClubUpgrade::Youth:
            team.youthFacilityLevel++;
            message = team.name + " mejora su cantera.";
            break;
        case ClubUpgrade::Training:
            team.trainingFacilityLevel++;
            message = team.name + " mejora su centro de entrenamiento.";
            break;
        case ClubUpgrade::Scouting:
            team.scoutingChief = clampInt(team.scoutingChief + 5, 1, 99);
            message = team.name + " fortalece su red de scouting.";
            break;
        case ClubUpgrade::Medical:
            team.medicalTeam = clampInt(team.medicalTeam + 5, 1, 99);
            message = team.name + " fortalece el cuerpo medico.";
            break;
    }
    ensureTeamIdentity(team);
    career.addNews(message);
    ServiceResult result;
    result.ok = true;
    result.messages.push_back(message + " Inversion " + formatMoney(cost) + ".");
    return result;
}

ServiceResult buyTransferTargetService(Career& career,
                                       const string& sellerTeamName,
                                       const string& playerName,
                                       NegotiationProfile profile,
                                       NegotiationPromise promise) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* seller = career.findTeamByName(sellerTeamName);
    if (!seller || seller == career.myTeam) return failure("No se encontro el club vendedor.");
    int sellerIdx = playerIndexByName(*seller, playerName);
    if (sellerIdx < 0) return failure("No se encontro el jugador seleccionado.");
    int maxSquadSize = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquadSize > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquadSize) {
        return failure("Tu plantel ya alcanzo el maximo permitido para la division.");
    }
    if (seller->players.size() <= 18) return failure("El club vendedor no libera jugadores con un plantel tan corto.");
    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*seller);
    Player player = seller->players[static_cast<size_t>(sellerIdx)];
    if (player.onLoan) return failure("El jugador esta a prestamo y no esta disponible.");
    string rejectionReason;
    if (playerRejectsMove(career, *career.myTeam, *seller, player, promise, rejectionReason)) {
        return failure(rejectionReason);
    }
    long long transferFee = max(player.value, player.releaseClause * 65 / 100);
    transferFee = static_cast<long long>(transferFee * negotiationFeeFactor(profile));
    long long rivalryExtra = rivalrySurcharge(*career.myTeam, *seller, transferFee);
    transferFee += rivalryExtra;
    int difficulty = agentDifficulty(player);
    long long agentFee = estimatedAgentFee(player, transferFee);
    agentFee = agentFee * (100 + difficulty / 3) / 100;
    if (career.myTeam->budget < transferFee + agentFee) {
        return failure("Presupuesto insuficiente para cubrir transferencia y honorarios.");
    }
    player.wage = max(player.wage, static_cast<long long>(wageDemandFor(player) *
                                                          negotiationWageFactor(profile) *
                                                          promiseWageFactor(promise)));
    player.wage = player.wage * (100 + difficulty / 12) / 100;
    player.contractWeeks = promiseContractWeeks(promise, player.contractWeeks);
    player.releaseClause = max(static_cast<long long>(player.value * negotiationClauseFactor(profile)),
                               static_cast<long long>((transferFee + agentFee) * negotiationClauseFactor(profile)));
    player.onLoan = false;
    player.parentClub.clear();
    player.loanWeeksRemaining = 0;
    player.wantsToLeave = false;
    player.happiness = clampInt(player.happiness + 4, 1, 99);
    applyNegotiatedPromise(player, promise);
    career.myTeam->budget -= (transferFee + agentFee);
    career.myTeam->addPlayer(player);
    seller->budget += transferFee;
    detachPlayerFromSelections(*seller, player.name);
    eraseNamedSelection(career.scoutingShortlist, seller->name + "|" + player.name);
    seller->players.erase(seller->players.begin() + sellerIdx);
    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*seller);
    career.addNews(player.name + " firma con " + career.myTeam->name + " desde " + seller->name + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Fichaje completado: " + player.name + " llega desde " + seller->name +
                              " por " + formatMoney(transferFee) + " + honorarios " + formatMoney(agentFee) +
                              " | perfil " + negotiationLabel(profile) +
                              " | promesa " + promiseLabel(promise) +
                              (rivalryExtra > 0 ? " | sobreprecio por rivalidad " + formatMoney(rivalryExtra) : "") +
                              " | agente " + to_string(difficulty) + "/90.");
    return result;
}

ServiceResult signPreContractService(Career& career,
                                     const string& sellerTeamName,
                                     const string& playerName,
                                     NegotiationProfile profile,
                                     NegotiationPromise promise) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* seller = career.findTeamByName(sellerTeamName);
    if (!seller || seller == career.myTeam) return failure("No se encontro el club del jugador.");
    int sellerIdx = playerIndexByName(*seller, playerName);
    if (sellerIdx < 0) return failure("No se encontro el jugador seleccionado.");
    const Player& player = seller->players[static_cast<size_t>(sellerIdx)];
    if (player.onLoan) return failure("No se puede firmar precontrato con un jugador cedido.");
    if (player.contractWeeks > 12) return failure("El jugador aun no es elegible para precontrato.");
    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*seller);
    string rejectionReason;
    if (playerRejectsMove(career, *career.myTeam, *seller, player, promise, rejectionReason)) {
        return failure(rejectionReason);
    }
    for (const auto& move : career.pendingTransfers) {
        if (move.playerName == player.name && move.toTeam == career.myTeam->name && move.preContract) {
            return failure("Ese jugador ya tiene un precontrato pendiente con tu club.");
        }
    }
    int difficulty = agentDifficulty(player);
    long long signingBonus = max(20000LL, static_cast<long long>(wageDemandFor(player) *
                                                                 (profile == NegotiationProfile::Safe ? 5 : 4) *
                                                                 promiseWageFactor(promise)));
    signingBonus = signingBonus * (100 + difficulty / 4) / 100;
    if (career.myTeam->budget < signingBonus) return failure("Presupuesto insuficiente para el bono de firma.");
    career.myTeam->budget -= signingBonus;
    eraseNamedSelection(career.scoutingShortlist, seller->name + "|" + player.name);
    career.pendingTransfers.push_back({player.name, seller->name, career.myTeam->name, career.currentSeason + 1, 0, 0,
                                       max(player.wage, static_cast<long long>(wageDemandFor(player) *
                                                                              negotiationWageFactor(profile) *
                                                                              promiseWageFactor(promise))) *
                                           (100 + difficulty / 12) / 100,
                                       promiseContractWeeks(promise, profile == NegotiationProfile::Aggressive ? 78 : 104),
                                       true,
                                       false,
                                       promiseLabel(promise)});
    career.addNews(player.name + " firma un precontrato con " + career.myTeam->name + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Precontrato firmado para la temporada siguiente. Bono " + formatMoney(signingBonus) +
                              " | perfil " + negotiationLabel(profile) +
                              " | promesa " + promiseLabel(promise) +
                              " | agente " + to_string(difficulty) + "/90.");
    return result;
}

ServiceResult renewPlayerContractService(Career& career,
                                         const string& playerName,
                                         NegotiationProfile profile,
                                         NegotiationPromise promise) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    int index = playerIndexByName(team, playerName);
    if (index < 0) return failure("No se encontro el jugador seleccionado.");
    Player& player = team.players[static_cast<size_t>(index)];
    ensureTeamIdentity(team);
    if (renewalNeedsStrongerPromise(player, promise, career.currentWeek)) {
        return failure(player.name + " exige una promesa contractual acorde a su rol actual antes de renovar.");
    }
    int difficulty = agentDifficulty(player);
    long long demandedWage = max(wageDemandFor(player), player.wage + max(5000LL, player.wage / 10));
    if (player.wantsToLeave) demandedWage = demandedWage * 115 / 100;
    if (promiseAtRisk(player, career.currentWeek)) demandedWage = demandedWage * 108 / 100;
    demandedWage = max(player.wage, static_cast<long long>(demandedWage *
                                                           negotiationWageFactor(profile) *
                                                           promiseWageFactor(promise)));
    demandedWage = demandedWage * (100 + difficulty / 14) / 100;
    int demandedWeeks = promiseContractWeeks(promise, max(profile == NegotiationProfile::Aggressive ? 78 : 104,
                                                          player.contractWeeks + 52));
    long long demandedClause = max(static_cast<long long>(player.value * negotiationClauseFactor(profile)),
                                   demandedWage * (profile == NegotiationProfile::Safe ? 48 : 40));
    if (team.budget < demandedWage * 6) return failure("Presupuesto insuficiente para renovar a " + player.name + ".");
    player.wage = demandedWage;
    player.contractWeeks = demandedWeeks;
    player.releaseClause = demandedClause;
    player.wantsToLeave = false;
    player.happiness = clampInt(player.happiness + 6, 1, 99);
    applyNegotiatedPromise(player, promise);
    ensureTeamIdentity(team);
    career.addNews(player.name + " renueva con " + team.name + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Contrato renovado: " + player.name + " | salario " + formatMoney(demandedWage) +
                              " | contrato " + to_string(demandedWeeks) + " sem | perfil " +
                              negotiationLabel(profile) + " | promesa " + promiseLabel(promise) +
                              " | agente " + to_string(difficulty) + "/90.");
    return result;
}

ServiceResult sellPlayerService(Career& career, const string& playerName) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    ensureTeamIdentity(team);
    if (team.players.size() <= 18) return failure("Debes mantener al menos 18 jugadores en plantel.");
    int index = playerIndexByName(team, playerName);
    if (index < 0) return failure("No se encontro el jugador seleccionado.");
    const Player& player = team.players[static_cast<size_t>(index)];
    if (player.onLoan && !player.parentClub.empty()) return failure("No puedes vender un jugador cedido.");
    long long transferFee = max(10000LL, player.value * 105 / 100);
    team.budget += transferFee;
    detachPlayerFromSelections(team, player.name);
    applyDepartureShock(team, player);
    career.addNews(player.name + " sale de " + team.name + " por " + formatMoney(transferFee) + ".");
    team.players.erase(team.players.begin() + index);
    ensureTeamIdentity(team);
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Venta completada: " + player.name + " deja el club por " + formatMoney(transferFee) + ".");
    return result;
}

ServiceResult cyclePlayerDevelopmentPlanService(Career& career, const string& playerName) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    int index = playerIndexByName(team, playerName);
    if (index < 0) return failure("No se encontro el jugador seleccionado.");
    Player& player = team.players[static_cast<size_t>(index)];
    player.developmentPlan = nextDevelopmentPlan(player.developmentPlan);
    career.addNews("Plan individual actualizado para " + player.name + ": " + player.developmentPlan + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back(player.name + " cambia su plan a " + player.developmentPlan + ".");
    return result;
}

ServiceResult cycleMatchInstructionService(Career& career) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    team.matchInstruction = nextMatchInstruction(team.matchInstruction);
    ensureTeamIdentity(team);
    career.addNews("Nueva instruccion de partido en " + team.name + ": " + team.matchInstruction + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Instruccion de partido actual: " + team.matchInstruction + ".");
    return result;
}

ServiceResult shortlistPlayerService(Career& career,
                                     const string& sellerTeamName,
                                     const string& playerName) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* seller = career.findTeamByName(sellerTeamName);
    if (!seller || seller == career.myTeam) return failure("No se encontro el club del jugador.");
    int sellerIdx = playerIndexByName(*seller, playerName);
    if (sellerIdx < 0) return failure("No se encontro el jugador seleccionado.");
    const Player& player = seller->players[static_cast<size_t>(sellerIdx)];

    string entry = seller->name + "|" + player.name;
    if (find(career.scoutingShortlist.begin(), career.scoutingShortlist.end(), entry) != career.scoutingShortlist.end()) {
        return failure("Ese jugador ya esta en la shortlist.");
    }
    career.scoutingShortlist.push_back(entry);
    if (career.scoutingShortlist.size() > 25) {
        career.scoutingShortlist.erase(career.scoutingShortlist.begin(),
                                       career.scoutingShortlist.begin() +
                                           static_cast<long long>(career.scoutingShortlist.size() - 25));
    }
    career.addNews("Scouting agrega a " + player.name + " (" + seller->name + ") a la shortlist.");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back(player.name + " agregado a la shortlist.");
    return result;
}

ServiceResult followShortlistService(Career& career) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    if (career.scoutingShortlist.empty()) return failure("No hay jugadores en la shortlist.");

    Team& team = *career.myTeam;
    long long cost = max(4000LL, 14000LL - team.scoutingChief * 60LL);
    if (team.budget < cost) return failure("Presupuesto insuficiente para seguimiento.");
    team.budget -= cost;

    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Seguimiento de shortlist completado. Costo " + formatMoney(cost) + ".");
    int error = clampInt(8 - team.scoutingChief / 18, 1, 5);

    for (const auto& item : career.scoutingShortlist) {
        auto fields = splitByDelimiter(item, '|');
        if (fields.size() < 2) continue;
        Team* seller = career.findTeamByName(fields[0]);
        if (!seller) continue;
        int sellerIdx = playerIndexByName(*seller, fields[1]);
        if (sellerIdx < 0) continue;
        const Player& player = seller->players[static_cast<size_t>(sellerIdx)];
        string note = "[Seguimiento] " + player.name + " | " + seller->name +
                      " | Hab " + to_string(clampInt(player.skill - error, 1, 99)) + "-" +
                      to_string(clampInt(player.skill + error, 1, 99)) +
                      " | Pot " + to_string(clampInt(player.potential - error, player.skill, 99)) + "-" +
                      to_string(clampInt(player.potential + error, player.skill, 99)) +
                      " | Contrato " + to_string(player.contractWeeks) +
                      " | Valor " + formatMoney(player.value) +
                      " | Pie " + player.preferredFoot +
                      " | Sec " + (player.secondaryPositions.empty() ? string("-") : joinStringValues(player.secondaryPositions, "/")) +
                      " | Forma " + playerFormLabel(player) +
                      " | Fiabilidad " + playerReliabilityLabel(player) +
                      " | Rasgos " + joinStringValues(player.traits, ", ") +
                      " | Perfil " + personalityLabel(player);
        result.messages.push_back("- " + note);
        career.scoutInbox.push_back(note);
    }

    if (career.scoutInbox.size() > 40) {
        career.scoutInbox.erase(career.scoutInbox.begin(),
                                career.scoutInbox.begin() + static_cast<long long>(career.scoutInbox.size() - 40));
    }
    career.addNews("El scouting actualiza informes de la shortlist de " + team.name + ".");
    return result;
}

std::string buildCompetitionSummaryService(const Career& career) {
    if (!career.myTeam) return "No hay carrera activa.";
    LeagueTable table = relevantCompetitionTable(career);
    ostringstream out;
    out << "=== Competicion ===\r\n";
    out << "Division: " << divisionDisplay(career.activeDivision) << " | Temporada " << career.currentSeason
        << " | Fecha " << career.currentWeek << "/" << max(1, static_cast<int>(career.schedule.size())) << "\r\n";
    out << "Club: " << career.myTeam->name << " | Posicion " << career.currentCompetitiveRank() << "/"
        << max(1, career.currentCompetitiveFieldSize()) << " | Pts " << career.myTeam->points << " | DG "
        << (career.myTeam->goalsFor - career.myTeam->goalsAgainst) << "\r\n";
    if (!table.teams.empty()) {
        out << "\r\nTop 5:\r\n";
        int limit = min(5, static_cast<int>(table.teams.size()));
        for (int i = 0; i < limit; ++i) {
            Team* team = table.teams[static_cast<size_t>(i)];
            out << i + 1 << ". " << team->name << " | " << team->points << " pts";
            if (team == career.myTeam) out << " <- tu club";
            out << "\r\n";
        }
    }
    if (career.currentWeek >= 1 && career.currentWeek <= static_cast<int>(career.schedule.size())) {
        out << "\r\nProxima fecha:\r\n";
        for (const auto& match : career.schedule[static_cast<size_t>(career.currentWeek - 1)]) {
            if (match.first < 0 || match.second < 0 ||
                match.first >= static_cast<int>(career.activeTeams.size()) ||
                match.second >= static_cast<int>(career.activeTeams.size())) {
                continue;
            }
            Team* home = career.activeTeams[static_cast<size_t>(match.first)];
            Team* away = career.activeTeams[static_cast<size_t>(match.second)];
            out << "- " << home->name << " vs " << away->name;
            if (home == career.myTeam || away == career.myTeam) out << " <- tu partido";
            out << "\r\n";
        }
        out << "\r\nInforme rival:\r\n";
        out << "- " << opponentSummary(career) << "\r\n";
    }
    return out.str();
}

std::string buildBoardSummaryService(const Career& career) {
    if (!career.myTeam) return "No hay carrera activa.";
    int youthUsed = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.age <= 20 && player.matchesPlayed > 0) youthUsed++;
    }
    vector<Team*> jobs = buildJobMarket(career);
    ostringstream out;
    out << "=== Directiva ===\r\n";
    out << "Confianza: " << boardStatusLabel(career.boardConfidence) << " (" << career.boardConfidence << "/100)\r\n";
    out << "Reputacion manager: " << career.managerReputation << "/100\r\n";
    out << "Perfil del DT: juego " << managerStyleLabel(*career.myTeam) << "\r\n";
    out << "Advertencias: " << career.boardWarningWeeks << "\r\n";
    out << "Vestuario: " << dressingRoomClimate(*career.myTeam)
        << " | Promesas en riesgo: " << promisesAtRisk(*career.myTeam, career.currentWeek) << "\r\n";
    out << "Presion esperada del club: " << teamExpectationLabel(*career.myTeam)
        << " | Prestigio " << teamPrestigeScore(*career.myTeam) << "\r\n";
    out << "\r\nObjetivos:\r\n";
    out << "- Puesto esperado: " << career.boardExpectedFinish << " | actual " << career.currentCompetitiveRank() << "\r\n";
    out << "- Presupuesto meta: " << formatMoney(career.boardBudgetTarget) << " | actual "
        << formatMoney(career.myTeam->budget) << "\r\n";
    out << "- Juveniles con minutos: " << youthUsed << "/" << career.boardYouthTarget << "\r\n";
    if (!career.boardMonthlyObjective.empty()) {
        out << "\r\nObjetivo mensual:\r\n";
        out << career.boardMonthlyObjective << "\r\n";
        out << "Progreso " << career.boardMonthlyProgress << "/" << career.boardMonthlyTarget << "\r\n";
    }
    out << "\r\nMercado de manager: " << jobs.size() << " club(es) compatibles\r\n";
    for (Team* job : jobs) {
        out << "- " << job->name << " (" << divisionDisplay(job->division) << ")\r\n";
    }
    return out.str();
}

std::string buildClubSummaryService(const Career& career) {
    if (!career.myTeam) return "No hay carrera activa.";
    const Team& team = *career.myTeam;
    int youthProjects = 0;
    for (const auto& player : team.players) {
        if (player.age <= 20 && player.potential - player.skill >= 8) youthProjects++;
    }
    vector<const Player*> leaders = dressingRoomLeaders(team);
    ostringstream out;
    out << "=== Club y Finanzas ===\r\n";
    out << "Presupuesto: " << formatMoney(team.budget) << " | Deuda: " << formatMoney(team.debt) << "\r\n";
    out << "Sponsor semanal: " << formatMoney(team.sponsorWeekly)
        << " | Masa salarial: " << formatMoney(weeklyWage(team)) << "\r\n";
    out << "Hinchas: " << team.fanBase << " | Valor plantel: " << formatMoney(team.getSquadValue()) << "\r\n";
    out << "Prestigio: " << teamPrestigeScore(team)
        << " | Identidad: " << (team.clubStyle.empty() ? "Equilibrio competitivo" : team.clubStyle)
        << " | Cantera: " << (team.youthIdentity.empty() ? "Talento local" : team.youthIdentity) << "\r\n";
    out << "Rival principal: " << (team.primaryRival.empty() ? string("Sin clasico definido") : team.primaryRival)
        << " | Expectativa: " << teamExpectationLabel(team) << "\r\n";
    out << "Vestuario: " << dressingRoomClimate(team)
        << " | Instruccion: " << team.matchInstruction
        << " | Proyectos juveniles: " << youthProjects << "\r\n";
    if (!leaders.empty()) {
        out << "Nucleo de lideres: ";
        for (size_t i = 0; i < leaders.size(); ++i) {
            if (i) out << ", ";
            out << leaders[i]->name;
        }
        out << "\r\n";
    }
    out << "\r\nInfraestructura:\r\n";
    out << "- Estadio " << team.stadiumLevel << " | mejora " << formatMoney(upgradeCost(team, ClubUpgrade::Stadium)) << "\r\n";
    out << "- Cantera " << team.youthFacilityLevel << " | mejora " << formatMoney(upgradeCost(team, ClubUpgrade::Youth)) << "\r\n";
    out << "- Entrenamiento " << team.trainingFacilityLevel << " | mejora " << formatMoney(upgradeCost(team, ClubUpgrade::Training)) << "\r\n";
    out << "- Scouting " << team.scoutingChief << " | mejora " << formatMoney(upgradeCost(team, ClubUpgrade::Scouting)) << "\r\n";
    out << "- Medico " << team.medicalTeam << " | mejora " << formatMoney(upgradeCost(team, ClubUpgrade::Medical)) << "\r\n";
    out << "\r\nRegion juvenil: " << team.youthRegion << "\r\n";
    return out.str();
}

std::string buildScoutingSummaryService(const Career& career) {
    if (!career.myTeam) return "No hay carrera activa.";
    const Team& team = *career.myTeam;
    ostringstream out;
    out << "=== Scouting ===\r\n";
    out << "Jefe de scouting: " << team.scoutingChief << "/99\r\n";
    out << "Costo de informe: " << formatMoney(max(3000LL, 9000LL - team.scoutingChief * 50LL)) << "\r\n";
    out << "Necesidad detectada: " << detectScoutingNeed(team) << "\r\n";
    out << "Informes guardados: " << career.scoutInbox.size() << "\r\n";
    out << "Shortlist activa: " << career.scoutingShortlist.size() << "\r\n";
    if (!career.scoutingShortlist.empty()) {
        out << "\r\nShortlist:\r\n";
        for (const auto& item : career.scoutingShortlist) {
            auto fields = splitByDelimiter(item, '|');
            if (fields.size() < 2) continue;
            const Team* seller = career.findTeamByName(fields[0]);
            if (!seller) continue;
            int sellerIdx = playerIndexByName(*seller, fields[1]);
            if (sellerIdx < 0) continue;
            const Player& player = seller->players[static_cast<size_t>(sellerIdx)];
            out << "- " << player.name << " (" << seller->name << ")"
                << " | Contrato " << player.contractWeeks
                << " | Valor " << formatMoney(player.value)
                << " | Pie " << player.preferredFoot
                << " | Forma " << playerFormLabel(player)
                << " | Fiabilidad " << playerReliabilityLabel(player)
                << " | Rasgos " << joinStringValues(player.traits, ", ")
                << " | Perfil " << personalityLabel(player) << "\r\n";
        }
    }
    if (career.scoutInbox.empty()) {
        out << "\r\nNo hay informes disponibles.";
        return out.str();
    }
    out << "\r\nUltimos informes:\r\n";
    int start = max(0, static_cast<int>(career.scoutInbox.size()) - 10);
    for (size_t i = static_cast<size_t>(start); i < career.scoutInbox.size(); ++i) {
        out << "- " << career.scoutInbox[i] << "\r\n";
    }
    return out.str();
}

ValidationSuiteSummary runValidationService() {
    return buildValidationSuiteSummary();
}
