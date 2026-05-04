#include "transfers/negotiation_system.h"

#include "career/dressing_room_service.h"
#include "career/career_reports.h"
#include "engine/manager_stress.h"
#include "utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

double negotiationFeeFactor(NegotiationProfile profile) {
    switch (profile) {
        case NegotiationProfile::Safe: return 1.08;
        case NegotiationProfile::Balanced: return 1.00;
        case NegotiationProfile::Aggressive: return 0.93;
    }
    return 1.00;
}

double negotiationWageFactor(NegotiationProfile profile) {
    switch (profile) {
        case NegotiationProfile::Safe: return 1.12;
        case NegotiationProfile::Balanced: return 1.00;
        case NegotiationProfile::Aggressive: return 0.95;
    }
    return 1.00;
}

double negotiationClauseFactor(NegotiationProfile profile) {
    switch (profile) {
        case NegotiationProfile::Safe: return 2.30;
        case NegotiationProfile::Balanced: return 2.00;
        case NegotiationProfile::Aggressive: return 1.70;
    }
    return 2.00;
}

string negotiationLabel(NegotiationProfile profile) {
    switch (profile) {
        case NegotiationProfile::Safe: return "Segura";
        case NegotiationProfile::Balanced: return "Balanceada";
        case NegotiationProfile::Aggressive: return "Agresiva";
    }
    return "Balanceada";
}

string promiseLabel(NegotiationPromise promise) {
    switch (promise) {
        case NegotiationPromise::None: return "Sin promesa";
        case NegotiationPromise::Starter: return "Titular";
        case NegotiationPromise::Rotation: return "Rotacion";
        case NegotiationPromise::Prospect: return "Proyecto";
    }
    return "Sin promesa";
}

double promiseWageFactor(NegotiationPromise promise) {
    switch (promise) {
        case NegotiationPromise::None: return 1.00;
        case NegotiationPromise::Starter: return 1.08;
        case NegotiationPromise::Rotation: return 1.03;
        case NegotiationPromise::Prospect: return 0.97;
    }
    return 1.00;
}

int promiseContractWeeks(NegotiationPromise promise, int currentWeeks) {
    switch (promise) {
        case NegotiationPromise::None: return max(104, currentWeeks);
        case NegotiationPromise::Starter: return max(124, currentWeeks + 26);
        case NegotiationPromise::Rotation: return max(104, currentWeeks + 12);
        case NegotiationPromise::Prospect: return max(156, currentWeeks + 52);
    }
    return max(104, currentWeeks);
}

int desiredStartsForPromise(NegotiationPromise promise, const Player& player) {
    switch (promise) {
        case NegotiationPromise::None: return player.desiredStarts;
        case NegotiationPromise::Starter: return 4;
        case NegotiationPromise::Rotation: return 2;
        case NegotiationPromise::Prospect: return (player.age <= 21) ? 2 : 1;
    }
    return player.desiredStarts;
}

string personalityLabel(const Player& player) {
    if (playerHasTrait(player, "Lider")) return "Lider";
    if (player.professionalism >= 75 && player.ambition >= 60) return "Competitivo";
    if (player.ambition >= 75 && player.happiness < 50) return "Inquieto";
    if (player.professionalism <= 45) return "Irregular";
    return "Estable";
}

vector<const Player*> dressingRoomLeaders(const Team& team) {
    vector<const Player*> leaders;
    for (const auto& player : team.players) {
        if (player.leadership >= 72 || playerHasTrait(player, "Lider")) {
            leaders.push_back(&player);
        }
    }
    sort(leaders.begin(), leaders.end(), [](const Player* left, const Player* right) {
        if (left->leadership != right->leadership) return left->leadership > right->leadership;
        if (left->chemistry != right->chemistry) return left->chemistry > right->chemistry;
        return left->name < right->name;
    });
    if (leaders.size() > 3) leaders.resize(3);
    return leaders;
}

