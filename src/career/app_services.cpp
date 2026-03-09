#include "app_services.h"

#include "career/season_flow_controller.h"
#include "career/career_reports.h"
#include "career/career_runtime.h"
#include "career/career_support.h"
#include "career/week_simulation.h"
#include "career/team_management.h"
#include "competition.h"
#include "transfers/negotiation_system.h"
#include "utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

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

ServiceResult toServiceResult(const SeasonStepResult& step) {
    ServiceResult result;
    result.ok = step.ok;
    result.messages = step.week.messages;
    if (result.messages.empty()) result.messages.push_back("Semana simulada.");
    return result;
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

void eraseNamedSelection(vector<string>& values, const string& name) {
    values.erase(remove(values.begin(), values.end(), name), values.end());
}

long long upgradeCost(const Team& team, ClubUpgrade upgrade) {
    switch (upgrade) {
        case ClubUpgrade::Stadium: return 60000LL * (team.stadiumLevel + 1);
        case ClubUpgrade::Youth: return 50000LL * (team.youthFacilityLevel + 1);
        case ClubUpgrade::Training: return 55000LL * (team.trainingFacilityLevel + 1);
        case ClubUpgrade::Scouting: return 36000LL + static_cast<long long>(team.scoutingChief) * 1300LL;
        case ClubUpgrade::Medical: return 33000LL + static_cast<long long>(team.medicalTeam) * 1200LL;
        case ClubUpgrade::AssistantCoach: return 35000LL + static_cast<long long>(team.assistantCoach) * 1200LL;
        case ClubUpgrade::FitnessCoach: return 32000LL + static_cast<long long>(team.fitnessCoach) * 1200LL;
        case ClubUpgrade::YouthCoach: return 34000LL + static_cast<long long>(team.youthCoach) * 1200LL;
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
        case ClubUpgrade::AssistantCoach: return "asistente tecnico";
        case ClubUpgrade::FitnessCoach: return "preparador fisico";
        case ClubUpgrade::YouthCoach: return "jefe de juveniles";
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

SeasonStepResult simulateSeasonStepService(Career& career) {
    SeasonFlowController controller(career);
    return controller.simulateWeek(autoOfferDecision, autoRenewDecision, autoManagerJobDecision);
}

ServiceResult simulateCareerWeekService(Career& career) {
    return toServiceResult(simulateSeasonStepService(career));
}

ScoutingSessionResult runScoutingSessionService(Career& career, const string& region, const string& focusPos) {
    ScoutingSessionResult session;
    if (!career.myTeam) {
        session.service = failure("No hay una carrera activa.");
        return session;
    }
    Team& team = *career.myTeam;
    long long scoutCost = max(3000LL, 9000LL - team.scoutingChief * 50LL);
    if (team.budget < scoutCost) {
        session.service = failure("Presupuesto insuficiente para ojeo.");
        return session;
    }
    session.resolvedRegion = region.empty() ? "Todas" : region;
    session.resolvedFocusPosition = normalizePosition(focusPos);
    if (session.resolvedFocusPosition == "N/A" || session.resolvedFocusPosition.empty()) {
        session.resolvedFocusPosition = detectScoutingNeed(team);
    }

    vector<pair<Team*, int>> reports;
    for (auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (session.resolvedRegion != "Todas" && club.youthRegion != session.resolvedRegion) continue;
        for (size_t i = 0; i < club.players.size(); ++i) {
            const Player& player = club.players[i];
            if (player.onLoan) continue;
            if (positionFitScore(player, session.resolvedFocusPosition) < 70) continue;
            reports.push_back({&club, static_cast<int>(i)});
        }
    }
    if (reports.empty()) {
        session.service = failure("No se encontraron jugadores para ese informe.");
        return session;
    }
    sort(reports.begin(), reports.end(), [&](const auto& left, const auto& right) {
        const Player& a = left.first->players[static_cast<size_t>(left.second)];
        const Player& b = right.first->players[static_cast<size_t>(right.second)];
        int fitA = a.potential + a.professionalism / 2 + a.currentForm / 2 +
                   positionFitScore(a, session.resolvedFocusPosition);
        int fitB = b.potential + b.professionalism / 2 + b.currentForm / 2 +
                   positionFitScore(b, session.resolvedFocusPosition);
        if (fitA != fitB) return fitA > fitB;
        return a.skill > b.skill;
    });
    if (reports.size() > 5) reports.resize(5);
    int error = clampInt(14 - team.scoutingChief / 10, 2, 10);
    team.budget -= scoutCost;
    session.scoutingCost = scoutCost;
    session.service.ok = true;
    session.service.messages.push_back("Scouting completado en " + session.resolvedRegion +
                                       " con foco " + session.resolvedFocusPosition +
                                       ". Costo " + formatMoneyValue(scoutCost) + ".");
    for (const auto& report : reports) {
        Team* club = report.first;
        const Player& player = club->players[static_cast<size_t>(report.second)];
        int estSkillLo = clampInt(player.skill - error, 1, 99);
        int estSkillHi = clampInt(player.skill + error, 1, 99);
        int estPotLo = clampInt(player.potential - error, player.skill, 99);
        int estPotHi = clampInt(player.potential + error, player.skill, 99);
        int fitScore = positionFitScore(player, session.resolvedFocusPosition);
        string fitLabel = fitScore >= 90 ? "ajuste alto" : (fitScore >= 75 ? "ajuste medio" : "ajuste parcial");

        ScoutingCandidate candidate;
        candidate.playerName = player.name;
        candidate.clubName = club->name;
        candidate.region = club->youthRegion;
        candidate.position = player.position;
        candidate.preferredFoot = player.preferredFoot;
        candidate.fitLabel = fitLabel;
        candidate.formLabel = playerFormLabel(player);
        candidate.reliabilityLabel = playerReliabilityLabel(player);
        candidate.personalityLabel = personalityLabel(player);
        candidate.secondaryPositions = player.secondaryPositions;
        candidate.traits = player.traits;
        candidate.estimatedSkillMin = estSkillLo;
        candidate.estimatedSkillMax = estSkillHi;
        candidate.estimatedPotentialMin = estPotLo;
        candidate.estimatedPotentialMax = estPotHi;
        candidate.fitScore = fitScore;
        candidate.bigMatches = player.bigMatches;
        candidate.marketValue = player.value;
        session.candidates.push_back(candidate);

        string note = player.name + " | " + club->name + " | " + club->youthRegion + " | Hab " +
                      to_string(estSkillLo) + "-" + to_string(estSkillHi) + " | Pot " +
                      to_string(estPotLo) + "-" + to_string(estPotHi) +
                      " | Pie " + player.preferredFoot +
                      " | Sec " + (player.secondaryPositions.empty() ? string("-") : joinStringValues(player.secondaryPositions, "/")) +
                      " | Forma " + playerFormLabel(player) +
                      " | Fiabilidad " + playerReliabilityLabel(player) +
                      " | Rasgos " + joinStringValues(player.traits, ", ") +
                      " | Perfil " + personalityLabel(player);
        session.service.messages.push_back("- " + note);
        career.scoutInbox.push_back(note);
    }
    if (career.scoutInbox.size() > 40) {
        career.scoutInbox.erase(career.scoutInbox.begin(),
                                career.scoutInbox.begin() + static_cast<long long>(career.scoutInbox.size() - 40));
    }
    career.addNews("El scouting completa un informe en la region " + session.resolvedRegion + " para " + team.name + ".");
    return session;
}

ServiceResult scoutPlayersService(Career& career, const string& region, const string& focusPos) {
    return runScoutingSessionService(career, region, focusPos).service;
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
        case ClubUpgrade::AssistantCoach:
            team.assistantCoach = clampInt(team.assistantCoach + 5, 1, 99);
            message = team.name + " refuerza su cuerpo tecnico.";
            break;
        case ClubUpgrade::FitnessCoach:
            team.fitnessCoach = clampInt(team.fitnessCoach + 5, 1, 99);
            message = team.name + " mejora su preparacion fisica.";
            break;
        case ClubUpgrade::YouthCoach:
            team.youthCoach = clampInt(team.youthCoach + 5, 1, 99);
            message = team.name + " mejora la conduccion de juveniles.";
            break;
    }
    ensureTeamIdentity(team);
    career.addNews(message);
    ServiceResult result;
    result.ok = true;
    result.messages.push_back(message + " Inversion " + formatMoneyValue(cost) + ".");
    return result;
}

ServiceResult changeYouthRegionService(Career& career, const string& region) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    static const vector<string> regions = {"Metropolitana", "Norte", "Centro", "Sur", "Patagonia"};
    if (find(regions.begin(), regions.end(), region) == regions.end()) {
        return failure("La region juvenil indicada no es valida.");
    }
    Team& team = *career.myTeam;
    if (team.youthRegion == region) return failure("Esa region juvenil ya esta activa.");
    const long long cost = 12000LL;
    if (team.budget < cost) return failure("Presupuesto insuficiente para reorientar la captacion juvenil.");
    team.budget -= cost;
    team.youthRegion = region;
    ensureTeamIdentity(team);
    career.addNews(team.name + " reorienta su captacion juvenil hacia " + team.youthRegion + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Nueva region juvenil: " + region + ". Inversion " + formatMoneyValue(cost) + ".");
    return result;
}

ServiceResult takeManagerJobService(Career& career, const string& teamName, const string& reason) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* team = career.findTeamByName(teamName);
    if (!team) return failure("No se encontro el club seleccionado.");
    if (team == career.myTeam) return failure("Ya diriges ese club.");
    takeManagerJob(career, team, reason.empty() ? string("Cambio de club voluntario.") : reason);
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Nuevo destino: " + career.myTeam->name + ".");
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
    int sellerIdx = team_mgmt::playerIndexByName(*seller, playerName);
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
    team_mgmt::detachPlayerFromSelections(*seller, player.name);
    eraseNamedSelection(career.scoutingShortlist, seller->name + "|" + player.name);
    seller->players.erase(seller->players.begin() + sellerIdx);
    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*seller);
    career.addNews(player.name + " firma con " + career.myTeam->name + " desde " + seller->name + ".");
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Fichaje completado: " + player.name + " llega desde " + seller->name +
                              " por " + formatMoneyValue(transferFee) + " + honorarios " + formatMoneyValue(agentFee) +
                              " | perfil " + negotiationLabel(profile) +
                              " | promesa " + promiseLabel(promise) +
                              (rivalryExtra > 0 ? " | sobreprecio por rivalidad " + formatMoneyValue(rivalryExtra) : "") +
                              " | agente " + to_string(difficulty) + "/90.");
    return result;
}

ServiceResult triggerReleaseClauseService(Career& career,
                                          const string& sellerTeamName,
                                          const string& playerName,
                                          NegotiationProfile profile,
                                          NegotiationPromise promise) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* seller = career.findTeamByName(sellerTeamName);
    if (!seller || seller == career.myTeam) return failure("No se encontro el club vendedor.");
    int sellerIdx = team_mgmt::playerIndexByName(*seller, playerName);
    if (sellerIdx < 0) return failure("No se encontro el jugador seleccionado.");
    int maxSquadSize = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquadSize > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquadSize) {
        return failure("Tu plantel ya alcanzo el maximo permitido para la division.");
    }

    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*seller);
    Player player = seller->players[static_cast<size_t>(sellerIdx)];
    if (player.onLoan) return failure("El jugador esta a prestamo y no esta disponible.");

    string rejectionReason;
    if (playerRejectsMove(career, *career.myTeam, *seller, player, promise, rejectionReason)) {
        return failure(rejectionReason);
    }

    long long transferFee = player.releaseClause;
    int difficulty = agentDifficulty(player);
    long long agentFee = estimatedAgentFee(player, transferFee);
    agentFee = agentFee * (100 + difficulty / 3) / 100;
    if (career.myTeam->budget < transferFee + agentFee) {
        return failure("Presupuesto insuficiente para ejecutar clausula y honorarios.");
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
    team_mgmt::detachPlayerFromSelections(*seller, player.name);
    eraseNamedSelection(career.scoutingShortlist, seller->name + "|" + player.name);
    seller->players.erase(seller->players.begin() + sellerIdx);
    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*seller);
    career.addNews(player.name + " llega a " + career.myTeam->name + " tras ejecutar su clausula en " + seller->name + ".");

    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Clausula ejecutada: " + player.name + " llega desde " + seller->name +
                              " por " + formatMoneyValue(transferFee) + " + honorarios " + formatMoneyValue(agentFee) +
                              " | perfil " + negotiationLabel(profile) +
                              " | promesa " + promiseLabel(promise) +
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
    int sellerIdx = team_mgmt::playerIndexByName(*seller, playerName);
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
    result.messages.push_back("Precontrato firmado para la temporada siguiente. Bono " + formatMoneyValue(signingBonus) +
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
    int index = team_mgmt::playerIndexByName(team, playerName);
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
    result.messages.push_back("Contrato renovado: " + player.name + " | salario " + formatMoneyValue(demandedWage) +
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
    int index = team_mgmt::playerIndexByName(team, playerName);
    if (index < 0) return failure("No se encontro el jugador seleccionado.");
    const Player& player = team.players[static_cast<size_t>(index)];
    if (player.onLoan && !player.parentClub.empty()) return failure("No puedes vender un jugador cedido.");
    long long transferFee = max(10000LL, player.value * 105 / 100);
    team.budget += transferFee;
    team_mgmt::detachPlayerFromSelections(team, player.name);
    team_mgmt::applyDepartureShock(team, player);
    career.addNews(player.name + " sale de " + team.name + " por " + formatMoneyValue(transferFee) + ".");
    team.players.erase(team.players.begin() + index);
    ensureTeamIdentity(team);
    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Venta completada: " + player.name + " deja el club por " + formatMoneyValue(transferFee) + ".");
    return result;
}

ServiceResult loanInPlayerService(Career& career,
                                  const string& sellerTeamName,
                                  const string& playerName,
                                  int loanWeeks) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* seller = career.findTeamByName(sellerTeamName);
    if (!seller || seller == career.myTeam) return failure("No se encontro el club de origen.");
    int sellerIdx = team_mgmt::playerIndexByName(*seller, playerName);
    if (sellerIdx < 0) return failure("No se encontro el jugador seleccionado.");
    int maxSquadSize = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquadSize > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquadSize) {
        return failure("Tu plantel ya alcanzo el maximo permitido para la division.");
    }

    Player player = seller->players[static_cast<size_t>(sellerIdx)];
    if (player.onLoan) return failure("El jugador ya esta cedido.");
    if (player.contractWeeks <= 12) return failure("El jugador no esta disponible para prestamo por su situacion contractual.");

    loanWeeks = clampInt(loanWeeks, 8, 26);
    long long fee = max(15000LL, player.value / 10);
    long long wageShare = max(player.wage / 2, wageDemandFor(player) * 55 / 100);
    if (career.myTeam->budget < fee) return failure("Presupuesto insuficiente para el cargo de prestamo.");

    player.onLoan = true;
    player.parentClub = seller->name;
    player.loanWeeksRemaining = loanWeeks;
    player.wage = wageShare;
    career.myTeam->budget -= fee;
    seller->budget += fee;
    seller->players.erase(seller->players.begin() + sellerIdx);
    career.myTeam->addPlayer(player);
    career.addNews(player.name + " llega a prestamo desde " + seller->name + ".");

    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Prestamo cerrado: " + player.name + " llega desde " + seller->name +
                              " | cargo " + formatMoneyValue(fee) +
                              " | salario semanal " + formatMoneyValue(wageShare) +
                              " | duracion " + to_string(loanWeeks) + " semanas.");
    return result;
}