string dressingRoomClimate(const Team& team) {
    int unhappy = 0;
    int leaders = 0;
    int totalChemistry = 0;
    for (const auto& player : team.players) {
        totalChemistry += player.chemistry;
        if (player.happiness <= 40 || player.wantsToLeave) unhappy++;
        if (player.leadership >= 72 || playerHasTrait(player, "Lider")) leaders++;
    }
    int avgChemistry = team.players.empty() ? 50 : totalChemistry / static_cast<int>(team.players.size());
    if (unhappy >= 4 || avgChemistry <= 42) return "Tenso";
    if (leaders >= 3 && avgChemistry >= 62 && team.morale >= 58) return "Fuerte";
    if (team.morale <= 38 || unhappy >= 2) return "Inestable";
    return "Estable";
}

bool promiseAtRisk(const Player& player, int currentWeek) {
    if (player.promisedRole == "Titular") {
        return player.startsThisSeason + 2 < max(2, currentWeek * 2 / 3);
    }
    if (player.promisedRole == "Rotacion") {
        return player.startsThisSeason + 1 < max(1, currentWeek / 3);
    }
    if (player.promisedRole == "Proyecto") {
        return player.age <= 22 && player.startsThisSeason < max(1, currentWeek / 4);
    }
    return false;
}

int promisesAtRisk(const Team& team, int currentWeek) {
    int total = 0;
    for (const auto& player : team.players) {
        if (promiseAtRisk(player, currentWeek)) total++;
    }
    return total;
}

void applyNegotiatedPromise(Player& player, NegotiationPromise promise) {
    if (promise == NegotiationPromise::None) return;
    player.promisedRole = promiseLabel(promise);
    player.promisedPosition = normalizePosition(player.position);
    player.desiredStarts = max(player.desiredStarts, desiredStartsForPromise(promise, player));
    player.wantsToLeave = false;
    player.happiness = clampInt(player.happiness + (promise == NegotiationPromise::Starter ? 5 : 3), 1, 99);
    player.moraleMomentum = clampInt(player.moraleMomentum + (promise == NegotiationPromise::Starter ? 3 : 2), -25, 25);
}

long long estimatedAgentFee(const Player& player, long long transferFee) {
    return max(10000LL, max(transferFee / 20, player.value / 15));
}

long long wageDemandFor(const Player& player) {
    long long performancePremium = static_cast<long long>(player.currentForm) * 60 +
                                   static_cast<long long>(player.bigMatches) * 40 +
                                   static_cast<long long>(player.consistency) * 30;
    performancePremium += static_cast<long long>(player.leadership) * 20;
    performancePremium += static_cast<long long>(max(0, player.potential - player.skill)) * 55;
    performancePremium -= static_cast<long long>(player.injuryProneness) * 18;
    return max(player.wage, static_cast<long long>(player.skill) * 170 + player.potential * 25 + performancePremium);
}

int agentDifficulty(const Player& player) {
    int difficulty = 38 + player.ambition / 2 + max(0, 60 - player.professionalism) / 2;
    difficulty += max(0, 55 - player.discipline) / 3;
    difficulty -= max(0, player.adaptation - 65) / 8;
    if (player.wantsToLeave) difficulty -= 6;
    if (playerHasTrait(player, "Caliente")) difficulty += 8;
    if (playerHasTrait(player, "Lider")) difficulty += 3;
    if (playerHasTrait(player, "Competidor")) difficulty += 4;
    if (player.personality == "Temperamental") difficulty += 5;
    if (player.personality == "Profesional") difficulty -= 3;
    return clampInt(difficulty, 20, 90);
}

int squadCompetitionAtRole(const Team& team, const Player& target) {
    int competition = 0;
    for (const auto& mate : team.players) {
        if (mate.name == target.name) continue;
        if (positionFitScore(mate, target.position) < 70) continue;
        if (mate.skill >= target.skill - 3) competition++;
    }
    return competition;
}

long long rivalrySurcharge(const Team& buyer, const Team& seller, long long baseFee) {
    if (!areRivalClubs(buyer, seller)) return 0;
    return max(25000LL, baseFee / 8);
}