ServiceResult loanOutPlayerService(Career& career,
                                   const string& playerName,
                                   const string& destinationTeamName,
                                   int loanWeeks) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team* receiver = career.findTeamByName(destinationTeamName);
    if (!receiver || receiver == career.myTeam) return failure("No se encontro el club destino.");
    Team& team = *career.myTeam;
    if (team.players.size() <= 18) return failure("Necesitas mantener al menos 18 jugadores en plantel.");

    int index = team_mgmt::playerIndexByName(team, playerName);
    if (index < 0) return failure("No se encontro el jugador seleccionado.");
    const int maxSquad = getCompetitionConfig(receiver->division).maxSquadSize;
    if (maxSquad > 0 && static_cast<int>(receiver->players.size()) >= maxSquad) {
        return failure("El club destino no tiene cupo de plantel.");
    }

    Player player = team.players[static_cast<size_t>(index)];
    if (player.onLoan) return failure("No puedes ceder un jugador que ya esta prestado.");

    loanWeeks = clampInt(loanWeeks, 8, 26);
    long long fee = max(10000LL, player.value / 12);
    player.onLoan = true;
    player.parentClub = team.name;
    player.loanWeeksRemaining = loanWeeks;
    receiver->addPlayer(player);
    team.budget += fee;
    receiver->budget = max(0LL, receiver->budget - fee);
    team_mgmt::detachPlayerFromSelections(team, player.name);
    team.players.erase(team.players.begin() + index);
    career.addNews(player.name + " sale a prestamo hacia " + receiver->name + ".");

    ServiceResult result;
    result.ok = true;
    result.messages.push_back("Prestamo acordado: " + player.name + " va a " + receiver->name +
                              " | ingreso " + formatMoneyValue(fee) +
                              " | duracion " + to_string(loanWeeks) + " semanas.");
    return result;
}