int clubAppealScore(const Career& career, const Team& team) {
    int score = teamPrestigeScore(team);
    score += clampInt(career.managerReputation / 3, 0, 25);
    if (team.division == "primera division") score += 8;
    else if (team.division == "primera b") score += 4;
    score += clampInt(team.fanBase / 6, 0, 14);
    score += clampInt(team.trainingFacilityLevel * 2 + team.youthFacilityLevel, 0, 14);
    score += clampInt(team.performanceAnalyst / 12 + team.assistantCoach / 14, 0, 10);
    return clampInt(score, 20, 99);
}

int projectAppealAdjustment(const Career& career, const Team& buyer, const Player& player) {
    int adjustment = 0;
    const DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(buyer, career.currentWeek);
    if (dressing.climate == "Fuerte") adjustment += 5;
    else if (dressing.climate == "Tenso") adjustment -= 6;
    if (dressing.promiseRiskCount >= 2) adjustment -= 4;
    if (buyer.youthIdentity == "Cantera estructurada" && player.age <= 21) adjustment += 7;
    if (buyer.transferPolicy == "Cantera y valor futuro" && player.age <= 22) adjustment += 5;
    if (player.adaptation >= 72) adjustment += 3;
    if (player.personality == "Profesional") adjustment += 2;
    if (player.personality == "Temperamental" && dressing.climate != "Fuerte") adjustment -= 4;
    if (buyer.headCoachStyle == "Intensidad" && playerHasTrait(player, "Presiona")) adjustment += 4;
    if (buyer.clubStyle == "Ataque por bandas" &&
        (normalizePosition(player.position) == "DEF" || normalizePosition(player.position) == "DEL")) {
        adjustment += 3;
    }
    if (buyer.clubStyle == "Bloque ordenado" && normalizePosition(player.position) == "DEF") adjustment += 3;
    if (buyer.jobSecurity < 40) adjustment -= 3;
    return adjustment;
}

bool playerRejectsMove(const Career& career,
                       const Team& buyer,
                       const Team& seller,
                       const Player& player,
                       NegotiationPromise promise,
                       string& reason) {
    int appeal = clubAppealScore(career, buyer) + projectAppealAdjustment(career, buyer, player);
    int threshold = teamPrestigeScore(seller) + player.ambition / 4 + agentDifficulty(player) / 6;
    if (promise == NegotiationPromise::Starter) appeal += 8;
    else if (promise == NegotiationPromise::Rotation) appeal += 3;
    else if (promise == NegotiationPromise::Prospect && player.age <= 21) appeal += 5;
    if (squadCompetitionAtRole(buyer, player) >= 3 && promise != NegotiationPromise::Starter) {
        threshold += 8;
    }
    if (buyer.debt > buyer.sponsorWeekly * 18) threshold += 4;
    if (appeal >= threshold) return false;

    if (teamPrestigeScore(buyer) + 4 < teamPrestigeScore(seller)) {
        reason = player.name + " rechaza la propuesta por la diferencia de reputacion entre clubes.";
    } else if (squadCompetitionAtRole(buyer, player) >= 3) {
        reason = player.name + " no ve claro el espacio competitivo sin una promesa mas fuerte.";
    } else if (dressing_room_service::buildSnapshot(buyer, career.currentWeek).climate == "Tenso") {
        reason = player.name + " detecta un vestuario inestable y pide mas garantias antes de aceptar.";
    } else {
        reason = "El agente de " + player.name + " considera insuficiente el proyecto deportivo.";
    }
    return true;
}

bool renewalNeedsStrongerPromise(const Player& player, NegotiationPromise promise, int currentWeek) {
    if (!promiseAtRisk(player, currentWeek)) return false;
    if (player.promisedRole == "Titular" && promise != NegotiationPromise::Starter) return true;
    if (player.promisedRole == "Rotacion" && promise == NegotiationPromise::None) return true;
    if (player.promisedRole == "Proyecto" && promise == NegotiationPromise::None && player.age <= 22) return true;
    return false;
}

namespace {

// Apply manager stress modifier to negotiation calculations
// High stress: manager makes poor decisions (accepts worse deals)
// Low stress: manager drives harder bargains
double getStressNegotiationModifier(const ManagerStressState& stress) {
    if (stress.stressLevel > 80) return 1.08;  // Very stressed: accepts 8% worse deals
    if (stress.stressLevel > 65) return 1.05;  // Stressed: accepts 5% worse deals
    if (stress.stressLevel > 50) return 1.02;  // Moderately stressed: slightly worse
    if (stress.stressLevel < 25) return 0.97;  // Very calm: negotiates better (-3%)
    if (stress.stressLevel < 40) return 0.99;  // Calm: negotiates slightly better
    return 1.00;  // Normal range
}

long long moveTowards(long long from, long long to, double ratio) {
    if (from >= to) return from;
    return from + static_cast<long long>((to - from) * ratio + 0.5);
}

void pushRound(NegotiationState& state, const string& line) {
    state.round++;
    state.roundSummaries.push_back("R" + to_string(state.round) + ": " + line);
}

int competitionPressureScore(const Career& career, const Team& buyer, const Team& seller, const Player& player) {
    int score = max(0, player.skill - buyer.getAverageSkill() + 6);
    score += max(0, player.potential - 78) / 2;
    score += max(0, teamPrestigeScore(seller) - teamPrestigeScore(buyer) + 6) / 2;
    score += clampInt(career.managerReputation / 10, 0, 8);
    score += player.ambition / 12;
    return clampInt(score, 0, 40);
}

long long transferExpectation(const Team& buyer,
                              const Team& seller,
                              const Player& player,
                              bool competingClubPresent,
                              const ManagerStressState& stress) {
    long long expectation = max(player.value, player.releaseClause * 65 / 100);
    if (player.contractWeeks <= 20) expectation = expectation * 92 / 100;
    if (player.wantsToLeave) expectation = expectation * 90 / 100;
    expectation += rivalrySurcharge(buyer, seller, expectation);
    if (competingClubPresent) expectation += max(15000LL, expectation / 10);
    
    // Apply manager stress: stressed managers willing to pay more
    double stressModifier = getStressNegotiationModifier(stress);
    expectation = static_cast<long long>(expectation * stressModifier);
    
    return expectation;
}

long long transferThreshold(long long expectation, const Player& player) {
    long long threshold = expectation * 96 / 100;
    if (player.contractWeeks <= 20) threshold = expectation * 90 / 100;
    if (player.wantsToLeave) threshold = min(threshold, expectation * 88 / 100);
    return threshold;
}

long long negotiatedWageDemand(const Career& career,
                               const Team& buyer,
                               const Team& seller,
                               const Player& player,
                               NegotiationProfile profile,
                               NegotiationPromise promise,
                               bool competingClubPresent) {
    long long wage = max(player.wage, static_cast<long long>(wageDemandFor(player) *
                                                             negotiationWageFactor(profile) *
                                                             promiseWageFactor(promise)));
    
    // Apply manager stress: stressed managers accept higher wages
    double stressModifier = getStressNegotiationModifier(career.managerStress);
    wage = static_cast<long long>(wage * stressModifier);
    
    const int projectAdjustment = projectAppealAdjustment(career, buyer, player);
    wage = wage * (100 + agentDifficulty(player) / 14) / 100;
    if (competingClubPresent) wage = wage * 106 / 100;
    if (teamPrestigeScore(buyer) + 4 < teamPrestigeScore(seller)) wage = wage * 103 / 100;
    if (promise == NegotiationPromise::Starter) wage = wage * 102 / 100;
    if (promise == NegotiationPromise::Prospect && player.age <= 21) wage = wage * 97 / 100;
    if (buyer.debt > buyer.sponsorWeekly * 18) wage = wage * 102 / 100;
    if (clubAppealScore(career, buyer) > clubAppealScore(career, seller) + 10) wage = wage * 97 / 100;
    if (projectAdjustment >= 8) wage = wage * 96 / 100;
    else if (projectAdjustment <= -6) wage = wage * 105 / 100;
    return max(player.wage, wage);
}

long long signingBonusDemand(const Player& player, long long referenceFee, bool preContract) {
    long long basis = preContract ? wageDemandFor(player) * 4 : estimatedAgentFee(player, referenceFee);
    return max(preContract ? 20000LL : 12000LL, basis / (preContract ? 1 : 2));
}

bool negotiatePlayerSide(const Career& career,
                         const Team& buyer,
                         const Team& seller,
                         const Player& player,
                         NegotiationProfile profile,
                         NegotiationPromise promise,
                         bool competingClubPresent,
                         bool preContract,
                         long long referenceFee,
                         NegotiationState& state) {
    string rejectionReason;
    if (playerRejectsMove(career, buyer, seller, player, promise, rejectionReason)) {
        state.status = rejectionReason;
        pushRound(state, rejectionReason);
        return false;
    }

    const long long demandedWage =
        negotiatedWageDemand(career, buyer, seller, player, profile, promise, competingClubPresent);
    const long long demandedBonus = signingBonusDemand(player, referenceFee, preContract);
    const long long demandedAgentFee = estimatedAgentFee(player, max(referenceFee, player.value / 2 + 1));
    const long long demandedLoyaltyBonus = max(preContract ? 18000LL : 12000LL, demandedWage * (preContract ? 5 : 3));
    const long long demandedAppearanceBonus = max(500LL, demandedWage / 26);
    const int demandedWeeks = promiseContractWeeks(promise, max(preContract ? 104 : player.contractWeeks, 78));
    const long long demandedClause =
        max(static_cast<long long>(player.value * negotiationClauseFactor(profile)),
            max(demandedWage * 42, (referenceFee + demandedBonus + demandedAgentFee) * 2));

    state.playerDemand = demandedWage;
    state.agreedPromisedRole = promiseLabel(promise);

    long long openingWage = max(player.wage, demandedWage * (profile == NegotiationProfile::Safe ? 98 : 91) / 100);
    if (profile == NegotiationProfile::Aggressive) openingWage = max(player.wage, demandedWage * 84 / 100);
    pushRound(state,
              "Propuesta salarial " + formatMoneyValue(openingWage) +
                  " | demanda del agente " + formatMoneyValue(demandedWage) +
                  " | fee agente " + formatMoneyValue(demandedAgentFee) +
                  (competingClubPresent ? " | hay competencia" : ""));

    long long improvedWage = moveTowards(openingWage,
                                         demandedWage,
                                         profile == NegotiationProfile::Safe ? 0.82
                                         : profile == NegotiationProfile::Balanced ? 0.66
                                                                                  : 0.52);
    pushRound(state, "El club mejora salario a " + formatMoneyValue(improvedWage) +
                         " y ofrece rol " + promiseLabel(promise) + ".");

    long long finalWage = moveTowards(improvedWage,
                                      demandedWage,
                                      profile == NegotiationProfile::Safe ? 0.90
                                      : profile == NegotiationProfile::Balanced ? 0.80
                                                                               : 0.68);
    long long acceptanceFloor = demandedWage * (promise == NegotiationPromise::Starter ? 95 : 97) / 100;
    if (promise == NegotiationPromise::Prospect && player.age <= 21) {
        acceptanceFloor = demandedWage * 93 / 100;
    }

    if (finalWage < acceptanceFloor) {
        state.status = "El entorno de " + player.name + " considera insuficiente la propuesta contractual.";
        pushRound(state, state.status);
        return false;
    }

    state.playerAccepted = true;
    state.agreedWage = finalWage;
    state.agreedBonus = demandedBonus;
    state.agreedAgentFee = demandedAgentFee;
    state.agreedLoyaltyBonus = demandedLoyaltyBonus;
    state.agreedAppearanceBonus = demandedAppearanceBonus;
    state.agreedClause = demandedClause;
    state.agreedContractWeeks = demandedWeeks;
    pushRound(state,
              "Acuerdo con el jugador: salario " + formatMoneyValue(state.agreedWage) +
                  ", firma " + formatMoneyValue(state.agreedBonus) +
                  ", agente " + formatMoneyValue(state.agreedAgentFee) +
                  ", fidelidad " + formatMoneyValue(state.agreedLoyaltyBonus) +
                  ", bonus por partido " + formatMoneyValue(state.agreedAppearanceBonus) +
                  ", clausula " + formatMoneyValue(state.agreedClause) + ".");
    return true;
}

}  // namespace