ServiceResult cyclePlayerDevelopmentPlanService(Career& career, const string& playerName) {
    if (!career.myTeam) return failure("No hay una carrera activa.");
    Team& team = *career.myTeam;
    int index = team_mgmt::playerIndexByName(team, playerName);
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
    int sellerIdx = team_mgmt::playerIndexByName(*seller, playerName);
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
    result.messages.push_back("Seguimiento de shortlist completado. Costo " + formatMoneyValue(cost) + ".");
    int error = clampInt(8 - team.scoutingChief / 18, 1, 5);

    for (const auto& item : career.scoutingShortlist) {
        auto fields = splitByDelimiter(item, '|');
        if (fields.size() < 2) continue;
        Team* seller = career.findTeamByName(fields[0]);
        if (!seller) continue;
        int sellerIdx = team_mgmt::playerIndexByName(*seller, fields[1]);
        if (sellerIdx < 0) continue;
        const Player& player = seller->players[static_cast<size_t>(sellerIdx)];
        string note = "[Seguimiento] " + player.name + " | " + seller->name +
                      " | Hab " + to_string(clampInt(player.skill - error, 1, 99)) + "-" +
                      to_string(clampInt(player.skill + error, 1, 99)) +
                      " | Pot " + to_string(clampInt(player.potential - error, player.skill, 99)) + "-" +
                      to_string(clampInt(player.potential + error, player.skill, 99)) +
                      " | Contrato " + to_string(player.contractWeeks) +
                      " | Valor " + formatMoneyValue(player.value) +
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

std::vector<std::string> listYouthRegionsService() {
    return {"Metropolitana", "Norte", "Centro", "Sur", "Patagonia"};
}

std::string buildCompetitionSummaryService(const Career& career) {
    return formatCareerReport(buildCompetitionReport(career));
}

std::string buildBoardSummaryService(const Career& career) {
    return formatCareerReport(buildBoardReport(career));
}

std::string buildClubSummaryService(const Career& career) {
    return formatCareerReport(buildClubReport(career));
}

std::string buildScoutingSummaryService(const Career& career) {
    return formatCareerReport(buildScoutingReport(career));
}

ValidationSuiteSummary runValidationService() {
    return buildValidationSuiteSummary();
}