NegotiationState runTransferNegotiation(const Career& career,
                                        const Team& buyer,
                                        const Team& seller,
                                        const Player& player,
                                        NegotiationProfile profile,
                                        NegotiationPromise promise) {
    NegotiationState state;
    state.competingClubPresent = competitionPressureScore(career, buyer, seller, player) >= 16;
    state.sellerExpectation = transferExpectation(buyer, seller, player, state.competingClubPresent, career.managerStress);

    long long openingFee = state.sellerExpectation * (profile == NegotiationProfile::Safe ? 97 : 90) / 100;
    if (profile == NegotiationProfile::Aggressive) openingFee = state.sellerExpectation * 83 / 100;
    pushRound(state,
              "Oferta inicial al club: " + formatMoneyValue(openingFee) +
                  " | expectativa vendedora " + formatMoneyValue(state.sellerExpectation) +
                  (state.competingClubPresent ? " | otro club sigue la operacion" : ""));

    long long sellerCounter = moveTowards(openingFee, state.sellerExpectation, 0.72);
    state.latestCounter = sellerCounter;
    pushRound(state, seller.name + " responde con " + formatMoneyValue(sellerCounter) + ".");

    long long improvedFee = moveTowards(openingFee,
                                        state.sellerExpectation,
                                        profile == NegotiationProfile::Safe ? 0.78
                                        : profile == NegotiationProfile::Balanced ? 0.64
                                                                                  : 0.48);
    pushRound(state, buyer.name + " mejora la oferta a " + formatMoneyValue(improvedFee) + ".");

    long long finalFee = moveTowards(improvedFee,
                                     state.sellerExpectation,
                                     profile == NegotiationProfile::Safe ? 0.92
                                     : profile == NegotiationProfile::Balanced ? 0.82
                                                                              : 0.68);
    if (state.competingClubPresent) finalFee += max(10000LL, state.sellerExpectation / 28);
    state.latestCounter = finalFee;

    if (finalFee < transferThreshold(state.sellerExpectation, player)) {
        state.status = seller.name + " mantiene una contraoferta fuera de mercado para " + player.name + ".";
        pushRound(state, state.status);
        return state;
    }

    state.clubAccepted = true;
    state.agreedFee = finalFee;
    pushRound(state, "Acuerdo con " + seller.name + " por " + formatMoneyValue(state.agreedFee) + ".");

    if (!negotiatePlayerSide(career,
                             buyer,
                             seller,
                             player,
                             profile,
                             promise,
                             state.competingClubPresent,
                             false,
                             state.agreedFee,
                             state)) {
        return state;
    }

    state.status = "Acuerdo total";
    return state;
}

NegotiationState runReleaseClauseNegotiation(const Career& career,
                                             const Team& buyer,
                                             const Team& seller,
                                             const Player& player,
                                             NegotiationProfile profile,
                                             NegotiationPromise promise) {
    NegotiationState state;
    state.clubAccepted = true;
    state.agreedFee = player.releaseClause;
    state.sellerExpectation = player.releaseClause;
    state.latestCounter = player.releaseClause;
    pushRound(state, "Se ejecuta clausula de " + formatMoneyValue(player.releaseClause) + " ante " + seller.name + ".");

    state.competingClubPresent = competitionPressureScore(career, buyer, seller, player) >= 18;
    if (!negotiatePlayerSide(career,
                             buyer,
                             seller,
                             player,
                             profile,
                             promise,
                             state.competingClubPresent,
                             false,
                             state.agreedFee,
                             state)) {
        return state;
    }

    state.status = "Clausula y contrato acordados";
    return state;
}

NegotiationState runPreContractNegotiation(const Career& career,
                                           const Team& buyer,
                                           const Team& seller,
                                           const Player& player,
                                           NegotiationProfile profile,
                                           NegotiationPromise promise) {
    NegotiationState state;
    state.clubAccepted = true;
    state.agreedFee = 0;
    state.sellerExpectation = 0;
    state.latestCounter = 0;
    pushRound(state, "El club actual no recibe fee, pero el agente exige un paquete de entrada competitivo.");

    state.competingClubPresent = competitionPressureScore(career, buyer, seller, player) >= 12;
    if (!negotiatePlayerSide(career,
                             buyer,
                             seller,
                             player,
                             profile,
                             promise,
                             state.competingClubPresent,
                             true,
                             0,
                             state)) {
        return state;
    }

    state.status = "Precontrato acordado";
    return state;
}

NegotiationState runRenewalNegotiation(const Career& career,
                                       const Team& team,
                                       const Player& player,
                                       NegotiationProfile profile,
                                       NegotiationPromise promise,
                                       int currentWeek) {
    (void)career;
    (void)team;
    NegotiationState state;
    if (renewalNeedsStrongerPromise(player, promise, currentWeek)) {
        state.status = player.name + " exige una promesa contractual acorde a su rol actual.";
        pushRound(state, state.status);
        return state;
    }

    state.clubAccepted = true;
    state.sellerExpectation = 0;
    state.competingClubPresent = false;
    const long long demandedWage = max(player.wage,
                                       static_cast<long long>(wageDemandFor(player) *
                                                              negotiationWageFactor(profile) *
                                                              promiseWageFactor(promise)));
    state.playerDemand = demandedWage * (100 + agentDifficulty(player) / 16) / 100;
    long long openingWage = max(player.wage, state.playerDemand * (profile == NegotiationProfile::Safe ? 97 : 90) / 100);
    if (profile == NegotiationProfile::Aggressive) openingWage = max(player.wage, state.playerDemand * 84 / 100);
    pushRound(state, "Renovacion: oferta inicial " + formatMoneyValue(openingWage) +
                         " | demanda " + formatMoneyValue(state.playerDemand) + ".");

    long long improvedWage = moveTowards(openingWage,
                                         state.playerDemand,
                                         profile == NegotiationProfile::Safe ? 0.78
                                         : profile == NegotiationProfile::Balanced ? 0.62
                                                                                  : 0.48);
    pushRound(state, "El agente estira la cuerda y el club responde con " + formatMoneyValue(improvedWage) + ".");

    long long finalWage = moveTowards(improvedWage,
                                      state.playerDemand,
                                      profile == NegotiationProfile::Safe ? 0.88
                                      : profile == NegotiationProfile::Balanced ? 0.78
                                                                               : 0.66);
    long long acceptanceFloor = state.playerDemand * (promise == NegotiationPromise::Starter ? 95 : 97) / 100;
    if (player.wantsToLeave) acceptanceFloor = state.playerDemand;
    if (finalWage < acceptanceFloor) {
        state.status = player.name + " no acepta renovar en esos terminos.";
        pushRound(state, state.status);
        return state;
    }

    state.playerAccepted = true;
    state.agreedWage = finalWage;
    state.agreedBonus = max(10000LL, finalWage * 3);
    state.agreedAgentFee = max(8000LL, estimatedAgentFee(player, player.value / 2 + 1) / 2);
    state.agreedLoyaltyBonus = max(12000LL, finalWage * 3);
    state.agreedAppearanceBonus = max(500LL, finalWage / 28);
    state.agreedClause = max(static_cast<long long>(player.value * negotiationClauseFactor(profile)),
                             finalWage * (profile == NegotiationProfile::Safe ? 48 : 40));
    state.agreedContractWeeks =
        promiseContractWeeks(promise, max(profile == NegotiationProfile::Aggressive ? 78 : 104, player.contractWeeks + 52));
    state.agreedPromisedRole = promiseLabel(promise);
    pushRound(state,
              "Renovacion cerrada: salario " + formatMoneyValue(state.agreedWage) +
                  ", firma " + formatMoneyValue(state.agreedBonus) +
                  ", agente " + formatMoneyValue(state.agreedAgentFee) +
                  ", fidelidad " + formatMoneyValue(state.agreedLoyaltyBonus) +
                  ", bonus por partido " + formatMoneyValue(state.agreedAppearanceBonus) +
                  ", contrato " + to_string(state.agreedContractWeeks) + " semanas.");
    state.status = "Renovacion acordada";
    return state;
}
