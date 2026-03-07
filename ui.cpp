#include "ui.h"

#include "competition.h"
#include "simulation.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

static ManagerJobSelectionCallback g_managerJobSelectionCallback = nullptr;
static UiMessageCallback g_uiMessageCallback = nullptr;
static IncomingOfferDecisionCallback g_incomingOfferDecisionCallback = nullptr;
static ContractRenewalDecisionCallback g_contractRenewalDecisionCallback = nullptr;

void setManagerJobSelectionCallback(ManagerJobSelectionCallback callback) {
    g_managerJobSelectionCallback = callback;
}

void setUiMessageCallback(UiMessageCallback callback) {
    g_uiMessageCallback = callback;
}

void setIncomingOfferDecisionCallback(IncomingOfferDecisionCallback callback) {
    g_incomingOfferDecisionCallback = callback;
}

void setContractRenewalDecisionCallback(ContractRenewalDecisionCallback callback) {
    g_contractRenewalDecisionCallback = callback;
}

static void emitUiMessage(const string& message) {
    if (g_uiMessageCallback) {
        g_uiMessageCallback(message);
    } else {
        cout << message << endl;
    }
}

static long long estimatedAgentFee(const Player& player, long long transferFee) {
    return max(10000LL, max(transferFee / 20, player.value / 15));
}

static long long wageDemandFor(const Player& player) {
    long long performancePremium = static_cast<long long>(player.currentForm) * 60 +
                                   static_cast<long long>(player.bigMatches) * 40 +
                                   static_cast<long long>(player.consistency) * 30;
    return max(player.wage, static_cast<long long>(player.skill) * 170 + player.potential * 25 + performancePremium);
}

static int playerIndexByName(const Team& team, const string& name) {
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

static string joinTeamNames(const vector<Team*>& teams) {
    string out;
    for (size_t i = 0; i < teams.size(); ++i) {
        if (!teams[i]) continue;
        if (!out.empty()) out += ", ";
        out += teams[i]->name;
    }
    return out;
}

static vector<pair<Team*, int>> buildTransferPool(Career& career, const string& filterPos, bool includeClauseTargets) {
    vector<pair<Team*, int>> pool;
    for (auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (!includeClauseTargets && club.players.size() <= 18) continue;
        for (size_t i = 0; i < club.players.size(); ++i) {
            const Player& player = club.players[i];
            if (player.onLoan) continue;
            if (!filterPos.empty() && positionFitScore(player, filterPos) < 70) continue;
            if (player.age > 35) continue;
            pool.push_back({&club, static_cast<int>(i)});
        }
    }
    sort(pool.begin(), pool.end(), [](const pair<Team*, int>& a, const pair<Team*, int>& b) {
        const Player& pa = a.first->players[a.second];
        const Player& pb = b.first->players[b.second];
        if (pa.skill != pb.skill) return pa.skill > pb.skill;
        if (pa.value != pb.value) return pa.value < pb.value;
        return pa.name < pb.name;
    });
    if (pool.size() > 25) pool.resize(25);
    return pool;
}

static bool negotiatePersonalTerms(Team& buyer, Player& player, long long transferFee, long long& agentFee) {
    long long minWage = wageDemandFor(player);
    int minWeeks = 52;
    agentFee = estimatedAgentFee(player, transferFee);
    cout << "Demandas del agente: salario >= $" << minWage
         << ", contrato minimo " << minWeeks << " semanas"
         << ", honorarios $" << agentFee << endl;
    long long offeredWage = readLongLong("Ofrece salario semanal: ", 0, 1000000000000LL);
    int contractWeeks = readInt("Semanas de contrato: ", 26, 260);
    if (offeredWage < minWage || contractWeeks < minWeeks) {
        cout << "El jugador rechazo las condiciones personales." << endl;
        return false;
    }
    if (buyer.budget < transferFee + agentFee) {
        cout << "Presupuesto insuficiente para cubrir transferencia y honorarios." << endl;
        return false;
    }
    player.wage = offeredWage;
    player.contractWeeks = contractWeeks;
    player.releaseClause = max(player.value * 2, (transferFee + agentFee) * 2);
    return true;
}

static void finalizeTransfer(Team& buyer, Team* seller, int sellerIndex, Player player, long long transferFee, long long agentFee) {
    buyer.budget -= (transferFee + agentFee);
    player.onLoan = false;
    player.parentClub.clear();
    player.loanWeeksRemaining = 0;
    buyer.addPlayer(player);
    if (seller && sellerIndex >= 0 && sellerIndex < static_cast<int>(seller->players.size())) {
        seller->budget += transferFee;
        seller->players.erase(seller->players.begin() + sellerIndex);
    }
}

static void buyFromClub(Career& career) {
    if (!career.myTeam) return;
    int maxSquadSize = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquadSize > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquadSize) {
        cout << "Tu plantel ya alcanzo el maximo permitido para la division." << endl;
        return;
    }
    string filter = normalizePosition(readLine("Filtrar por posicion (ARQ/DEF/MED/DEL o Enter para todas): "));
    if (filter == "N/A") filter.clear();
    auto pool = buildTransferPool(career, filter, false);
    if (pool.empty()) {
        cout << "No hay jugadores transferibles en este momento." << endl;
        return;
    }

    cout << "\nJugadores disponibles:" << endl;
    for (size_t i = 0; i < pool.size(); ++i) {
        Team* seller = pool[i].first;
        const Player& player = seller->players[pool[i].second];
        long long ask = max(player.value, player.releaseClause * 65 / 100);
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << seller->name << ")"
             << " Hab " << player.skill << " | Valor $" << player.value
             << " | Pie " << player.preferredFoot
             << " | Forma " << playerFormLabel(player)
             << " | Pedido $" << ask << " | Clausula $" << player.releaseClause << endl;
    }
    int choice = readInt("Elige jugador (0 para cancelar): ", 0, static_cast<int>(pool.size()));
    if (choice == 0) return;

    Team* seller = pool[choice - 1].first;
    int sellerIdx = pool[choice - 1].second;
    const Player& target = seller->players[sellerIdx];
    long long minAccept = max(target.value, target.releaseClause * 60 / 100);
    long long ask = max(minAccept, target.value * (105 + randInt(0, 20)) / 100);
    cout << "El club pide alrededor de $" << ask << " por " << target.name << endl;
    long long offer = readLongLong("Oferta de transferencia: ", 0, 1000000000000LL);
    if (offer < minAccept) {
        cout << seller->name << " rechazo la oferta. Minimo estimado: $" << minAccept << endl;
        return;
    }
    if (offer < ask && randInt(1, 100) <= 45) {
        cout << seller->name << " responde con una contraoferta de $" << ask << endl;
        int acceptCounter = readInt("Aceptar contraoferta? (1. Si, 2. No): ", 1, 2);
        if (acceptCounter != 1) return;
        offer = ask;
    }

    Player player = target;
    long long agentFee = 0;
    if (!negotiatePersonalTerms(*career.myTeam, player, offer, agentFee)) return;
    finalizeTransfer(*career.myTeam, seller, sellerIdx, player, offer, agentFee);
    career.addNews(player.name + " firma con " + career.myTeam->name + " desde " + seller->name + ".");
    cout << "Fichaje completado: " << player.name << " llega desde " << seller->name
         << " por $" << offer << " + honorarios $" << agentFee << endl;
}

static void triggerReleaseClause(Career& career) {
    if (!career.myTeam) return;
    int maxSquadSize = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquadSize > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquadSize) {
        cout << "Tu plantel ya alcanzo el maximo permitido para la division." << endl;
        return;
    }
    string filter = normalizePosition(readLine("Filtrar por posicion (ARQ/DEF/MED/DEL o Enter para todas): "));
    if (filter == "N/A") filter.clear();
    auto pool = buildTransferPool(career, filter, true);
    if (pool.empty()) {
        cout << "No hay clausulas ejecutables disponibles." << endl;
        return;
    }

    cout << "\nObjetivos con clausula:" << endl;
    for (size_t i = 0; i < pool.size(); ++i) {
        Team* seller = pool[i].first;
        const Player& player = seller->players[pool[i].second];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << seller->name << ")"
             << " Hab " << player.skill << " | Clausula $" << player.releaseClause
             << " | Salario actual $" << player.wage << endl;
    }
    int choice = readInt("Ejecutar clausula de: ", 0, static_cast<int>(pool.size()));
    if (choice == 0) return;

    Team* seller = pool[choice - 1].first;
    int sellerIdx = pool[choice - 1].second;
    Player player = seller->players[sellerIdx];
    long long clause = player.releaseClause;
    long long agentFee = 0;
    cout << "Ejecutar clausula cuesta $" << clause << endl;
    if (!negotiatePersonalTerms(*career.myTeam, player, clause, agentFee)) return;
    finalizeTransfer(*career.myTeam, seller, sellerIdx, player, clause, agentFee);
    career.addNews(player.name + " llega tras ejecutar su clausula de rescision.");
    cout << "Clausula ejecutada: " << player.name << " firma inmediatamente." << endl;
}

static void sellPlayer(Team& team) {
    const int minSquadSize = 18;
    if (team.players.size() <= static_cast<size_t>(minSquadSize)) {
        cout << "No puedes vender. Debes mantener al menos " << minSquadSize << " jugadores." << endl;
        return;
    }
    cout << "Selecciona jugador para vender:" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        const Player& player = team.players[i];
        cout << i + 1 << ". " << player.name << " (" << player.position << ")"
             << " Valor $" << player.value << " | Clausula $" << player.releaseClause
             << " | Salario $" << player.wage << endl;
    }
    int idx = readInt("Elige jugador: ", 1, static_cast<int>(team.players.size())) - 1;
    Player player = team.players[idx];
    long long marketCeiling = player.value * (95 + randInt(0, 35)) / 100;
    long long askingPrice = readLongLong("Precio pedido: ", 0, 1000000000000LL);
    if (askingPrice <= marketCeiling) {
        team.budget += askingPrice;
        cout << player.name << " vendido por $" << askingPrice << endl;
        team.players.erase(team.players.begin() + idx);
        return;
    }
    if (askingPrice <= marketCeiling * 115 / 100) {
        cout << "El mercado responde con una contraoferta de $" << marketCeiling << endl;
        int accept = readInt("Aceptar? (1. Si, 2. No): ", 1, 2);
        if (accept == 1) {
            team.budget += marketCeiling;
            cout << player.name << " vendido por $" << marketCeiling << endl;
            team.players.erase(team.players.begin() + idx);
        } else {
            cout << "Operacion cancelada." << endl;
        }
        return;
    }
    cout << "No hubo interesados por ese monto." << endl;
}

static void loanInPlayer(Career& career) {
    if (!career.myTeam) return;
    int maxSquadSize = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquadSize > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquadSize) {
        cout << "Tu plantel ya alcanzo el maximo permitido para la division." << endl;
        return;
    }
    auto pool = buildTransferPool(career, "", false);
    vector<pair<Team*, int>> loanable;
    for (const auto& entry : pool) {
        const Player& player = entry.first->players[entry.second];
        if (player.onLoan) continue;
        if (player.contractWeeks <= 12) continue;
        loanable.push_back(entry);
    }
    if (loanable.empty()) {
        cout << "No hay jugadores disponibles para prestamo." << endl;
        return;
    }
    cout << "\nJugadores disponibles a prestamo:" << endl;
    for (size_t i = 0; i < loanable.size(); ++i) {
        const Player& player = loanable[i].first->players[loanable[i].second];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << loanable[i].first->name << ")"
             << " Hab " << player.skill << " | Valor $" << player.value << endl;
    }
    int choice = readInt("Jugador (0 para cancelar): ", 0, static_cast<int>(loanable.size()));
    if (choice == 0) return;

    Team* seller = loanable[choice - 1].first;
    int sellerIdx = loanable[choice - 1].second;
    Player player = seller->players[sellerIdx];
    int loanWeeks = readInt("Duracion del prestamo (8-26 semanas): ", 8, 26);
    long long fee = max(15000LL, player.value / 10);
    long long wageShare = max(player.wage / 2, wageDemandFor(player) * 55 / 100);
    cout << "Condiciones: cargo de prestamo $" << fee << ", salario semanal $" << wageShare << endl;
    if (career.myTeam->budget < fee) {
        cout << "Presupuesto insuficiente." << endl;
        return;
    }
    player.onLoan = true;
    player.parentClub = seller->name;
    player.loanWeeksRemaining = loanWeeks;
    player.wage = wageShare;
    career.myTeam->budget -= fee;
    seller->budget += fee;
    seller->players.erase(seller->players.begin() + sellerIdx);
    career.myTeam->addPlayer(player);
    career.addNews(player.name + " llega a prestamo desde " + seller->name + ".");
    cout << "Prestamo cerrado." << endl;
}

static void loanOutPlayer(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->players.size() <= 18) {
        cout << "Necesitas mantener al menos 18 jugadores en plantel." << endl;
        return;
    }
    vector<int> candidates;
    for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
        const Player& player = career.myTeam->players[i];
        if (player.onLoan) continue;
        if (player.age > 30) continue;
        candidates.push_back(static_cast<int>(i));
    }
    if (candidates.empty()) {
        cout << "No hay jugadores aptos para ceder." << endl;
        return;
    }
    cout << "\nJugadores para ceder:" << endl;
    for (size_t i = 0; i < candidates.size(); ++i) {
        const Player& player = career.myTeam->players[candidates[i]];
        cout << i + 1 << ". " << player.name << " (" << player.position << ") Hab " << player.skill
             << " | Edad " << player.age << endl;
    }
    int playerChoice = readInt("Jugador (0 para cancelar): ", 0, static_cast<int>(candidates.size()));
    if (playerChoice == 0) return;

    vector<Team*> destinations;
    for (auto* team : career.activeTeams) {
        int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        if (team && team != career.myTeam &&
            (maxSquad <= 0 || static_cast<int>(team->players.size()) < maxSquad)) {
            destinations.push_back(team);
        }
    }
    if (destinations.empty()) {
        cout << "No hay clubes receptores disponibles." << endl;
        return;
    }
    cout << "\nClubes interesados:" << endl;
    for (size_t i = 0; i < destinations.size(); ++i) {
        cout << i + 1 << ". " << destinations[i]->name << endl;
    }
    int clubChoice = readInt("Club destino: ", 1, static_cast<int>(destinations.size()));
    int loanWeeks = readInt("Duracion del prestamo (8-26 semanas): ", 8, 26);

    int idx = candidates[playerChoice - 1];
    Player player = career.myTeam->players[idx];
    Team* receiver = destinations[clubChoice - 1];
    long long fee = max(10000LL, player.value / 12);
    player.onLoan = true;
    player.parentClub = career.myTeam->name;
    player.loanWeeksRemaining = loanWeeks;
    receiver->addPlayer(player);
    career.myTeam->budget += fee;
    receiver->budget = max(0LL, receiver->budget - fee);
    career.myTeam->players.erase(career.myTeam->players.begin() + idx);
    career.addNews(player.name + " sale a prestamo hacia " + receiver->name + ".");
    cout << "Prestamo acordado. Ingreso $" << fee << endl;
}

static void signPreContract(Career& career) {
    if (!career.myTeam) return;
    vector<pair<Team*, int>> pool = buildTransferPool(career, "", true);
    vector<pair<Team*, int>> eligible;
    for (const auto& entry : pool) {
        const Player& player = entry.first->players[entry.second];
        if (player.contractWeeks > 12) continue;
        if (player.onLoan) continue;
        bool alreadyPending = false;
        for (const auto& move : career.pendingTransfers) {
            if (move.playerName == player.name && move.toTeam == career.myTeam->name && move.preContract) {
                alreadyPending = true;
                break;
            }
        }
        if (!alreadyPending) eligible.push_back(entry);
    }
    if (eligible.empty()) {
        cout << "No hay jugadores elegibles para precontrato." << endl;
        return;
    }
    cout << "\nJugadores elegibles para precontrato:" << endl;
    for (size_t i = 0; i < eligible.size(); ++i) {
        const Player& player = eligible[i].first->players[eligible[i].second];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << eligible[i].first->name << ")"
             << " Hab " << player.skill << " | Contrato restante " << player.contractWeeks << " sem" << endl;
    }
    int choice = readInt("Jugador (0 para cancelar): ", 0, static_cast<int>(eligible.size()));
    if (choice == 0) return;

    Team* from = eligible[choice - 1].first;
    Player player = from->players[eligible[choice - 1].second];
    long long signingBonus = max(20000LL, wageDemandFor(player) * 4);
    cout << "Bono de firma: $" << signingBonus << endl;
    if (career.myTeam->budget < signingBonus) {
        cout << "Presupuesto insuficiente." << endl;
        return;
    }
    long long newWage = readLongLong("Salario semanal ofrecido: ", 0, 1000000000000LL);
    int contractWeeks = readInt("Semanas de contrato ofrecidas: ", 52, 260);
    if (newWage < wageDemandFor(player)) {
        cout << "El jugador rechazo la oferta." << endl;
        return;
    }
    career.myTeam->budget -= signingBonus;
    career.pendingTransfers.push_back({player.name, from->name, career.myTeam->name, career.currentSeason + 1, 0, 0,
                                       newWage, contractWeeks, true, false, "Sin promesa"});
    career.addNews(player.name + " firma un precontrato con " + career.myTeam->name + ".");
    cout << "Precontrato firmado para la temporada siguiente." << endl;
}

void transferMarket(Career& career) {
    if (!career.myTeam) return;
    Team& team = *career.myTeam;
    cout << "\n=== Mercado de Transferencias ===" << endl;
    cout << "Presupuesto: $" << team.budget << endl;
    cout << "1. Comprar desde otro club" << endl;
    cout << "2. Ejecutar clausula de rescision" << endl;
    cout << "3. Precontrato" << endl;
    cout << "4. Pedir prestamo" << endl;
    cout << "5. Ceder a prestamo" << endl;
    cout << "6. Vender jugador" << endl;
    cout << "7. Volver" << endl;
    int choice = readInt("Elige una opcion: ", 1, 7);

    if (choice == 1) {
        buyFromClub(career);
    } else if (choice == 2) {
        triggerReleaseClause(career);
    } else if (choice == 3) {
        signPreContract(career);
    } else if (choice == 4) {
        loanInPlayer(career);
    } else if (choice == 5) {
        loanOutPlayer(career);
    } else if (choice == 6) {
        sellPlayer(team);
    }
}

void scoutPlayers(Career& career) {
    if (!career.myTeam) return;
    Team& team = *career.myTeam;
    cout << "\n=== Ojeo de Jugadores ===" << endl;
    cout << "Presupuesto: $" << team.budget << endl;
    cout << "Jefe de scouting: " << team.scoutingChief << "/99" << endl;
    long long scoutCost = max(3000LL, 9000LL - team.scoutingChief * 50LL);
    cout << "Costo de ojeo: $" << scoutCost << endl;
    cout << "1. Otear jugadores" << endl;
    cout << "2. Ver informes guardados" << endl;
    cout << "3. Volver" << endl;
    int choice = readInt("Elige una opcion: ", 1, 3);

    if (choice == 2) {
        if (career.scoutInbox.empty()) {
            cout << "No hay informes guardados." << endl;
            return;
        }
        for (const auto& note : career.scoutInbox) {
            cout << "- " << note << endl;
        }
        return;
    }
    if (choice == 3) return;

    if (team.budget < scoutCost) {
        cout << "Presupuesto insuficiente para ojeo." << endl;
        return;
    }
    team.budget -= scoutCost;
    cout << "Regiones: 1. Metropolitana 2. Norte 3. Centro 4. Sur 5. Patagonia 6. Todas" << endl;
    int regionChoice = readInt("Elegir region: ", 1, 6);
    vector<string> regionOptions = {"Metropolitana", "Norte", "Centro", "Sur", "Patagonia", "Todas"};
    string region = regionOptions[regionChoice - 1];
    string focusPos = normalizePosition(readLine("Foco de posicion (ARQ/DEF/MED/DEL o Enter para necesidad): "));
    if (focusPos == "N/A") {
        unordered_map<string, int> counts;
        unordered_map<string, int> skills;
        for (const auto& player : team.players) {
            string pos = normalizePosition(player.position);
            if (pos == "N/A") continue;
            counts[pos]++;
            skills[pos] += player.skill;
        }
        vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
        focusPos = "MED";
        int bestScore = 1000000;
        for (const auto& pos : positions) {
            int countPos = counts[pos];
            int avgSkill = countPos > 0 ? skills[pos] / countPos : 0;
            int score = countPos * 20 + avgSkill;
            if (score < bestScore) {
                bestScore = score;
                focusPos = pos;
            }
        }
        cout << "Necesidad detectada: " << focusPos << endl;
    }

    vector<pair<Team*, int>> reports;
    for (auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (region != "Todas" && club.youthRegion != region) continue;
        for (size_t i = 0; i < club.players.size(); ++i) {
            const Player& player = club.players[i];
            if (player.onLoan) continue;
            if (!focusPos.empty() && positionFitScore(player, focusPos) < 70) continue;
            reports.push_back({&club, static_cast<int>(i)});
        }
    }
    if (reports.empty()) {
        cout << "No se encontraron jugadores para ese informe." << endl;
        return;
    }
    sort(reports.begin(), reports.end(), [&](const pair<Team*, int>& a, const pair<Team*, int>& b) {
        const Player& pa = a.first->players[a.second];
        const Player& pb = b.first->players[b.second];
        int aFit = positionFitScore(pa, focusPos) + pa.potential + pa.professionalism / 2 + pa.currentForm / 2;
        int bFit = positionFitScore(pb, focusPos) + pb.potential + pb.professionalism / 2 + pb.currentForm / 2;
        if (aFit != bFit) return aFit > bFit;
        return pa.skill > pb.skill;
    });
    if (reports.size() > 5) reports.resize(5);

    int error = clampInt(14 - team.scoutingChief / 10, 2, 10);
    cout << "\nInforme de scouting:" << endl;
    for (size_t i = 0; i < reports.size(); ++i) {
        Team* club = reports[i].first;
        const Player& player = club->players[reports[i].second];
        int estSkillLo = clampInt(player.skill - error, 1, 99);
        int estSkillHi = clampInt(player.skill + error, 1, 99);
        int estPotLo = clampInt(player.potential - error, player.skill, 99);
        int estPotHi = clampInt(player.potential + error, player.skill, 99);
        int fitScore = positionFitScore(player, focusPos);
        string fit = fitScore >= 90 ? "ajuste alto" : (fitScore >= 75 ? "ajuste medio" : "ajuste parcial");
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << club->name << ", " << club->youthRegion << ")"
             << " Hab " << estSkillLo << "-" << estSkillHi
             << " | Pot " << estPotLo << "-" << estPotHi
             << " | " << fit
             << " | Pie " << player.preferredFoot
             << " | Sec " << (player.secondaryPositions.empty() ? string("-") : joinStringValues(player.secondaryPositions, "/"))
             << " | Forma " << playerFormLabel(player)
             << " | Fiabilidad " << playerReliabilityLabel(player)
             << " | Partidos grandes " << player.bigMatches
             << " | Valor $" << player.value << endl;
        career.scoutInbox.push_back(player.name + " | " + club->name + " | " + club->youthRegion + " | Hab " +
                                    to_string(estSkillLo) + "-" + to_string(estSkillHi) + " | Pot " +
                                    to_string(estPotLo) + "-" + to_string(estPotHi) +
                                    " | Pie " + player.preferredFoot +
                                    " | Sec " + (player.secondaryPositions.empty() ? string("-") : joinStringValues(player.secondaryPositions, "/")) +
                                    " | Forma " + playerFormLabel(player) +
                                    " | Perfil " + playerReliabilityLabel(player));
    }
    if (career.scoutInbox.size() > 30) {
        career.scoutInbox.erase(career.scoutInbox.begin(),
                                career.scoutInbox.begin() + static_cast<long long>(career.scoutInbox.size() - 30));
    }
    career.addNews("El scouting completa un informe en la region " + region + " para " + team.name + ".");

    int pursue = readInt("Intentar fichar uno de los informes? (0 para no): ", 0, static_cast<int>(reports.size()));
    if (pursue == 0) return;
    Team* seller = reports[pursue - 1].first;
    int sellerIdx = reports[pursue - 1].second;
    Player player = seller->players[sellerIdx];
    long long fee = max(player.value, player.releaseClause * 60 / 100);
    long long agentFee = 0;
    cout << "Costo estimado de traspaso: $" << fee << endl;
    if (!negotiatePersonalTerms(team, player, fee, agentFee)) return;
    finalizeTransfer(team, seller, sellerIdx, player, fee, agentFee);
    career.addNews(player.name + " llega tras recomendacion del scouting.");
    cout << "Fichaje concretado tras el informe." << endl;
}

void retirePlayer(Team& team) {
    cout << "\n=== Retiro de Jugadores ===" << endl;
    vector<int> eligibleIndices;
    cout << "Jugadores elegibles para retiro (35-45 anos):" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].age >= 35 && team.players[i].age <= 45) {
            eligibleIndices.push_back(static_cast<int>(i));
            cout << eligibleIndices.size() << ". " << team.players[i].name << " (Edad: " << team.players[i].age << ")" << endl;
        }
    }
    if (eligibleIndices.empty()) {
        cout << "No hay jugadores elegibles para retiro." << endl;
        return;
    }
    int choice = readInt("Selecciona un jugador para retirar (0 para cancelar): ", 0, static_cast<int>(eligibleIndices.size()));
    if (choice >= 1 && choice <= static_cast<int>(eligibleIndices.size())) {
        int index = eligibleIndices[choice - 1];
        cout << team.players[index].name << " se ha retirado." << endl;
        team.players.erase(team.players.begin() + index);
    }
}

void checkAchievements(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->wins >= 10 &&
        find(career.achievements.begin(), career.achievements.end(), "10 Victorias") == career.achievements.end()) {
        career.achievements.push_back("10 Victorias");
        cout << "Logro desbloqueado: 10 Victorias!" << endl;
    }
}

void displayMainMenu() {
    cout << "\n=== Football Manager Game ===" << endl;
    cout << "1. Modo Carrera" << endl;
    cout << "2. Iniciar Juego Rapido" << endl;
    cout << "3. Modo Copa" << endl;
    cout << "4. Validar sistema" << endl;
    cout << "5. Salir" << endl;
}

void displayGameMenu() {
    cout << "\n=== Juego Rapido ===" << endl;
    cout << "1. Ver Equipo" << endl;
    cout << "2. Agregar Jugador" << endl;
    cout << "3. Entrenar Jugador" << endl;
    cout << "4. Cambiar Tacticas" << endl;
    cout << "5. Simular Partido" << endl;
    cout << "6. Cargar Equipo desde Archivo" << endl;
    cout << "7. Editar Equipo" << endl;
    cout << "8. Volver al Menu Principal" << endl;
}

void displayCareerMenu() {
    cout << "\nModo Carrera" << endl;
    cout << "1. Ver Equipo" << endl;
    cout << "2. Entrenar Jugador" << endl;
    cout << "3. Cambiar Tacticas" << endl;
    cout << "4. Simular Semana" << endl;
    cout << "5. Centro de Competicion" << endl;
    cout << "6. Ver Tabla de Posiciones" << endl;
    cout << "7. Mercado de Transferencias" << endl;
    cout << "8. Ojeo de Jugadores" << endl;
    cout << "9. Ver Estadisticas" << endl;
    cout << "10. Objetivos y Directiva" << endl;
    cout << "11. Noticias" << endl;
    cout << "12. Historial de Temporadas" << endl;
    cout << "13. Alineacion y Rotacion" << endl;
    cout << "14. Club y Finanzas" << endl;
    cout << "15. Ver Logros" << endl;
    cout << "16. Guardar Carrera" << endl;
    cout << "17. Retirar Jugador" << endl;
    cout << "18. Plan de Entrenamiento" << endl;
    cout << "19. Editar Equipo" << endl;
    cout << "20. Volver al Menu Principal" << endl;
}

void viewTeam(Team& team) {
    cout << "\nEquipo: " << team.name << endl;
    cout << "Tacticas: " << team.tactics << ", Formacion: " << team.formation
         << ", Plan: " << team.trainingFocus << ", Instruccion: " << team.matchInstruction
         << ", Presupuesto: $" << team.budget << endl;
    cout << "Presion " << team.pressingIntensity << " | Linea " << team.defensiveLine
         << " | Tempo " << team.tempo << " | Amplitud " << team.width
         << " | Marcaje " << team.markingStyle << " | Rotacion " << team.rotationPolicy << endl;
    cout << "Capitan: " << (team.captain.empty() ? "No definido" : team.captain)
         << " | Penales: " << (team.penaltyTaker.empty() ? "No definido" : team.penaltyTaker)
         << " | Tiros libres: " << (team.freeKickTaker.empty() ? "No definido" : team.freeKickTaker)
         << " | Corners: " << (team.cornerTaker.empty() ? "No definido" : team.cornerTaker) << endl;
    cout << "Staff A/F/S/J/M: " << team.assistantCoach << "/" << team.fitnessCoach << "/" << team.scoutingChief
         << "/" << team.youthCoach << "/" << team.medicalTeam
         << " | Region juvenil: " << team.youthRegion << endl;
    cout << "Moral: " << team.morale << " | Temporada: " << team.wins << "G-" << team.draws << "E-" << team.losses << "P"
         << " | GF " << team.goalsFor << " / GA " << team.goalsAgainst << " | Pts " << team.points << endl;
    if (team.players.empty()) {
        cout << "No hay jugadores en el equipo." << endl;
        return;
    }
    int injuredCount = 0;
    int suspendedCount = 0;
    int lowFitCount = 0;
    int totalFit = 0;
    for (const auto& p : team.players) {
        if (p.injured) injuredCount++;
        if (p.matchesSuspended > 0) suspendedCount++;
        if (p.fitness < 60) lowFitCount++;
        totalFit += p.fitness;
    }
    int avgFit = team.players.empty() ? 0 : totalFit / static_cast<int>(team.players.size());
    if (injuredCount >= 3 || suspendedCount >= 2 || avgFit < 65) {
        cout << "[AVISO] Plantel con baja condicion: " << injuredCount << " lesionados, "
             << suspendedCount << " suspendidos, "
             << lowFitCount << " con condicion <60. Promedio: " << avgFit << endl;
    }

    auto xi = team.getStartingXIIndices();
    vector<bool> isXI(team.players.size(), false);
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(isXI.size())) isXI[idx] = true;
    }
    cout << "\nXI Titular:" << endl;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const auto& p = team.players[idx];
        cout << "- " << p.name << " (" << p.position << ")"
             << (p.injured ? " [LES]" : "") << " - Habilidad: " << p.skill
             << (p.matchesSuspended > 0 ? " [SUSP]" : "") << ", Condicion: " << p.fitness << endl;
    }

    auto bench = team.getBenchIndices();
    if (!bench.empty()) {
        cout << "\nBanca:" << endl;
        for (int idx : bench) {
            if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
            const auto& p = team.players[idx];
            cout << "- " << p.name << " (" << p.position << ")"
                 << (p.injured ? " [LES]" : "")
                 << (p.matchesSuspended > 0 ? " [SUSP]" : "")
                 << " Hab " << p.skill << " Cond " << p.fitness << endl;
        }
    }

    cout << "\nPlantel (ordenado por posicion):" << endl;
    vector<string> order = {"ARQ", "DEF", "MED", "DEL", "N/A"};
    for (const auto& pos : order) {
        vector<int> idxs;
        for (size_t i = 0; i < team.players.size(); ++i) {
            string norm = normalizePosition(team.players[i].position);
            if (norm == pos || (pos == "N/A" && norm == "N/A")) idxs.push_back(static_cast<int>(i));
        }
        if (idxs.empty()) continue;
        sort(idxs.begin(), idxs.end(), [&](int a, int b) {
            if (team.players[a].skill != team.players[b].skill) return team.players[a].skill > team.players[b].skill;
            return team.players[a].fitness > team.players[b].fitness;
        });
        cout << pos << ":" << endl;
        for (int i : idxs) {
            const auto& p = team.players[i];
            cout << i + 1 << ". " << p.name << (isXI[i] ? " [XI]" : "")
                 << (p.injured ? " [LES]" : "") << " - Atq " << p.attack
                 << (p.matchesSuspended > 0 ? (string(" [SUSP ") + to_string(p.matchesSuspended) + "]") : "")
                 << ", Def " << p.defense << ", Res " << p.stamina
                 << ", Cond " << p.fitness << ", Hab " << p.skill << ", Pot " << p.potential
                 << ", Rol " << p.role << ", Edad " << p.age << ", Valor $" << p.value
                 << ", Salario $" << p.wage << ", Clausula $" << p.releaseClause
                 << ", Contrato " << p.contractWeeks << " sem"
                 << ", Pie " << p.preferredFoot
                 << ", Sec " << (p.secondaryPositions.empty() ? string("-") : joinStringValues(p.secondaryPositions, "/"))
                 << ", Forma " << playerFormLabel(p) << " (" << p.currentForm << ")"
                 << ", Fiabilidad " << playerReliabilityLabel(p)
                 << ", Partidos grandes " << p.bigMatches
                 << ", TA " << p.seasonYellowCards << ", TR " << p.seasonRedCards
                 << ", Fel " << p.happiness << ", Quim " << p.chemistry
                 << ", Lider " << p.leadership << ", Profesionalismo " << p.professionalism
                 << ", Disciplina tactica " << p.tacticalDiscipline
                 << ", Plan " << p.developmentPlan << ", Promesa " << p.promisedRole
                 << ", Rasgos " << joinStringValues(p.traits, ", ")
                 << (p.wantsToLeave ? ", Quiere salir" : "")
                 << (p.onLoan ? ", Prestamo desde " + p.parentClub + " (" + to_string(p.loanWeeksRemaining) + " sem)" : "")
                 << endl;
        }
    }
}

void addPlayer(Team& team) {
    Player p;
    p.name = readLine("Nombre del jugador: ");
    p.position = normalizePosition(readLine("Posicion (ARQ/DEF/MED/DEL): "));
    if (p.position == "N/A") p.position = "MED";
    p.attack = readInt("Ataque (0-100): ", 0, 100);
    p.defense = readInt("Defensa (0-100): ", 0, 100);
    p.stamina = readInt("Resistencia (0-100): ", 0, 100);
    p.fitness = p.stamina;
    p.skill = readInt("Habilidad (0-100): ", 0, 100);
    p.potential = clampInt(p.skill + randInt(0, 5), p.skill, 95);
    p.age = readInt("Edad: ", 15, 50);
    p.value = readLongLong("Valor: ", 0, 1000000000000LL);
    p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 600);
    p.releaseClause = max(50000LL, p.value * 2);
    p.setPieceSkill = clampInt(p.skill + randInt(-8, 8), 20, 99);
    p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
    p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
    p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    p.desiredStarts = (p.skill >= 70) ? 2 : 1;
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
    ensurePlayerProfile(p, true);
    team.addPlayer(p);
    cout << "Jugador agregado!" << endl;
}

static int trainingDeltaForStat(int stat) {
    int roll = randInt(1, 5);
    if (stat >= 90) return 1;
    if (stat >= 80) return min(roll, 2);
    if (stat >= 70) return min(roll, 3);
    return roll;
}

void trainPlayer(Team& team, int season, int week) {
    if (team.players.empty()) {
        cout << "No hay jugadores para entrenar." << endl;
        return;
    }
    cout << "Selecciona jugador para entrenar:" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        cout << i + 1 << ". " << team.players[i].name << (team.players[i].injured ? " (Lesionado)" : "") << endl;
    }
    int playerIndex = readInt("Elige jugador: ", 1, static_cast<int>(team.players.size()));
    Player& p = team.players[playerIndex - 1];
    if (p.injured) {
        cout << "No puedes entrenar a un jugador lesionado." << endl;
        return;
    }
    if (season >= 0 && week >= 0 && p.lastTrainedSeason == season && p.lastTrainedWeek == week) {
        cout << "Este jugador ya entreno esta semana." << endl;
        return;
    }
    cout << "Que quieres entrenar?" << endl;
    cout << "1. Ataque" << endl;
    cout << "2. Defensa" << endl;
    cout << "3. Resistencia" << endl;
    cout << "4. Habilidad" << endl;
    int trainChoice = readInt("Elige opcion: ", 1, 4);
    long long cost = 5000;
    if (team.budget < cost) {
        cout << "Presupuesto insuficiente para entrenar." << endl;
        return;
    }
    team.budget -= cost;
    int improvement = 1;
    switch (trainChoice) {
        case 1:
            improvement = trainingDeltaForStat(p.attack);
            p.attack = min(100, p.attack + improvement);
            cout << "Ataque mejorado a " << p.attack << endl;
            break;
        case 2:
            improvement = trainingDeltaForStat(p.defense);
            p.defense = min(100, p.defense + improvement);
            cout << "Defensa mejorada a " << p.defense << endl;
            break;
        case 3:
            improvement = trainingDeltaForStat(p.stamina);
            p.stamina = min(100, p.stamina + improvement);
            p.fitness = min(p.stamina, p.fitness + improvement);
            cout << "Resistencia mejorada a " << p.stamina << endl;
            break;
        case 4:
            improvement = trainingDeltaForStat(p.skill);
            p.skill = min(100, p.skill + improvement);
            if (p.skill > p.potential) p.potential = p.skill;
            cout << "Habilidad mejorada a " << p.skill << endl;
            break;
        default:
            break;
    }
    if (season >= 0 && week >= 0) {
        p.lastTrainedSeason = season;
        p.lastTrainedWeek = week;
    }
}

void changeTactics(Team& team) {
    cout << "Tacticas actuales: " << team.tactics << endl;
    cout << "1. Defensive" << endl;
    cout << "2. Balanced" << endl;
    cout << "3. Offensive" << endl;
    cout << "4. Pressing" << endl;
    cout << "5. Counter" << endl;
    int tacticsChoice = readInt("Elige tactica: ", 1, 5);
    switch (tacticsChoice) {
        case 1: team.tactics = "Defensive"; break;
        case 2: team.tactics = "Balanced"; break;
        case 3: team.tactics = "Offensive"; break;
        case 4: team.tactics = "Pressing"; break;
        case 5: team.tactics = "Counter"; break;
        default: break;
    }
    cout << "Intensidad de presion actual: " << team.pressingIntensity << " (1-5)" << endl;
    team.pressingIntensity = readInt("Nueva intensidad de presion: ", 1, 5);
    cout << "Linea defensiva actual: " << team.defensiveLine << " (1-5)" << endl;
    team.defensiveLine = readInt("Nueva linea defensiva: ", 1, 5);
    cout << "Tempo actual: " << team.tempo << " (1-5)" << endl;
    team.tempo = readInt("Nuevo tempo: ", 1, 5);
    cout << "Amplitud actual: " << team.width << " (1-5)" << endl;
    team.width = readInt("Nueva amplitud: ", 1, 5);
    cout << "Marcaje actual: " << team.markingStyle << endl;
    cout << "1. Zonal" << endl;
    cout << "2. Hombre" << endl;
    int marking = readInt("Tipo de marcaje: ", 1, 2);
    team.markingStyle = (marking == 2) ? "Hombre" : "Zonal";
    cout << "Instruccion de partido actual: " << team.matchInstruction << endl;
    cout << "1. Equilibrado" << endl;
    cout << "2. Laterales altos" << endl;
    cout << "3. Bloque bajo" << endl;
    cout << "4. Balon parado" << endl;
    cout << "5. Presion final" << endl;
    cout << "6. Por bandas" << endl;
    cout << "7. Juego directo" << endl;
    cout << "8. Contra-presion" << endl;
    cout << "9. Pausar juego" << endl;
    int instruction = readInt("Nueva instruccion: ", 1, 9);
    if (instruction == 2) team.matchInstruction = "Laterales altos";
    else if (instruction == 3) team.matchInstruction = "Bloque bajo";
    else if (instruction == 4) team.matchInstruction = "Balon parado";
    else if (instruction == 5) team.matchInstruction = "Presion final";
    else if (instruction == 6) team.matchInstruction = "Por bandas";
    else if (instruction == 7) team.matchInstruction = "Juego directo";
    else if (instruction == 8) team.matchInstruction = "Contra-presion";
    else if (instruction == 9) team.matchInstruction = "Pausar juego";
    else team.matchInstruction = "Equilibrado";
    cout << "Tacticas cambiadas a " << team.tactics << endl;
}

void manageLineup(Team& team) {
    if (team.players.empty()) {
        cout << "No hay jugadores disponibles." << endl;
        return;
    }
    auto selectNamedList = [&](vector<string>& target, int maxCount, bool excludePreferredXI) {
        vector<string> chosen;
        for (size_t i = 0; i < team.players.size(); ++i) {
            const Player& p = team.players[i];
            bool inPreferredXI = find(team.preferredXI.begin(), team.preferredXI.end(), p.name) != team.preferredXI.end();
            if (excludePreferredXI && inPreferredXI) continue;
            cout << i + 1 << ". " << p.name << " (" << p.position << ")"
                 << (p.injured ? " [LES]" : "")
                 << (p.matchesSuspended > 0 ? " [SUSP]" : "")
                 << " Hab " << p.skill << " Cond " << p.fitness << endl;
        }
        while (static_cast<int>(chosen.size()) < maxCount) {
            int idx = readInt("Jugador (0 para terminar): ", 0, static_cast<int>(team.players.size()));
            if (idx == 0) break;
            const Player& p = team.players[idx - 1];
            if (excludePreferredXI &&
                find(team.preferredXI.begin(), team.preferredXI.end(), p.name) != team.preferredXI.end()) {
                cout << "Ese jugador ya esta en los titulares preferidos." << endl;
                continue;
            }
            if (find(chosen.begin(), chosen.end(), p.name) != chosen.end()) {
                cout << "Ya fue elegido." << endl;
                continue;
            }
            chosen.push_back(p.name);
        }
        target = chosen;
    };

    auto selectSinglePlayer = [&](const string& prompt, string& target) {
        for (size_t i = 0; i < team.players.size(); ++i) {
            cout << i + 1 << ". " << team.players[i].name << " (" << team.players[i].position << ")" << endl;
        }
        int idx = readInt(prompt, 1, static_cast<int>(team.players.size())) - 1;
        target = team.players[idx].name;
    };

    while (true) {
        cout << "\n=== Alineacion y Rotacion ===" << endl;
        cout << "Capitan: " << (team.captain.empty() ? "No definido" : team.captain) << endl;
        cout << "Penales: " << (team.penaltyTaker.empty() ? "No definido" : team.penaltyTaker) << endl;
        cout << "Tiros libres: " << (team.freeKickTaker.empty() ? "No definido" : team.freeKickTaker)
             << " | Corners: " << (team.cornerTaker.empty() ? "No definido" : team.cornerTaker) << endl;
        cout << "Rotacion: " << team.rotationPolicy << endl;
        cout << "Titulares preferidos: " << team.preferredXI.size() << "/11" << endl;
        cout << "Banca preferida: " << team.preferredBench.size() << "/7" << endl;
        cout << "1. Autoasignar mejor XI" << endl;
        cout << "2. Elegir titulares manualmente" << endl;
        cout << "3. Autoasignar banca" << endl;
        cout << "4. Elegir banca manualmente" << endl;
        cout << "5. Elegir capitan" << endl;
        cout << "6. Elegir lanzador de penales" << endl;
        cout << "7. Elegir lanzador de tiros libres" << endl;
        cout << "8. Elegir lanzador de corners" << endl;
        cout << "9. Politica de rotacion" << endl;
        cout << "10. Ver plan actual" << endl;
        cout << "11. Volver" << endl;
        int choice = readInt("Elige opcion: ", 1, 11);
        if (choice == 11) break;

        if (choice == 1) {
            team.preferredXI.clear();
            auto xi = team.getStartingXIIndices();
            for (int idx : xi) {
                if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                    team.preferredXI.push_back(team.players[idx].name);
                }
            }
            cout << "XI preferido actualizado automaticamente." << endl;
        } else if (choice == 2) {
            selectNamedList(team.preferredXI, 11, false);
            if (!team.preferredXI.empty()) {
                cout << "Titulares preferidos actualizados." << endl;
            }
        } else if (choice == 3) {
            team.preferredBench.clear();
            auto bench = team.getBenchIndices();
            for (int idx : bench) {
                if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                    team.preferredBench.push_back(team.players[idx].name);
                }
            }
            cout << "Banca preferida actualizada automaticamente." << endl;
        } else if (choice == 4) {
            selectNamedList(team.preferredBench, 7, true);
            cout << "Banca preferida actualizada." << endl;
        } else if (choice == 5) {
            selectSinglePlayer("Jugador: ", team.captain);
            cout << "Capitan actualizado." << endl;
        } else if (choice == 6) {
            selectSinglePlayer("Jugador: ", team.penaltyTaker);
            cout << "Lanzador de penales actualizado." << endl;
        } else if (choice == 7) {
            selectSinglePlayer("Jugador: ", team.freeKickTaker);
            cout << "Lanzador de tiros libres actualizado." << endl;
        } else if (choice == 8) {
            selectSinglePlayer("Jugador: ", team.cornerTaker);
            cout << "Lanzador de corners actualizado." << endl;
        } else if (choice == 9) {
            cout << "1. Titulares" << endl;
            cout << "2. Balanceado" << endl;
            cout << "3. Rotacion" << endl;
            int mode = readInt("Politica: ", 1, 3);
            if (mode == 1) team.rotationPolicy = "Titulares";
            else if (mode == 2) team.rotationPolicy = "Balanceado";
            else team.rotationPolicy = "Rotacion";
            cout << "Politica actualizada." << endl;
        } else if (choice == 10) {
            auto xi = team.getStartingXIIndices();
            cout << "XI actual:" << endl;
            for (int idx : xi) {
                if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                    cout << "- " << team.players[idx].name << " (" << team.players[idx].position << ")" << endl;
                }
            }
            auto bench = team.getBenchIndices();
            if (!bench.empty()) {
                cout << "Banca actual:" << endl;
                for (int idx : bench) {
                    if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                        cout << "- " << team.players[idx].name << " (" << team.players[idx].position << ")" << endl;
                    }
                }
            }
        }
    }
}

void setTrainingPlan(Team& team) {
    cout << "\nPlan de entrenamiento actual: " << team.trainingFocus << endl;
    cout << "1. Balanceado" << endl;
    cout << "2. Fisico" << endl;
    cout << "3. Tecnico" << endl;
    cout << "4. Tactico" << endl;
    cout << "5. Preparacion partido" << endl;
    cout << "6. Recuperacion" << endl;
    int choice = readInt("Elige plan: ", 1, 6);
    switch (choice) {
        case 1: team.trainingFocus = "Balanceado"; break;
        case 2: team.trainingFocus = "Fisico"; break;
        case 3: team.trainingFocus = "Tecnico"; break;
        case 4: team.trainingFocus = "Tactico"; break;
        case 5: team.trainingFocus = "Preparacion partido"; break;
        case 6: team.trainingFocus = "Recuperacion"; break;
        default: break;
    }
    cout << "Plan actualizado a " << team.trainingFocus << endl;
}

static void applyTrainingPlan(Team& team) {
    string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    int facilityBonus = max(0, team.trainingFacilityLevel - 1);
    int assistantBonus = max(0, team.assistantCoach - 55) / 15;
    int fitnessBonus = max(0, team.fitnessCoach - 55) / 15;
    if (focus == "Fisico") {
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 2 + facilityBonus + fitnessBonus, 15, p.stamina);
            if (randInt(1, 100) <= 12 + facilityBonus * 2 + fitnessBonus * 2) {
                p.stamina = min(100, p.stamina + 1);
                if (p.fitness > p.stamina) p.fitness = p.stamina;
            }
        }
    } else if (focus == "Tecnico") {
        for (auto& p : team.players) {
            if (p.injured) continue;
            if (p.skill >= p.potential) continue;
            if (randInt(1, 100) <= 18 + facilityBonus * 2 + assistantBonus * 2) {
                p.skill = min(100, p.skill + 1);
                string pos = normalizePosition(p.position);
                if (pos == "ARQ" || pos == "DEF") p.defense = min(100, p.defense + 1);
                else if (pos == "MED") {
                    p.attack = min(100, p.attack + 1);
                    p.defense = min(100, p.defense + 1);
                } else {
                    p.attack = min(100, p.attack + 1);
                }
            }
        }
    } else if (focus == "Tactico") {
        team.morale = clampInt(team.morale + 2 + assistantBonus, 0, 100);
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 1 + facilityBonus + fitnessBonus, 15, p.stamina);
            p.tacticalDiscipline = clampInt(p.tacticalDiscipline + 1, 1, 99);
        }
    } else if (focus == "Preparacion partido") {
        team.morale = clampInt(team.morale + 1 + assistantBonus, 0, 100);
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 1 + facilityBonus, 15, p.stamina);
            p.currentForm = clampInt(p.currentForm + 1 + assistantBonus / 2, 1, 99);
            p.chemistry = clampInt(p.chemistry + 1, 1, 99);
        }
    } else if (focus == "Recuperacion") {
        for (auto& p : team.players) {
            if (p.injured) {
                p.injuryWeeks = max(0, p.injuryWeeks - 1);
                if (p.injuryWeeks == 0) {
                    p.injured = false;
                    p.injuryType.clear();
                }
            }
            p.fitness = clampInt(p.fitness + 3 + facilityBonus + fitnessBonus, 15, p.stamina);
            if (p.fitness >= p.stamina - 2) {
                p.currentForm = clampInt(p.currentForm + 1, 1, 99);
            }
        }
    } else {
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 1 + facilityBonus + fitnessBonus, 15, p.stamina);
        }
    }

    int planChance = clampInt(10 + facilityBonus * 2 + assistantBonus, 6, 28);
    for (auto& p : team.players) {
        if (p.injured) continue;
        if (p.developmentPlan == "Fisico") {
            p.fitness = clampInt(p.fitness + 1, 15, p.stamina);
            if (randInt(1, 100) <= planChance) p.stamina = min(100, p.stamina + 1);
        } else if (p.developmentPlan == "Defensa" || p.developmentPlan == "Reflejos") {
            if (randInt(1, 100) <= planChance) p.defense = min(100, p.defense + 1);
        } else if (p.developmentPlan == "Creatividad") {
            if (randInt(1, 100) <= planChance) {
                p.attack = min(100, p.attack + 1);
                p.setPieceSkill = min(99, p.setPieceSkill + 1);
            }
        } else if (p.developmentPlan == "Finalizacion") {
            if (randInt(1, 100) <= planChance) p.attack = min(100, p.attack + 1);
        } else if (p.developmentPlan == "Liderazgo") {
            if (randInt(1, 100) <= max(8, planChance - 2)) {
                p.leadership = min(99, p.leadership + 1);
                if (randInt(1, 100) <= 40) p.professionalism = min(99, p.professionalism + 1);
            }
        }
    }
}

void editTeam(Team& team) {
    while (true) {
        cout << "\n=== Editor Rapido de Equipo ===" << endl;
        cout << "1. Renombrar equipo" << endl;
        cout << "2. Cambiar formacion" << endl;
        cout << "3. Editar jugador" << endl;
        cout << "4. Eliminar jugador" << endl;
        cout << "5. Volver" << endl;
        int choice = readInt("Elige una opcion: ", 1, 5);
        if (choice == 5) break;

        if (choice == 1) {
            string name = readLine("Nuevo nombre del equipo: ");
            if (!name.empty()) team.name = name;
            cout << "Nombre actualizado." << endl;
        } else if (choice == 2) {
            string form = readLine("Nueva formacion (ej: 4-4-2): ");
            if (!form.empty()) team.formation = form;
            cout << "Formacion actualizada." << endl;
        } else if (choice == 3) {
            if (team.players.empty()) {
                cout << "No hay jugadores para editar." << endl;
                continue;
            }
            cout << "Selecciona jugador:" << endl;
            for (size_t i = 0; i < team.players.size(); ++i) {
                cout << i + 1 << ". " << team.players[i].name << " (" << team.players[i].position << ")" << endl;
            }
            int idx = readInt("Jugador: ", 1, static_cast<int>(team.players.size())) - 1;
            Player& p = team.players[idx];
            cout << "1. Nombre" << endl;
            cout << "2. Posicion" << endl;
            cout << "3. Ataque" << endl;
            cout << "4. Defensa" << endl;
            cout << "5. Resistencia" << endl;
            cout << "6. Habilidad" << endl;
            cout << "7. Edad" << endl;
            cout << "8. Valor" << endl;
            cout << "9. Condicion" << endl;
            cout << "10. Potencial" << endl;
            cout << "11. Rol" << endl;
            cout << "12. Salario" << endl;
            cout << "13. Contrato (semanas)" << endl;
            cout << "14. Volver" << endl;
            int field = readInt("Campo a editar: ", 1, 14);
            if (field == 14) continue;
            switch (field) {
                case 1: {
                    string v = readLine("Nuevo nombre: ");
                    if (!v.empty()) p.name = v;
                    break;
                }
                case 2: {
                    string v = readLine("Nueva posicion: ");
                    string pos = normalizePosition(v);
                    p.position = (pos == "N/A") ? "MED" : pos;
                    break;
                }
                case 3:
                    p.attack = readInt("Nuevo ataque (0-100): ", 0, 100);
                    break;
                case 4:
                    p.defense = readInt("Nueva defensa (0-100): ", 0, 100);
                    break;
                case 5:
                    p.stamina = readInt("Nueva resistencia (0-100): ", 0, 100);
                    if (p.fitness > p.stamina) p.fitness = p.stamina;
                    break;
                case 6:
                    p.skill = readInt("Nueva habilidad (0-100): ", 0, 100);
                    if (p.potential < p.skill) p.potential = p.skill;
                    break;
                case 7:
                    p.age = readInt("Nueva edad: ", 15, 50);
                    break;
                case 8:
                    p.value = readLongLong("Nuevo valor: ", 0, 1000000000000LL);
                    break;
                case 9:
                    p.fitness = readInt("Nueva condicion (0-100): ", 0, 100);
                    if (p.fitness > p.stamina) p.fitness = p.stamina;
                    break;
                case 10:
                    p.potential = readInt("Nuevo potencial (0-100): ", 0, 100);
                    if (p.potential < p.skill) p.potential = p.skill;
                    break;
                case 11: {
                    string v = readLine("Nuevo rol: ");
                    if (!v.empty()) p.role = v;
                    break;
                }
                case 12:
                    p.wage = readLongLong("Nuevo salario: ", 0, 1000000000000LL);
                    break;
                case 13:
                    p.contractWeeks = readInt("Nuevo contrato (semanas): ", 0, 520);
                    break;
                default:
                    break;
            }
            cout << "Jugador actualizado." << endl;
        } else if (choice == 4) {
            if (team.players.size() <= 11) {
                cout << "No puedes dejar al equipo con menos de 11 jugadores." << endl;
                continue;
            }
            cout << "Selecciona jugador para eliminar:" << endl;
            for (size_t i = 0; i < team.players.size(); ++i) {
                cout << i + 1 << ". " << team.players[i].name << endl;
            }
            int idx = readInt("Jugador: ", 1, static_cast<int>(team.players.size())) - 1;
            cout << team.players[idx].name << " eliminado." << endl;
            team.players.erase(team.players.begin() + idx);
        }
    }
}

void displayStatistics(Team& team) {
    cout << "\n--- Estadisticas del Equipo ---" << endl;
    cout << "Moral: " << team.morale << endl;
    cout << "Victorias: " << team.wins << endl;
    cout << "Empates: " << team.draws << endl;
    cout << "Derrotas: " << team.losses << endl;
    cout << "Goles a Favor: " << team.goalsFor << endl;
    cout << "Goles en Contra: " << team.goalsAgainst << endl;
    cout << "Goles de Visita: " << team.awayGoals << endl;
    cout << "Tarjetas Amarillas: " << team.yellowCards << endl;
    cout << "Tarjetas Rojas: " << team.redCards << endl;
    cout << "Puntos: " << team.points << endl;
    cout << "\n--- Estadisticas de Jugadores ---" << endl;
    for (const auto& p : team.players) {
        cout << p.name << ": Goles " << p.goals
             << ", Asistencias " << p.assists
             << ", Partidos " << p.matchesPlayed
             << ", TA " << p.seasonYellowCards
             << ", TR " << p.seasonRedCards
             << ", Susp " << p.matchesSuspended
             << ", Tit " << p.startsThisSeason
             << ", Fel " << p.happiness
             << ", Quim " << p.chemistry
             << (p.wantsToLeave ? ", Quiere salir" : "")
             << endl;
    }
}

void displayAchievementsMenu(Career& career) {
    cout << "\n=== Logros ===" << endl;
    if (career.achievements.empty()) {
        cout << "No hay logros desbloqueados." << endl;
        return;
    }
    for (size_t i = 0; i < career.achievements.size(); ++i) {
        cout << i + 1 << ". " << career.achievements[i] << endl;
    }
    int choice = readInt("Resetear logros? (1. Si, 2. No): ", 1, 2);
    if (choice == 1) {
        career.achievements.clear();
        cout << "Logros reiniciados." << endl;
    }
}

static bool usesGroupFormat(const Career& career) {
    return career.usesGroupFormat();
}

static LeagueTable buildTableFromTeams(const vector<Team*>& teams, const string& title, const string& ruleId) {
    LeagueTable table;
    table.title = title;
    table.ruleId = ruleId;
    for (auto* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

static LeagueTable buildGroupTable(const Career& career, const vector<int>& idx, const string& title) {
    vector<Team*> teams;
    for (int i : idx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size())) {
            teams.push_back(career.activeTeams[i]);
        }
    }
    return buildTableFromTeams(teams, title, career.activeDivision);
}

static int groupForTeam(const Career& career, const Team* team) {
    for (int i : career.groupNorthIdx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size()) && career.activeTeams[i] == team) return 0;
    }
    for (int i : career.groupSouthIdx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size()) && career.activeTeams[i] == team) return 1;
    }
    return -1;
}

static string regionalGroupTitle(const Career& career, bool north) {
    return competitionGroupTitle(career.activeDivision, north);
}

void displayLeagueTables(Career& career) {
    if (!usesGroupFormat(career) || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.leagueTable.displayTable();
        return;
    }
    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, regionalGroupTitle(career, true));
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, regionalGroupTitle(career, false));
    north.displayTable();
    south.displayTable();
}

static LeagueTable relevantCompetitionTable(const Career& career) {
    if (!career.myTeam) return career.leagueTable;
    if (!usesGroupFormat(career) || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }
    int group = groupForTeam(career, career.myTeam);
    if (group == 0) return buildGroupTable(career, career.groupNorthIdx, regionalGroupTitle(career, true));
    if (group == 1) return buildGroupTable(career, career.groupSouthIdx, regionalGroupTitle(career, false));
    LeagueTable table = career.leagueTable;
    table.sortTable();
    return table;
}

static void displayTableSlice(const LeagueTable& table, int start, int count) {
    int end = min(static_cast<int>(table.teams.size()), start + count);
    for (int i = start; i < end; ++i) {
        Team* team = table.teams[i];
        int gd = team->goalsFor - team->goalsAgainst;
        cout << setw(2) << i + 1 << ". " << left << setw(22) << team->name.substr(0, 22)
             << right << " Pts " << setw(3) << team->points
             << " DG " << setw(3) << gd
             << " G " << team->wins << endl;
    }
}

static string boardStatusLabel(int confidence) {
    if (confidence >= 75) return "Muy alta";
    if (confidence >= 55) return "Estable";
    if (confidence >= 35) return "En observacion";
    if (confidence >= 20) return "Bajo presion";
    return "Critica";
}

void displayCompetitionCenter(Career& career) {
    if (!career.myTeam) return;
    career.leagueTable.sortTable();
    LeagueTable table = relevantCompetitionTable(career);
    int rank = career.currentCompetitiveRank();
    int totalWeeks = static_cast<int>(career.schedule.size());
    int remaining = max(0, totalWeeks - career.currentWeek + 1);

    cout << "\n=== Centro de Competicion ===" << endl;
    cout << "Division: " << divisionDisplay(career.activeDivision)
         << " | Temporada " << career.currentSeason
         << " | Fecha " << career.currentWeek << "/" << totalWeeks << endl;
    cout << "Equipo: " << career.myTeam->name
         << " | Posicion: " << rank << "/" << max(1, career.currentCompetitiveFieldSize())
         << " | Pts: " << career.myTeam->points
         << " | DG: " << (career.myTeam->goalsFor - career.myTeam->goalsAgainst) << endl;
    cout << "Confianza directiva: " << boardStatusLabel(career.boardConfidence)
         << " (" << career.boardConfidence << "/100)" << endl;
    cout << "Fechas por jugar: " << remaining << endl;
    if (!career.boardMonthlyObjective.empty()) {
        int weeksLeft = max(0, career.boardMonthlyDeadlineWeek - career.currentWeek);
        cout << "Objetivo mensual: " << career.boardMonthlyObjective
             << " | Progreso " << career.boardMonthlyProgress << "/" << career.boardMonthlyTarget
             << " | Cierre en " << weeksLeft << " semana(s)" << endl;
    }
    if (!career.cupChampion.empty()) {
        cout << "Copa de temporada: campeon " << career.cupChampion << endl;
    } else if (career.cupActive) {
        cout << "Copa de temporada: ronda " << career.cupRound + 1
             << " | Equipos vivos " << career.cupRemainingTeams.size() << endl;
    }

    cout << "\nZona alta:" << endl;
    displayTableSlice(table, 0, min(5, static_cast<int>(table.teams.size())));
    if (static_cast<int>(table.teams.size()) > 5) {
        cout << "\nZona baja:" << endl;
        displayTableSlice(table, max(0, static_cast<int>(table.teams.size()) - 3), 3);
    }

    if (career.currentWeek <= static_cast<int>(career.schedule.size())) {
        cout << "\nProxima fecha:" << endl;
        const auto& matches = career.schedule[career.currentWeek - 1];
        for (const auto& match : matches) {
            Team* home = career.activeTeams[match.first];
            Team* away = career.activeTeams[match.second];
            string marker = (home == career.myTeam || away == career.myTeam) ? " <- tu partido" : "";
            cout << "- " << home->name << " vs " << away->name << marker << endl;
        }
    }

    if (!career.newsFeed.empty()) {
        cout << "\nUltimas noticias:" << endl;
        int start = max(0, static_cast<int>(career.newsFeed.size()) - 3);
        for (size_t i = static_cast<size_t>(start); i < career.newsFeed.size(); ++i) {
            cout << "- " << career.newsFeed[i] << endl;
        }
    }
    if (!career.lastMatchAnalysis.empty()) {
        cout << "\nUltimo analisis:" << endl;
        cout << career.lastMatchAnalysis << endl;
    }
}

static vector<Team*> buildJobMarket(Career& career, bool emergency) {
    vector<Team*> jobs;
    for (auto& team : career.allTeams) {
        if (&team == career.myTeam) continue;
        int strength = team.getAverageSkill();
        if (emergency) {
            if (strength <= career.managerReputation + 15) jobs.push_back(&team);
        } else if (strength <= career.managerReputation + 25 || team.division == career.myTeam->division) {
            jobs.push_back(&team);
        }
    }
    sort(jobs.begin(), jobs.end(), [](Team* a, Team* b) {
        if (a->getSquadValue() != b->getSquadValue()) return a->getSquadValue() > b->getSquadValue();
        return a->name < b->name;
    });
    if (jobs.size() > 12) jobs.resize(12);
    return jobs;
}

static void takeManagerJob(Career& career, Team* newClub, const string& reason) {
    if (!newClub) return;
    string oldClub = career.myTeam ? career.myTeam->name : "Sin club";
    career.myTeam = newClub;
    career.setActiveDivision(newClub->division);
    career.initializeBoardObjectives();
    career.boardConfidence = clampInt(career.boardConfidence + 10, 35, 80);
    career.addNews(career.managerName + " deja " + oldClub + " y asume en " + newClub->name + ". " + reason);
}

void displayBoardStatus(Career& career) {
    if (!career.myTeam) return;
    int youthUsed = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.age <= 20 && player.matchesPlayed > 0) youthUsed++;
    }
    int rank = career.currentCompetitiveRank();

    cout << "\n=== Objetivos y Directiva ===" << endl;
    cout << "Confianza: " << boardStatusLabel(career.boardConfidence)
         << " (" << career.boardConfidence << "/100)" << endl;
    cout << "Rachas de advertencia: " << career.boardWarningWeeks << endl;
    cout << "\nObjetivos de temporada:" << endl;
    cout << "- Terminar en puesto " << career.boardExpectedFinish << " o mejor. Actual: " << rank << endl;
    cout << "- Cerrar con presupuesto >= $" << career.boardBudgetTarget
         << ". Actual: $" << career.myTeam->budget << endl;
    cout << "- Dar minutos a " << career.boardYouthTarget << " juvenil(es) sub-20. Actual: " << youthUsed << endl;
    cout << "- Reputacion del manager: " << career.managerReputation << "/100" << endl;
    if (!career.boardMonthlyObjective.empty()) {
        int weeksLeft = max(0, career.boardMonthlyDeadlineWeek - career.currentWeek);
        cout << "\nObjetivo mensual:" << endl;
        cout << "- " << career.boardMonthlyObjective << endl;
        cout << "- Progreso: " << career.boardMonthlyProgress << " / " << career.boardMonthlyTarget << endl;
        cout << "- Semanas restantes: " << weeksLeft << endl;
    }

    if (career.boardConfidence < 20) {
        cout << "\nEstado: la directiva considera un ultimatum deportivo." << endl;
    } else if (career.boardConfidence < 35) {
        cout << "\nEstado: el cargo esta bajo presion." << endl;
    } else {
        cout << "\nEstado: situacion controlada." << endl;
    }

    cout << "\n1. Buscar ofertas de club" << endl;
    cout << "2. Volver" << endl;
    int action = readInt("Elige opcion: ", 1, 2);
    if (action != 1) return;

    vector<Team*> jobs = buildJobMarket(career, false);
    if (jobs.empty()) {
        cout << "No hay ofertas adecuadas por ahora." << endl;
        return;
    }
    cout << "\nOfertas disponibles:" << endl;
    for (size_t i = 0; i < jobs.size(); ++i) {
        cout << i + 1 << ". " << jobs[i]->name << " (" << divisionDisplay(jobs[i]->division) << ")"
             << " Valor plantel $" << jobs[i]->getSquadValue() << endl;
    }
    int choice = readInt("Club (0 para cancelar): ", 0, static_cast<int>(jobs.size()));
    if (choice == 0) return;
    takeManagerJob(career, jobs[choice - 1], "Cambio de club voluntario.");
    cout << "Nuevo destino: " << career.myTeam->name << endl;
}

void displayNewsFeed(const Career& career) {
    cout << "\n=== Noticias ===" << endl;
    if (career.newsFeed.empty()) {
        cout << "No hay novedades registradas." << endl;
        return;
    }
    int start = max(0, static_cast<int>(career.newsFeed.size()) - 15);
    for (size_t i = static_cast<size_t>(start); i < career.newsFeed.size(); ++i) {
        cout << "- " << career.newsFeed[i] << endl;
    }
}

void displaySeasonHistory(const Career& career) {
    cout << "\n=== Historial de Temporadas ===" << endl;
    if (career.history.empty()) {
        cout << "Aun no hay temporadas finalizadas." << endl;
        return;
    }
    for (const auto& entry : career.history) {
        cout << "Temporada " << entry.season << " | " << divisionDisplay(entry.division)
             << " | Club " << entry.club << " | Puesto " << entry.finish << endl;
        cout << "  Campeon: " << entry.champion << endl;
        if (!entry.promoted.empty()) cout << "  Ascensos: " << entry.promoted << endl;
        if (!entry.relegated.empty()) cout << "  Descensos: " << entry.relegated << endl;
        if (!entry.note.empty()) cout << "  Nota: " << entry.note << endl;
    }
}

void displayClubOperations(Career& career) {
    if (!career.myTeam) return;
    Team& team = *career.myTeam;
    cout << "\n=== Club y Finanzas ===" << endl;
    cout << "Presupuesto: $" << team.budget << " | Deuda: $" << team.debt << endl;
    cout << "Sponsor semanal: $" << team.sponsorWeekly << " | Base de hinchas: " << team.fanBase << endl;
    cout << "Estadio: Nivel " << team.stadiumLevel
         << " | Juveniles: Nivel " << team.youthFacilityLevel
         << " | Entrenamiento: Nivel " << team.trainingFacilityLevel << endl;
    cout << "Staff A/F/S/J/M: " << team.assistantCoach << "/" << team.fitnessCoach << "/" << team.scoutingChief
         << "/" << team.youthCoach << "/" << team.medicalTeam << endl;
    cout << "Region juvenil actual: " << team.youthRegion << endl;
    cout << "1. Mejorar estadio" << endl;
    cout << "2. Mejorar cantera" << endl;
    cout << "3. Mejorar centro de entrenamiento" << endl;
    cout << "4. Contratar asistente tecnico" << endl;
    cout << "5. Contratar preparador fisico" << endl;
    cout << "6. Contratar jefe de scouting" << endl;
    cout << "7. Contratar jefe de juveniles" << endl;
    cout << "8. Mejorar cuerpo medico" << endl;
    cout << "9. Cambiar region juvenil" << endl;
    cout << "10. Volver" << endl;
    int choice = readInt("Elige opcion: ", 1, 10);
    if (choice == 10) return;

    long long cost = 0;
    if (choice == 1) cost = 60000LL * (team.stadiumLevel + 1);
    else if (choice == 2) cost = 50000LL * (team.youthFacilityLevel + 1);
    else if (choice == 3) cost = 55000LL * (team.trainingFacilityLevel + 1);
    else if (choice == 4) cost = 35000LL + static_cast<long long>(team.assistantCoach) * 1200LL;
    else if (choice == 5) cost = 32000LL + static_cast<long long>(team.fitnessCoach) * 1200LL;
    else if (choice == 6) cost = 36000LL + static_cast<long long>(team.scoutingChief) * 1300LL;
    else if (choice == 7) cost = 34000LL + static_cast<long long>(team.youthCoach) * 1200LL;
    else if (choice == 8) cost = 33000LL + static_cast<long long>(team.medicalTeam) * 1200LL;
    else cost = 12000LL;
    if (team.budget < cost) {
        cout << "Presupuesto insuficiente para la inversion." << endl;
        return;
    }
    team.budget -= cost;
    if (choice == 1) {
        team.stadiumLevel++;
        team.fanBase += 3;
        team.sponsorWeekly += 5000;
        career.addNews(team.name + " amplia su estadio.");
    } else if (choice == 2) {
        team.youthFacilityLevel++;
        career.addNews(team.name + " mejora su cantera.");
    } else if (choice == 3) {
        team.trainingFacilityLevel++;
        career.addNews(team.name + " mejora su centro de entrenamiento.");
    } else if (choice == 4) {
        team.assistantCoach = clampInt(team.assistantCoach + 5, 1, 99);
        career.addNews(team.name + " refuerza su cuerpo tecnico.");
    } else if (choice == 5) {
        team.fitnessCoach = clampInt(team.fitnessCoach + 5, 1, 99);
        career.addNews(team.name + " mejora su preparacion fisica.");
    } else if (choice == 6) {
        team.scoutingChief = clampInt(team.scoutingChief + 5, 1, 99);
        career.addNews(team.name + " fortalece su red de scouting.");
    } else if (choice == 7) {
        team.youthCoach = clampInt(team.youthCoach + 5, 1, 99);
        career.addNews(team.name + " mejora la conduccion de juveniles.");
    } else if (choice == 8) {
        team.medicalTeam = clampInt(team.medicalTeam + 5, 1, 99);
        career.addNews(team.name + " fortalece el cuerpo medico.");
    } else {
        vector<string> regions = {"Metropolitana", "Norte", "Centro", "Sur", "Patagonia"};
        for (size_t i = 0; i < regions.size(); ++i) {
            cout << i + 1 << ". " << regions[i] << endl;
        }
        int regionChoice = readInt("Nueva region juvenil: ", 1, static_cast<int>(regions.size()));
        team.youthRegion = regions[regionChoice - 1];
        career.addNews(team.name + " reorienta su captacion juvenil hacia " + team.youthRegion + ".");
    }
    cout << "Inversion completada por $" << cost << endl;
}

static int teamRank(const LeagueTable& table, const Team* team) {
    for (size_t i = 0; i < table.teams.size(); ++i) {
        if (table.teams[i] == team) return static_cast<int>(i) + 1;
    }
    return -1;
}

static bool isKeyMatch(const LeagueTable& table, const Team* home, const Team* away) {
    int rh = teamRank(table, home);
    int ra = teamRank(table, away);
    if (rh <= 0 || ra <= 0) return false;
    if (rh <= 3 || ra <= 3) return true;
    if (abs(rh - ra) <= 2) return true;
    return false;
}

static long long divisionBaseIncome(const string& division) {
    return getCompetitionConfig(division).baseIncome;
}

static int divisionWageFactor(const string& division) {
    return getCompetitionConfig(division).wageFactor;
}

static long long weeklyWage(const Team& team) {
    long long sum = 0;
    for (const auto& p : team.players) sum += p.wage;
    int factor = divisionWageFactor(team.division);
    return sum * factor / 100;
}

static void applyWeeklyFinances(Career& career, const vector<int>& pointsBefore) {
    unordered_map<Team*, int> homeGames;
    if (career.currentWeek >= 1 && career.currentWeek <= static_cast<int>(career.schedule.size())) {
        for (const auto& match : career.schedule[career.currentWeek - 1]) {
            homeGames[career.activeTeams[match.first]]++;
        }
    }
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        int pointsDelta = team->points - pointsBefore[i];
        if (pointsDelta >= 3) team->fanBase = clampInt(team->fanBase + 1, 10, 99);
        else if (pointsDelta == 0 && team->fanBase > 12 && randInt(1, 100) <= 30) team->fanBase--;
        long long ticketIncome = static_cast<long long>(homeGames[team]) * (team->fanBase * 2500LL + team->stadiumLevel * 7000LL);
        long long seasonTickets = (career.currentWeek % 4 == 1) ? team->fanBase * 900LL : 0LL;
        long long sponsor = team->sponsorWeekly + max(0, pointsDelta) * 800LL;
        long long performanceBonus = pointsDelta * 4000LL;
        long long solidarity = randInt(0, 3000);
        long long income = divisionBaseIncome(team->division) + sponsor + ticketIncome + seasonTickets + performanceBonus + solidarity;
        long long wages = weeklyWage(*team);
        long long debtPayment = min(team->debt, max(0LL, income / 8));
        team->debt -= debtPayment;
        long long debtInterest = max(0LL, team->debt / 250);
        long long infrastructure = (team->trainingFacilityLevel + team->youthFacilityLevel + team->stadiumLevel - 3) * 1500LL;
        long long net = income - wages - debtPayment - debtInterest - infrastructure;
        team->budget = max(0LL, team->budget + net);
        if (career.currentWeek % 8 == 0 && pointsDelta >= 3) {
            team->sponsorWeekly += max(500LL, team->fanBase * 30LL);
        }
        if (career.myTeam == team) {
            ostringstream out;
            out << "Finanzas semanales: +" << income
                << " (entradas " << ticketIncome << ", abonos " << seasonTickets << ", sponsor " << sponsor << ")"
                << " / -" << wages << " salarios"
                << " / -" << debtPayment << " deuda"
                << " / -" << debtInterest << " interes"
                << " / -" << infrastructure << " infraestructura"
                << " = " << net;
            emitUiMessage(out.str());
        }
    }
}

static void weeklyDashboard(const Career& career) {
    if (!career.myTeam) return;
    int rank = teamRank(career.leagueTable, career.myTeam);
    if (usesGroupFormat(career)) {
        int group = groupForTeam(career, career.myTeam);
        if (group == 0) {
            LeagueTable north = buildGroupTable(career, career.groupNorthIdx, regionalGroupTitle(career, true));
            rank = teamRank(north, career.myTeam);
        } else if (group == 1) {
            LeagueTable south = buildGroupTable(career, career.groupSouthIdx, regionalGroupTitle(career, false));
            rank = teamRank(south, career.myTeam);
        }
    }
    int injured = 0;
    int suspended = 0;
    int expiring = 0;
    for (const auto& p : career.myTeam->players) {
        if (p.injured) injured++;
        if (p.matchesSuspended > 0) suspended++;
        if (p.contractWeeks > 0 && p.contractWeeks <= 4) expiring++;
    }
    emitUiMessage("");
    emitUiMessage("--- Resumen Semanal ---");
    {
        ostringstream out;
        out << "Posicion: " << rank << " | Pts: " << career.myTeam->points
            << " | Moral: " << career.myTeam->morale
            << " | Presupuesto: $" << career.myTeam->budget
            << " | Directiva: " << career.boardConfidence << "/100";
        emitUiMessage(out.str());
    }
    {
        ostringstream out;
        out << "Sponsor: $" << career.myTeam->sponsorWeekly << " | Deuda: $" << career.myTeam->debt
            << " | Infraestructura E/C/T: "
            << career.myTeam->stadiumLevel << "/" << career.myTeam->youthFacilityLevel << "/" << career.myTeam->trainingFacilityLevel;
        emitUiMessage(out.str());
    }
    {
        ostringstream out;
        out << "Lesionados: " << injured
            << " | Suspendidos: " << suspended
            << " | Contratos por vencer (<=4 sem): " << expiring
            << " | Shortlist: " << career.scoutingShortlist.size();
        emitUiMessage(out.str());
    }
    if (!career.boardMonthlyObjective.empty()) {
        emitUiMessage("Objetivo mensual: " + career.boardMonthlyObjective +
                      " | " + to_string(career.boardMonthlyProgress) + "/" + to_string(career.boardMonthlyTarget));
    }
    if (!career.cupChampion.empty()) {
        emitUiMessage("Copa: campeon " + career.cupChampion);
    } else if (career.cupActive) {
        emitUiMessage("Copa: ronda " + to_string(career.cupRound + 1) +
                      " | vivos " + to_string(career.cupRemainingTeams.size()));
    }
}

static void applyClubEvent(Career& career) {
    if (!career.myTeam) return;
    if (randInt(1, 100) > 15) return;
    int event = randInt(1, 4);
    if (event == 1) {
        long long bonus = 50000 + randInt(0, 30000);
        career.myTeam->budget += bonus;
        career.addNews("Nuevo patrocinio para " + career.myTeam->name + " por $" + to_string(bonus) + ".");
        emitUiMessage("[Evento] Patrocinio sorpresa: +" + to_string(bonus));
    } else if (event == 2) {
        career.myTeam->morale = clampInt(career.myTeam->morale - 5, 0, 100);
        career.addNews("La hinchada presiona a " + career.myTeam->name + " tras los ultimos resultados.");
        emitUiMessage("[Evento] Protesta de hinchas: moral -5.");
    } else if (event == 3) {
        if (!career.myTeam->players.empty()) {
            int idx = randInt(0, static_cast<int>(career.myTeam->players.size()) - 1);
            Player& p = career.myTeam->players[idx];
            p.injured = true;
            p.injuryType = "Leve";
            p.injuryWeeks = randInt(1, 2);
            p.injuryHistory++;
            career.addNews(p.name + " sufre una lesion leve en entrenamiento.");
            emitUiMessage("[Evento] Accidente en entrenamiento: " + p.name +
                          " fuera " + to_string(p.injuryWeeks) + " semanas.");
        }
    } else {
        int maxSquad = getCompetitionConfig(career.myTeam->division).maxSquadSize;
        if (maxSquad > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquad) return;
        int minSkill, maxSkill;
        getDivisionSkillRange(career.myTeam->division, minSkill, maxSkill);
        int youthBoost = max(0, career.myTeam->youthFacilityLevel - 1);
        Player youth = makeRandomPlayer("MED", minSkill + youthBoost, maxSkill + youthBoost, 16, 18);
        youth.potential = clampInt(youth.skill + randInt(8 + youthBoost, 15 + youthBoost), youth.skill, 99);
        career.myTeam->addPlayer(youth);
        career.addNews("La cantera promociona a " + youth.name + " en " + career.myTeam->name + ".");
        emitUiMessage("[Evento] Cantera: se unio " + youth.name +
                      " (pot " + to_string(youth.potential) + ").");
    }
}

static void adjustCpuTactics(Team& team, const Team& opponent, const Team* myTeam) {
    if (&team == myTeam) return;
    int diff = team.getAverageSkill() - opponent.getAverageSkill();
    if (team.morale >= 70) {
        team.tactics = "Pressing";
    } else if (diff >= 6) {
        team.tactics = "Offensive";
    } else if (diff <= -6) {
        team.tactics = "Defensive";
    } else if (team.morale <= 35) {
        team.tactics = "Defensive";
    } else {
        team.tactics = "Balanced";
    }

    if (diff >= 6) {
        team.pressingIntensity = 4;
        team.defensiveLine = 4;
        team.tempo = 4;
        team.width = 4;
        team.markingStyle = "Zonal";
    } else if (diff <= -6) {
        team.pressingIntensity = 2;
        team.defensiveLine = 2;
        team.tempo = 2;
        team.width = 3;
        team.markingStyle = "Hombre";
    } else {
        team.pressingIntensity = 3;
        team.defensiveLine = 3;
        team.tempo = 3;
        team.width = 3;
        team.markingStyle = (team.morale < 40) ? "Hombre" : "Zonal";
    }
}

static void addYouthPlayers(Team& team, int count) {
    int minSkill, maxSkill;
    getDivisionSkillRange(team.division, minSkill, maxSkill);
    for (int i = 0; i < count; ++i) {
        int maxSquad = getCompetitionConfig(team.division).maxSquadSize;
        if (maxSquad > 0 && static_cast<int>(team.players.size()) >= maxSquad) return;
        vector<string> positions;
        if (team.youthRegion == "Norte") positions = {"DEL", "MED", "MED", "DEF"};
        else if (team.youthRegion == "Sur") positions = {"DEF", "MED", "ARQ", "DEL"};
        else if (team.youthRegion == "Patagonia") positions = {"ARQ", "DEF", "DEF", "MED"};
        else if (team.youthRegion == "Centro") positions = {"MED", "MED", "DEL", "DEF"};
        else positions = {"ARQ", "DEF", "MED", "DEL"};
        string pos = positions[randInt(0, static_cast<int>(positions.size()) - 1)];
        int youthBoost = max(0, team.youthFacilityLevel - 1) + max(0, team.youthCoach - 55) / 12;
        Player youth = makeRandomPlayer(pos, minSkill + youthBoost, maxSkill + youthBoost, 16, 18);
        bool eliteProspect = randInt(1, 100) <= clampInt(6 + team.youthFacilityLevel * 3 + max(0, team.youthCoach - 60) / 5, 4, 28);
        int potFloor = eliteProspect ? 14 + youthBoost : 10 + youthBoost;
        int potTop = eliteProspect ? 24 + youthBoost : 18 + youthBoost;
        youth.potential = clampInt(youth.skill + randInt(potFloor, potTop), youth.skill, 99);
        youth.chemistry = clampInt(youth.chemistry + max(0, team.youthCoach - 55) / 10, 1, 99);
        youth.professionalism = clampInt(youth.professionalism + max(0, team.youthCoach - 55) / 12, 1, 99);
        if (eliteProspect) {
            youth.value = max(youth.value, static_cast<long long>(youth.potential) * 12000LL);
            youth.role = (pos == "DEL") ? "Poacher" : (pos == "MED" ? "BoxToBox" : defaultRoleForPosition(pos));
        }
        team.addPlayer(youth);
    }
}

struct TeamTableSnapshot {
    int points;
    int goalsFor;
    int goalsAgainst;
    int awayGoals;
    int wins;
    int draws;
    int losses;
    int yellowCards;
    int redCards;
    vector<HeadToHeadRecord> headToHead;
};

static TeamTableSnapshot captureTableState(const Team& team) {
    return {team.points, team.goalsFor, team.goalsAgainst, team.awayGoals, team.wins, team.draws,
            team.losses, team.yellowCards, team.redCards, team.headToHead};
}

static void restoreTableState(Team& team, const TeamTableSnapshot& snapshot) {
    team.points = snapshot.points;
    team.goalsFor = snapshot.goalsFor;
    team.goalsAgainst = snapshot.goalsAgainst;
    team.awayGoals = snapshot.awayGoals;
    team.wins = snapshot.wins;
    team.draws = snapshot.draws;
    team.losses = snapshot.losses;
    team.yellowCards = snapshot.yellowCards;
    team.redCards = snapshot.redCards;
    team.headToHead = snapshot.headToHead;
}

static void storeMatchAnalysis(Career& career,
                               const Team& home,
                               const Team& away,
                               const MatchResult& result,
                               bool cupMatch) {
    if (!career.myTeam) return;
    bool myHome = (&home == career.myTeam);
    bool myAway = (&away == career.myTeam);
    if (!myHome && !myAway) return;

    int myGoals = myHome ? result.homeGoals : result.awayGoals;
    int oppGoals = myHome ? result.awayGoals : result.homeGoals;
    int myShots = myHome ? result.homeShots : result.awayShots;
    int oppShots = myHome ? result.awayShots : result.homeShots;
    int myPoss = myHome ? result.homePossession : result.awayPossession;
    int oppPoss = myHome ? result.awayPossession : result.homePossession;
    int mySubs = myHome ? result.homeSubstitutions : result.awaySubstitutions;
    int oppSubs = myHome ? result.awaySubstitutions : result.homeSubstitutions;
    int myCorners = myHome ? result.homeCorners : result.awayCorners;
    int oppCorners = myHome ? result.awayCorners : result.homeCorners;
    double myXg = max(0.2, myShots * 0.11 + myCorners * 0.05 + max(0, myPoss - 50) * 0.015);
    double oppXg = max(0.2, oppShots * 0.11 + oppCorners * 0.05 + max(0, oppPoss - 50) * 0.015);
    string opponent = myHome ? away.name : home.name;
    string verdict = (myGoals > oppGoals) ? "Partido controlado" : (myGoals < oppGoals ? "Derrota con ajustes pendientes" : "Empate cerrado");
    string recommendation = "Mantener base y ajustar rotacion segun forma individual";

    if (myShots < oppShots && myPoss < 50) verdict += ", falto presencia ofensiva";
    else if (myShots > oppShots && myGoals <= oppGoals) verdict += ", falto eficacia";
    else if (myPoss >= 55 && myGoals >= oppGoals) verdict += ", buen control del ritmo";
    if (myCorners > oppCorners + 2) verdict += ", se cargo el area rival";
    if (myPoss < 45 && myGoals > oppGoals) verdict += ", transiciones muy efectivas";
    if (myShots + 2 < oppShots && myPoss < 48) {
        recommendation = "Subir un punto la altura de linea o el tempo para recuperar iniciativa";
    } else if (myGoals < oppGoals && myShots >= oppShots) {
        recommendation = "Trabajar finalizacion y pelota parada en la semana";
    } else if (myCorners + 2 < oppCorners) {
        recommendation = "Explorar 'Por bandas' o laterales mas altos para cargar el area";
    } else if (myPoss >= 55 && myGoals == 0) {
        recommendation = "Mantener posesion pero acelerar ultimo tercio con juego mas directo";
    }

    career.lastMatchAnalysis =
        string(cupMatch ? "Copa" : "Liga") + ": " + career.myTeam->name + " " + to_string(myGoals) + "-" +
        to_string(oppGoals) + " " + opponent + " | Tiros " + to_string(myShots) + "-" + to_string(oppShots) +
        " | Posesion " + to_string(myPoss) + "-" + to_string(oppPoss) +
        " | Corners " + to_string(myCorners) + "-" + to_string(oppCorners) +
        " | xG " + to_string(static_cast<int>(myXg * 10 + 0.5)) + "/" + to_string(static_cast<int>(oppXg * 10 + 0.5)) +
        " | Cambios " + to_string(mySubs) + "-" + to_string(oppSubs) +
        " | Clima " + result.weather +
        " | " + verdict +
        " | Recomendacion: " + recommendation;
}

static void simulateSeasonCupRound(Career& career) {
    if (!career.cupActive) return;
    vector<Team*> alive;
    for (const auto& name : career.cupRemainingTeams) {
        Team* team = career.findTeamByName(name);
        if (team && team->division == career.activeDivision) alive.push_back(team);
    }
    if (alive.size() <= 1) {
        career.cupActive = false;
        if (!alive.empty()) {
            career.cupChampion = alive.front()->name;
            career.addNews("Copa de temporada: " + career.cupChampion + " se consagra campeon.");
        }
        return;
    }

    career.cupRound++;
    cout << "\n--- Copa de temporada: ronda " << career.cupRound << " ---" << endl;
    vector<string> nextRound;
    if (alive.size() % 2 == 1) {
        Team* bye = alive.back();
        nextRound.push_back(bye->name);
        alive.pop_back();
        cout << "Pase libre: " << bye->name << endl;
    }

    for (size_t i = 0; i < alive.size(); i += 2) {
        Team* home = alive[i];
        Team* away = alive[i + 1];
        TeamTableSnapshot homeSnap = captureTableState(*home);
        TeamTableSnapshot awaySnap = captureTableState(*away);
        bool verbose = (home == career.myTeam || away == career.myTeam);
        cout << home->name << " vs " << away->name << endl;
        MatchResult result = playMatch(*home, *away, verbose, true, true);
        restoreTableState(*home, homeSnap);
        restoreTableState(*away, awaySnap);
        storeMatchAnalysis(career, *home, *away, result, true);

        Team* winner = home;
        if (result.awayGoals > result.homeGoals) winner = away;
        else if (result.homeGoals == result.awayGoals) {
            winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
            cout << "Gana por penales: " << winner->name << endl;
        }
        nextRound.push_back(winner->name);
    }

    career.cupRemainingTeams = nextRound;
    if (career.cupRemainingTeams.size() == 1) {
        career.cupActive = false;
        career.cupChampion = career.cupRemainingTeams.front();
        career.addNews("Copa de temporada: " + career.cupChampion + " se consagra campeon.");
        cout << "Campeon de la copa: " << career.cupChampion << endl;
    }
}

static void updateSquadDynamics(Career& career, int pointsDelta) {
    if (!career.myTeam) return;
    int captainLeadership = 0;
    int captainIdx = playerIndexByName(*career.myTeam, career.myTeam->captain);
    if (captainIdx >= 0 && captainIdx < static_cast<int>(career.myTeam->players.size())) {
        captainLeadership = career.myTeam->players[captainIdx].leadership;
    }

    auto expectedStartsForPromise = [&](const Player& player) {
        if (player.promisedRole == "Titular") return max(2, career.currentWeek * 2 / 3);
        if (player.promisedRole == "Rotacion") return max(1, career.currentWeek / 3);
        if (player.promisedRole == "Proyecto") return (player.age <= 22) ? max(1, career.currentWeek / 4) : max(0, career.currentWeek / 6);
        return max(0, career.currentWeek / 2 + player.desiredStarts - 1);
    };

    for (auto& player : career.myTeam->players) {
        bool wasUnhappy = player.wantsToLeave;
        int expectedStarts = expectedStartsForPromise(player);
        int unmetStarts = max(0, expectedStarts - player.startsThisSeason);
        if (unmetStarts > 0) {
            player.happiness = clampInt(player.happiness - min(4, 1 + unmetStarts), 1, 99);
            if (player.skill >= career.myTeam->getAverageSkill() && unmetStarts >= 2) {
                player.chemistry = clampInt(player.chemistry - 1, 1, 99);
            }
            if (player.promisedRole != "Sin promesa" && unmetStarts >= 2) {
                player.happiness = clampInt(player.happiness - 2, 1, 99);
            }
        } else {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
            if (player.promisedRole != "Sin promesa") {
                player.happiness = clampInt(player.happiness + 1, 1, 99);
            }
        }

        if (pointsDelta == 3) {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        } else if (pointsDelta == 0 && player.ambition >= 65) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.injured) player.happiness = clampInt(player.happiness - 1, 1, 99);
        if (player.wage < wageDemandFor(player) * 80 / 100) player.happiness = clampInt(player.happiness - 1, 1, 99);
        if (captainLeadership >= 75) player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        if (career.myTeam->morale >= 65) player.happiness = clampInt(player.happiness + 1, 1, 99);
        if (playerHasTrait(player, "Lider") && career.myTeam->morale >= 55) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
        if (playerHasTrait(player, "Caliente") && pointsDelta == 0) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.leadership >= 75 && player.happiness >= 60) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
        if (player.professionalism >= 75 && player.startsThisSeason >= expectedStarts) {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
        }

        int unrestThreshold = (player.skill >= career.myTeam->getAverageSkill() + 3) ? 38 : 32;
        player.wantsToLeave = player.happiness <= unrestThreshold &&
                              (player.ambition >= 60 || player.professionalism <= 45 || unmetStarts >= 3 || player.promisedRole != "Sin promesa");
        if (!wasUnhappy && player.wantsToLeave) {
            career.addNews(player.name + " queda disconforme con su situacion en " + career.myTeam->name + ".");
        }
    }
}

static void runMonthlyDevelopment(Career& career) {
    if (career.currentWeek <= 0 || career.currentWeek % 4 != 0) return;
    for (auto* team : career.activeTeams) {
        int improved = 0;
        int eliteImproved = 0;
        for (auto& player : team->players) {
            if (player.age > 21 || player.skill >= player.potential) continue;
            int chance = 6 + max(0, team->youthCoach - 55) / 10 + max(0, team->trainingFacilityLevel - 1) * 2;
            chance += max(0, player.professionalism - 55) / 15;
            if (player.developmentPlan == "Finalizacion" || player.developmentPlan == "Creatividad" ||
                player.developmentPlan == "Defensa" || player.developmentPlan == "Reflejos") {
                chance += 2;
            }
            if (player.promisedRole == "Proyecto") chance += 2;
            if (playerHasTrait(player, "Proyecto") || playerHasTrait(player, "Competidor")) chance += 2;
            if (randInt(1, 100) <= clampInt(chance, 4, 28)) {
                int gain = 1;
                if (player.age <= 19 && player.potential - player.skill >= 10 && randInt(1, 100) <= 28) gain++;
                player.skill = min(100, player.skill + gain);
                string pos = normalizePosition(player.position);
                if (pos == "ARQ" || pos == "DEF") player.defense = min(100, player.defense + 1);
                else if (pos == "MED") {
                    player.attack = min(100, player.attack + 1);
                    player.defense = min(100, player.defense + 1);
                } else {
                    player.attack = min(100, player.attack + 1);
                }
                if (player.developmentPlan == "Liderazgo") {
                    player.leadership = min(99, player.leadership + 1);
                    player.professionalism = min(99, player.professionalism + 1);
                } else if (player.developmentPlan == "Creatividad") {
                    player.setPieceSkill = min(99, player.setPieceSkill + 1);
                } else if (player.developmentPlan == "Fisico") {
                    player.stamina = min(100, player.stamina + 1);
                    player.fitness = min(player.stamina, player.fitness + 1);
                }
                improved++;
                if (gain > 1) eliteImproved++;
            }
        }
        int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        if ((maxSquad <= 0 || static_cast<int>(team->players.size()) < maxSquad) &&
            randInt(1, 100) <= 4 + team->youthFacilityLevel * 3) {
            addYouthPlayers(*team, 1);
            if (team == career.myTeam) {
                career.addNews("La cantera suma un nuevo prospecto desde la region " + team->youthRegion + ".");
            }
        }
        if (team == career.myTeam && improved > 0) {
            career.addNews("Informe juvenil: " + to_string(improved) + " jugador(es) joven(es) mejoran este mes.");
            if (eliteImproved > 0) {
                career.addNews("Informe de cantera: " + to_string(eliteImproved) + " prospecto(s) muestran una aceleracion especial.");
            }
        }
    }
}

static string weakestSquadPosition(const Team& team) {
    vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    unordered_map<string, int> counts;
    unordered_map<string, int> skills;
    for (const auto& player : team.players) {
        string pos = normalizePosition(player.position);
        if (pos == "N/A") continue;
        counts[pos]++;
        skills[pos] += player.skill;
    }
    string bestPos = "MED";
    int bestScore = 1000000;
    for (const auto& pos : positions) {
        int count = counts[pos];
        int avgSkill = (count > 0) ? (skills[pos] / count) : 0;
        int score = count * 20 + avgSkill;
        if (score < bestScore) {
            bestScore = score;
            bestPos = pos;
        }
    }
    return bestPos;
}

static void updateShortlistAlerts(Career& career) {
    if (!career.myTeam || career.scoutingShortlist.empty()) return;
    if (career.currentWeek % 4 != 0) return;

    vector<string> active;
    for (const auto& item : career.scoutingShortlist) {
        auto parts = splitByDelimiter(item, '|');
        if (parts.size() < 2) continue;
        Team* seller = career.findTeamByName(parts[0]);
        if (!seller) continue;
        int idx = playerIndexByName(*seller, parts[1]);
        if (idx < 0) continue;
        const Player& player = seller->players[static_cast<size_t>(idx)];
        active.push_back(item);
        if (player.contractWeeks <= 12) {
            career.addNews("Alerta de shortlist: " + player.name + " entra en ventana de precontrato con " + seller->name + ".");
        } else if (player.value <= player.releaseClause * 60 / 100) {
            career.addNews("Alerta de shortlist: " + player.name + " mantiene un costo accesible en " + seller->name + ".");
        }
    }
    career.scoutingShortlist = active;
}

static void generateWeeklyNarratives(Career& career, int myTeamPointsDelta) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    int field = max(1, career.currentCompetitiveFieldSize());
    if (rank > 0 && rank <= max(2, field / 4)) {
        career.addNews("La prensa destaca a " + career.myTeam->name + " por su presencia en la zona alta.");
    } else if (rank >= max(2, field - field / 4)) {
        career.addNews("La prensa pone a " + career.myTeam->name + " en la pelea por evitar el fondo.");
    }

    if (myTeamPointsDelta == 3 && career.myTeam->morale >= 65) {
        career.addNews("El vestuario de " + career.myTeam->name + " atraviesa un momento de confianza.");
    } else if (myTeamPointsDelta == 0 && career.boardConfidence <= 30) {
        career.addNews("Crece la tension institucional alrededor de " + career.myTeam->name + ".");
    }

    int promiseAlerts = 0;
    int leaders = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.promisedRole == "Titular" && player.startsThisSeason + 2 < max(2, career.currentWeek * 2 / 3)) promiseAlerts++;
        if (player.promisedRole == "Rotacion" && player.startsThisSeason + 1 < max(1, career.currentWeek / 3)) promiseAlerts++;
        if ((player.leadership >= 72 || playerHasTrait(player, "Lider")) && player.happiness >= 55) leaders++;
    }
    if (promiseAlerts > 0) {
        career.addNews("Se acumulan " + to_string(promiseAlerts) + " promesa(s) de rol bajo presion en el plantel.");
    }
    if (career.myTeam->fanBase >= 65 && myTeamPointsDelta == 3) {
        career.addNews("La aficion responde con entusiasmo y empuja la recaudacion del club.");
    } else if (career.myTeam->fanBase >= 45 && myTeamPointsDelta == 0) {
        career.addNews("La prensa cuestiona la falta de resultados recientes de " + career.myTeam->name + ".");
    }
    if (leaders >= 3 && career.myTeam->morale >= 60) {
        career.addNews("Los lideres del vestuario sostienen un ambiente competitivo en " + career.myTeam->name + ".");
    }

    for (const auto& player : career.myTeam->players) {
        if (player.contractWeeks > 0 && player.contractWeeks <= 4) {
            career.addNews("Contrato al limite: " + player.name + " entra en sus ultimas " + to_string(player.contractWeeks) + " semana(s).");
            break;
        }
    }
}

static void applyDepartureShock(Team& team, const Player& player) {
    bool keyPlayer = player.skill >= team.getAverageSkill();
    if (!keyPlayer) return;
    team.morale = clampInt(team.morale - 3, 0, 100);
    for (auto& mate : team.players) {
        if (mate.name == player.name) continue;
        mate.chemistry = clampInt(mate.chemistry - 1, 1, 99);
        if (player.leadership >= 70) mate.happiness = clampInt(mate.happiness - 1, 1, 99);
    }
}

static void detachNamedResponsibilities(Team& team, const string& playerName) {
    team.preferredXI.erase(remove(team.preferredXI.begin(), team.preferredXI.end(), playerName), team.preferredXI.end());
    team.preferredBench.erase(remove(team.preferredBench.begin(), team.preferredBench.end(), playerName), team.preferredBench.end());
    if (team.captain == playerName) team.captain.clear();
    if (team.penaltyTaker == playerName) team.penaltyTaker.clear();
    if (team.freeKickTaker == playerName) team.freeKickTaker.clear();
    if (team.cornerTaker == playerName) team.cornerTaker.clear();
}

static void processIncomingOffers(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->players.size() <= 18) return;
    bool squadUnrest = false;
    for (const auto& player : career.myTeam->players) {
        if (player.wantsToLeave) {
            squadUnrest = true;
            break;
        }
    }
    if (randInt(1, 100) > (squadUnrest ? 50 : 32)) return;
    vector<int> candidates;
    for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
        const Player& player = career.myTeam->players[i];
        if (!player.injured && player.contractWeeks > 8) {
            candidates.push_back(static_cast<int>(i));
            if (player.wantsToLeave) candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) return;
    int idx = candidates[randInt(0, static_cast<int>(candidates.size()) - 1)];
    Player& p = career.myTeam->players[idx];
    long long maxOffer = max(p.value, p.value * (105 + randInt(0, 40)) / 100);
    long long offer = max(p.value * 9 / 10, maxOffer * 85 / 100);
    emitUiMessage("");
    emitUiMessage("Oferta recibida por " + p.name + ": $" + to_string(offer) +
                  " (tope estimado del mercado: $" + to_string(maxOffer) + ")");
    int choice = 3;
    long long counter = 0;
    if (g_incomingOfferDecisionCallback) {
        IncomingOfferDecision decision = g_incomingOfferDecisionCallback(career, p, offer, maxOffer);
        if (decision.action >= 1 && decision.action <= 3) {
            choice = decision.action;
            counter = decision.counterOffer;
        }
    } else {
        cout << "1. Aceptar" << endl;
        cout << "2. Contraofertar" << endl;
        cout << "3. Rechazar" << endl;
        choice = readInt("Respuesta: ", 1, 3);
        if (choice == 2) {
            counter = readLongLong("Tu contraoferta: ", 0, 1000000000000LL);
        }
    }
    if (choice == 1) {
        career.myTeam->budget += offer;
        emitUiMessage("Transferencia aceptada. " + p.name + " vendido.");
        career.addNews(p.name + " es vendido por $" + to_string(offer) + ".");
        detachNamedResponsibilities(*career.myTeam, p.name);
        applyDepartureShock(*career.myTeam, p);
        career.myTeam->players.erase(career.myTeam->players.begin() + idx);
    } else if (choice == 2) {
        if (counter <= maxOffer) {
            career.myTeam->budget += counter;
            emitUiMessage("Contraoferta aceptada. " + p.name + " vendido por $" + to_string(counter));
            career.addNews(p.name + " es vendido tras contraoferta por $" + to_string(counter) + ".");
            detachNamedResponsibilities(*career.myTeam, p.name);
            applyDepartureShock(*career.myTeam, p);
            career.myTeam->players.erase(career.myTeam->players.begin() + idx);
        } else {
            emitUiMessage("La contraoferta fue rechazada.");
        }
    } else {
        emitUiMessage("Oferta rechazada.");
    }
}

static void processCpuTransfers(Career& career) {
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        if (career.myTeam == team) continue;
        if (randInt(1, 100) > 14) continue;

        if (team->budget < 120000 && team->players.size() > 18) {
            int sellIdx = 0;
            for (size_t i = 1; i < team->players.size(); ++i) {
                const Player& current = team->players[i];
                const Player& best = team->players[sellIdx];
                if (current.value > best.value || (current.value == best.value && current.age > best.age)) {
                    sellIdx = static_cast<int>(i);
                }
            }
            long long saleValue = team->players[sellIdx].value;
            detachNamedResponsibilities(*team, team->players[sellIdx].name);
            team->players.erase(team->players.begin() + sellIdx);
            team->budget += saleValue;
        }

        if (team->players.size() >= 24 || team->budget < 150000) continue;
        string pos = weakestSquadPosition(*team);
        Team* seller = nullptr;
        int sellerIdx = -1;
        int bestScore = -100000;
        for (auto& club : career.allTeams) {
            if (&club == team || &club == career.myTeam) continue;
            if (club.players.size() <= 18) continue;
            for (size_t i = 0; i < club.players.size(); ++i) {
                const Player& target = club.players[i];
                if (target.onLoan) continue;
                if (normalizePosition(target.position) != pos) continue;
                long long fee = max(target.value, target.releaseClause * 55 / 100);
                if (fee > team->budget * 65 / 100) continue;
                int score = target.skill * 3 + target.potential - target.age * 2;
                if (target.contractWeeks <= 20) score += 6;
                if (score > bestScore) {
                    bestScore = score;
                    seller = &club;
                    sellerIdx = static_cast<int>(i);
                }
            }
        }

        if (seller && sellerIdx >= 0) {
            Player newP = seller->players[static_cast<size_t>(sellerIdx)];
            long long fee = max(newP.value, newP.releaseClause * 55 / 100);
            team->budget -= fee;
            seller->budget += fee;
            newP.releaseClause = max(newP.value * 2, fee * 2);
            newP.wantsToLeave = false;
            team->addPlayer(newP);
            detachNamedResponsibilities(*seller, newP.name);
            seller->players.erase(seller->players.begin() + sellerIdx);
        } else {
            int minSkill, maxSkill;
            getDivisionSkillRange(team->division, minSkill, maxSkill);
            Player newP = makeRandomPlayer(pos, minSkill, maxSkill, 18, 29);
            long long fee = max(newP.value, newP.releaseClause * 55 / 100);
            if (team->budget >= fee) {
                team->budget -= fee;
                newP.releaseClause = max(newP.value * 2, fee * 2);
                team->addPlayer(newP);
            }
        }
    }
}

static void updateContracts(Career& career) {
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        for (size_t i = 0; i < team->players.size();) {
            Player& p = team->players[i];
            if (p.contractWeeks > 0) p.contractWeeks--;
            if (p.contractWeeks > 0) {
                ++i;
                continue;
            }
            if (team == career.myTeam) {
                long long demandedWage = max(p.wage, wageDemandFor(p));
                if (p.wantsToLeave) demandedWage = demandedWage * 120 / 100;
                if (p.skill >= team->getAverageSkill()) demandedWage = demandedWage * 110 / 100;
                int demandedWeeks = randInt(78, 182);
                long long demandedClause = max(p.value * 2, demandedWage * (p.skill >= team->getAverageSkill() ? 48 : 40));
                emitUiMessage("");
                emitUiMessage("Contrato expirado: " + p.name);
                emitUiMessage("Demanda renovar por " + to_string(demandedWeeks) +
                              " semanas | Salario $" + to_string(demandedWage) +
                              " | Clausula $" + to_string(demandedClause));
                if (p.wantsToLeave) {
                    emitUiMessage(p.name + " esta inquieto por su rol en el club y exige mejores condiciones.");
                }
                bool renew = false;
                if (g_contractRenewalDecisionCallback) {
                    renew = g_contractRenewalDecisionCallback(career, *team, p, demandedWage, demandedWeeks, demandedClause);
                } else {
                    int choice = readInt("Renovar? (1. Si, 2. No): ", 1, 2);
                    renew = (choice == 1);
                }
                if (renew) {
                    if (team->budget < demandedWage * 6) {
                        emitUiMessage("No hay margen salarial suficiente. " + p.name + " deja el club.");
                        career.addNews(p.name + " deja el club tras no acordar renovacion.");
                        detachNamedResponsibilities(*team, p.name);
                        applyDepartureShock(*team, p);
                        team->players.erase(team->players.begin() + i);
                    } else {
                        p.contractWeeks = demandedWeeks;
                        p.wage = demandedWage;
                        p.releaseClause = demandedClause;
                        p.wantsToLeave = false;
                        p.happiness = clampInt(p.happiness + 6, 1, 99);
                        emitUiMessage("Renovado. Nuevo salario $" + to_string(p.wage));
                        career.addNews(p.name + " renueva contrato en " + team->name + ".");
                        ++i;
                    }
                } else {
                    emitUiMessage(p.name + " deja el club.");
                    career.addNews(p.name + " deja el club al finalizar su contrato.");
                    detachNamedResponsibilities(*team, p.name);
                    applyDepartureShock(*team, p);
                    team->players.erase(team->players.begin() + i);
                }
            } else {
                if (team->budget > p.wage * 8 && randInt(1, 100) <= 70) {
                    p.contractWeeks = randInt(52, 156);
                    p.wage = static_cast<long long>(p.wage * (1.05 + randInt(0, 15) / 100.0));
                    p.releaseClause = max(p.value * 2, p.wage * 45);
                    ++i;
                } else {
                    detachNamedResponsibilities(*team, p.name);
                    team->players.erase(team->players.begin() + i);
                }
            }
        }
    }
}

static void processLoanReturns(Career& career) {
    for (auto& team : career.allTeams) {
        for (size_t i = 0; i < team.players.size();) {
            Player& player = team.players[i];
            if (!player.onLoan) {
                ++i;
                continue;
            }
            if (player.loanWeeksRemaining > 0) player.loanWeeksRemaining--;
            if (player.loanWeeksRemaining > 0) {
                ++i;
                continue;
            }
            Team* parent = career.findTeamByName(player.parentClub);
            if (!parent || parent == &team) {
                player.onLoan = false;
                player.parentClub.clear();
                ++i;
                continue;
            }
            Player returning = player;
            returning.onLoan = false;
            returning.parentClub.clear();
            returning.loanWeeksRemaining = 0;
            parent->addPlayer(returning);
            career.addNews(returning.name + " regresa desde prestamo a " + parent->name + ".");
            team.players.erase(team.players.begin() + static_cast<long long>(i));
        }
    }
}

static void updateManagerReputation(Career& career) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    if (rank > 0) {
        if (rank <= max(1, career.boardExpectedFinish - 1)) career.managerReputation = clampInt(career.managerReputation + 2, 1, 100);
        else if (rank > career.boardExpectedFinish + 2) career.managerReputation = clampInt(career.managerReputation - 1, 1, 100);
    }
}

static void handleManagerStatus(Career& career) {
    if (!career.myTeam) return;
    if (career.boardConfidence >= 20 && career.boardWarningWeeks < 6) return;
    emitUiMessage("");
    emitUiMessage("[Directiva] " + career.myTeam->name + " decide despedirte.");
    career.addNews(career.managerName + " fue despedido de " + career.myTeam->name + ".");
    career.managerReputation = clampInt(career.managerReputation - 8, 10, 100);
    vector<Team*> jobs = buildJobMarket(career, true);
    if (jobs.empty()) {
        for (auto& team : career.allTeams) {
            if (&team != career.myTeam) jobs.push_back(&team);
        }
    }
    emitUiMessage("Debes elegir nuevo club:");
    for (size_t i = 0; i < jobs.size(); ++i) {
        emitUiMessage(to_string(i + 1) + ". " + jobs[i]->name + " (" + divisionDisplay(jobs[i]->division) + ")");
    }
    int choice = 1;
    if (g_managerJobSelectionCallback) {
        int selected = g_managerJobSelectionCallback(career, jobs);
        if (selected >= 0 && selected < static_cast<int>(jobs.size())) {
            choice = selected + 1;
        }
    } else {
        choice = readInt("Club: ", 1, static_cast<int>(jobs.size()));
    }
    takeManagerJob(career, jobs[choice - 1], "Llega tras un despido reciente.");
}

static void recordSeasonHistory(Career& career,
                                const string& champion,
                                const vector<Team*>& promoted,
                                const vector<Team*>& relegated,
                                const string& note) {
    if (!career.myTeam) return;
    SeasonHistoryEntry entry;
    entry.season = career.currentSeason;
    entry.division = career.activeDivision;
    entry.club = career.myTeam->name;
    entry.finish = career.currentCompetitiveRank();
    entry.champion = champion;
    entry.promoted = joinTeamNames(promoted);
    entry.relegated = joinTeamNames(relegated);
    entry.note = note;
    career.history.push_back(entry);
    if (career.history.size() > 30) {
        career.history.erase(career.history.begin(), career.history.begin() + static_cast<long long>(career.history.size() - 30));
    }
}

static void awardSeasonPrizeMoney(Career& career, const LeagueTable& table) {
    if (!career.myTeam) return;
    int rank = teamRank(table, career.myTeam);
    if (rank <= 0) return;
    int size = max(1, static_cast<int>(table.teams.size()));
    long long prize = max(10000LL, static_cast<long long>(size - rank + 1) * 12000LL);
    career.myTeam->budget += prize;
    career.addNews(career.myTeam->name + " recibe $" + to_string(prize) + " por su ubicacion final.");
}

static void advanceToNextSeason(Career& career) {
    career.currentSeason++;
    career.currentWeek = 1;
    career.agePlayers();
    career.executePendingTransfers();
    if (career.myTeam) {
        career.setActiveDivision(career.myTeam->division);
    } else {
        career.setActiveDivision(career.activeDivision);
    }
    career.resetSeason();
    for (auto* team : career.activeTeams) {
        addYouthPlayers(*team, 1);
    }
}

static int divisionIndex(const string& id) {
    for (size_t i = 0; i < kDivisions.size(); ++i) {
        if (kDivisions[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

static vector<Team*> topByValue(vector<Team*> teams, int count) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() > b->getSquadValue();
    });
    if (count < 0) count = 0;
    if (static_cast<int>(teams.size()) > count) teams.resize(count);
    return teams;
}

static vector<Team*> sortByValue(vector<Team*> teams) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() > b->getSquadValue();
    });
    return teams;
}

static vector<Team*> bottomByValue(vector<Team*> teams, int count) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() < b->getSquadValue();
    });
    if (count < 0) count = 0;
    if (static_cast<int>(teams.size()) > count) teams.resize(count);
    return teams;
}

static Team* simulatePlayoffMatch(Team* home, Team* away, const string& label) {
    if (!home) return away;
    if (!away) return home;
    cout << label << ": " << home->name << " vs " << away->name << endl;
    Team h1 = *home;
    Team a1 = *away;
    MatchResult r1 = playMatch(h1, a1, false, true);
    Team h2 = *home;
    Team a2 = *away;
    MatchResult r2 = playMatch(a2, h2, false, true);

    int homeAgg = r1.homeGoals + r2.awayGoals;
    int awayAgg = r1.awayGoals + r2.homeGoals;

    cout << "Ida: " << home->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << away->name << endl;
    cout << "Vuelta: " << away->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << home->name << endl;
    cout << "Global: " << home->name << " " << homeAgg << " - " << awayAgg << " " << away->name << endl;
    if (homeAgg > awayAgg) return home;
    if (awayAgg > homeAgg) return away;
    Team* winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

static vector<vector<pair<int, int>>> buildRoundRobinIndexSchedule(int teamCount, bool doubleRound) {
    vector<vector<pair<int, int>>> out;
    if (teamCount < 2) return out;
    vector<int> idx;
    idx.reserve(teamCount + 1);
    for (int i = 0; i < teamCount; ++i) idx.push_back(i);
    if (idx.size() % 2 == 1) idx.push_back(-1);

    int size = static_cast<int>(idx.size());
    int rounds = size - 1;
    for (int round = 0; round < rounds; ++round) {
        vector<pair<int, int>> matches;
        for (int i = 0; i < size / 2; ++i) {
            int a = idx[i];
            int b = idx[size - 1 - i];
            if (a == -1 || b == -1) continue;
            if (round % 2 == 0) matches.push_back({a, b});
            else matches.push_back({b, a});
        }
        out.push_back(matches);
        int last = idx.back();
        for (int i = size - 1; i > 1; --i) idx[i] = idx[i - 1];
        idx[1] = last;
    }

    if (doubleRound) {
        int base = static_cast<int>(out.size());
        for (int i = 0; i < base; ++i) {
            vector<pair<int, int>> rev;
            for (auto& m : out[i]) rev.push_back({m.second, m.first});
            out.push_back(rev);
        }
    }
    return out;
}

static void simulateBackgroundDivisionWeek(Career& career, const string& divisionId) {
    if (divisionId.empty() || divisionId == career.activeDivision) return;
    vector<Team*> teams = career.getDivisionTeams(divisionId);
    if (teams.size() < 2) return;
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
        return a->name < b->name;
    });
    auto schedule = buildRoundRobinIndexSchedule(static_cast<int>(teams.size()), true);
    int round = career.currentWeek - 1;
    if (round < 0 || round >= static_cast<int>(schedule.size())) return;

    int headlineMargin = -1;
    string headline;
    for (const auto& match : schedule[static_cast<size_t>(round)]) {
        Team* home = teams[static_cast<size_t>(match.first)];
        Team* away = teams[static_cast<size_t>(match.second)];
        MatchResult result = playMatch(*home, *away, false, false);
        int margin = abs(result.homeGoals - result.awayGoals);
        if (margin > headlineMargin) {
            headlineMargin = margin;
            headline = home->name + " " + to_string(result.homeGoals) + "-" + to_string(result.awayGoals) + " " + away->name;
        }
    }

    LeagueTable table;
    table.title = divisionDisplay(divisionId);
    table.ruleId = divisionId;
    for (Team* team : teams) table.addTeam(team);
    table.sortTable();
    if (!headline.empty() && (career.currentWeek % 4 == 0 || headlineMargin >= 3)) {
        Team* leader = table.teams.empty() ? nullptr : table.teams.front();
        string news = "[Mundo] " + divisionDisplay(divisionId) + ": " + headline;
        if (leader) news += " | Lider " + leader->name + " (" + to_string(leader->points) + " pts)";
        career.addNews(news);
    }
}

static void awardLegPoints(int goalsA, int goalsB, int& pointsA, int& pointsB) {
    if (goalsA > goalsB) {
        pointsA += 3;
    } else if (goalsB > goalsA) {
        pointsB += 3;
    } else {
        pointsA += 1;
        pointsB += 1;
    }
}

static Team* simulateSingleLegKnockout(Team* home, Team* away, const string& label, bool verbose, bool neutralVenue = false) {
    if (!home) return away;
    if (!away) return home;
    cout << label << ": " << home->name << " vs " << away->name << endl;
    Team h1 = *home;
    Team a1 = *away;
    MatchResult r = playMatch(h1, a1, verbose, true, neutralVenue);
    cout << "Resultado: " << home->name << " " << r.homeGoals << " - " << r.awayGoals << " " << away->name << endl;
    if (r.homeGoals > r.awayGoals) return home;
    if (r.awayGoals > r.homeGoals) return away;
    Team* winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

static Team* simulateTwoLegTie(Team* firstHome, Team* firstAway, const string& label, bool extraTimeFinal) {
    if (!firstHome) return firstAway;
    if (!firstAway) return firstHome;
    cout << label << ": " << firstAway->name << " vs " << firstHome->name << endl;

    Team h1 = *firstHome;
    Team a1 = *firstAway;
    MatchResult r1 = playMatch(h1, a1, false, true);
    Team h2 = *firstAway;
    Team a2 = *firstHome;
    MatchResult r2 = playMatch(h2, a2, false, true);

    int firstHomePoints = 0;
    int firstAwayPoints = 0;
    awardLegPoints(r1.homeGoals, r1.awayGoals, firstHomePoints, firstAwayPoints);
    awardLegPoints(r2.awayGoals, r2.homeGoals, firstHomePoints, firstAwayPoints);

    int firstHomeAgg = r1.homeGoals + r2.awayGoals;
    int firstAwayAgg = r1.awayGoals + r2.homeGoals;
    int firstHomeGD = firstHomeAgg - firstAwayAgg;
    int firstAwayGD = -firstHomeGD;

    cout << "Ida: " << firstHome->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << firstAway->name << endl;
    cout << "Vuelta: " << firstAway->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << firstHome->name << endl;
    cout << "Puntos serie: " << firstHome->name << " " << firstHomePoints << " - " << firstAwayPoints << " " << firstAway->name << endl;
    cout << "Global: " << firstHome->name << " " << firstHomeAgg << " - " << firstAwayAgg << " " << firstAway->name << endl;

    if (firstHomePoints > firstAwayPoints) return firstHome;
    if (firstAwayPoints > firstHomePoints) return firstAway;
    if (firstHomeGD > firstAwayGD) return firstHome;
    if (firstAwayGD > firstHomeGD) return firstAway;

    if (extraTimeFinal) {
        cout << "Serie igualada en puntos y diferencia de gol." << endl;
        int etHome = randInt(0, 1);
        int etAway = randInt(0, 1);
        cout << "Prorroga (vuelta): " << firstAway->name << " " << etHome << " - " << etAway << " " << firstHome->name << endl;
        if (etHome > etAway) return firstAway;
        if (etAway > etHome) return firstHome;
    }

    Team* winner = (teamPenaltyStrength(*firstHome) >= teamPenaltyStrength(*firstAway)) ? firstHome : firstAway;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

static Team* liguillaAscensoPrimeraB(const vector<Team*>& table) {
    if (table.size() < 2) return table.empty() ? nullptr : table.front();

    unordered_map<Team*, int> seedPos;
    for (size_t i = 0; i < table.size(); ++i) seedPos[table[i]] = static_cast<int>(i) + 1;

    Team* s2 = table.size() > 1 ? table[1] : nullptr;
    Team* s3 = table.size() > 2 ? table[2] : nullptr;
    Team* s4 = table.size() > 3 ? table[3] : nullptr;
    Team* s5 = table.size() > 4 ? table[4] : nullptr;
    Team* s6 = table.size() > 5 ? table[5] : nullptr;
    Team* s7 = table.size() > 6 ? table[6] : nullptr;
    Team* s8 = table.size() > 7 ? table[7] : nullptr;

    cout << "\nLiguilla de Ascenso (2°-8°)" << endl;
    Team* w1 = simulateTwoLegTie(s8, s3, "Primera ronda 1 (3° vs 8°)", false);
    Team* w2 = simulateTwoLegTie(s7, s4, "Primera ronda 2 (4° vs 7°)", false);
    Team* w3 = simulateTwoLegTie(s6, s5, "Primera ronda 3 (5° vs 6°)", false);

    vector<Team*> winners;
    if (w1) winners.push_back(w1);
    if (w2) winners.push_back(w2);
    if (w3) winners.push_back(w3);

    auto seed = [&](Team* t) {
        auto it = seedPos.find(t);
        return it == seedPos.end() ? 99 : it->second;
    };

    Team* worst = nullptr;
    int worstSeed = -1;
    for (auto* w : winners) {
        int s = seed(w);
        if (s > worstSeed) {
            worstSeed = s;
            worst = w;
        }
    }

    Team* semiA = nullptr;
    if (s2 && worst) {
        semiA = simulateTwoLegTie(worst, s2, "Semifinal A (2° vs peor clasificado)", false);
    } else {
        semiA = s2 ? s2 : worst;
    }

    vector<Team*> remaining;
    for (auto* w : winners) {
        if (w && w != worst) remaining.push_back(w);
    }

    Team* semiB = nullptr;
    if (remaining.size() == 2) {
        Team* a = remaining[0];
        Team* b = remaining[1];
        Team* firstHome = seed(a) > seed(b) ? a : b;
        Team* firstAway = (firstHome == a) ? b : a;
        semiB = simulateTwoLegTie(firstHome, firstAway, "Semifinal B", false);
    } else if (remaining.size() == 1) {
        semiB = remaining[0];
    }

    if (!semiA) return semiB;
    if (!semiB) return semiA;

    Team* firstHome = seed(semiA) > seed(semiB) ? semiA : semiB;
    Team* firstAway = (firstHome == semiA) ? semiB : semiA;
    Team* winner = simulateTwoLegTie(firstHome, firstAway, "Final Liguilla", true);
    if (winner) cout << "Ganador liguilla: " << winner->name << endl;
    return winner;
}

static Team* teamAtPos(const LeagueTable& table, int pos) {
    int idx = pos - 1;
    if (idx < 0 || idx >= static_cast<int>(table.teams.size())) return nullptr;
    return table.teams[idx];
}

static Team* loserOfTwoTeamTie(Team* winner, Team* a, Team* b) {
    if (!a) return b;
    if (!b) return a;
    return winner == a ? b : a;
}

static Team* simulateTwoLegAggregateTie(Team* firstHome, Team* firstAway, const string& label) {
    if (!firstHome) return firstAway;
    if (!firstAway) return firstHome;
    cout << label << ": " << firstAway->name << " vs " << firstHome->name << endl;

    Team h1 = *firstHome;
    Team a1 = *firstAway;
    MatchResult r1 = playMatch(h1, a1, false, true);
    Team h2 = *firstAway;
    Team a2 = *firstHome;
    MatchResult r2 = playMatch(h2, a2, false, true);

    int firstHomeAgg = r1.homeGoals + r2.awayGoals;
    int firstAwayAgg = r1.awayGoals + r2.homeGoals;

    cout << "Ida: " << firstHome->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << firstAway->name << endl;
    cout << "Vuelta: " << firstAway->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << firstHome->name << endl;
    cout << "Global: " << firstHome->name << " " << firstHomeAgg << " - " << firstAwayAgg << " " << firstAway->name << endl;

    if (firstHomeAgg > firstAwayAgg) return firstHome;
    if (firstAwayAgg > firstHomeAgg) return firstAway;

    Team* winner = (teamPenaltyStrength(*firstHome) >= teamPenaltyStrength(*firstAway)) ? firstHome : firstAway;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

struct TerceraBSeasonOutcome {
    vector<Team*> directPromoted;
    vector<Team*> promotionCandidates;
    Team* champion = nullptr;
};

struct TerceraARelegationOutcome {
    vector<Team*> promotionTeams;
    vector<Team*> directRelegated;
};

static TerceraBSeasonOutcome resolveTerceraBSeason(const vector<Team*>& northRanked,
                                                   const vector<Team*>& southRanked,
                                                   bool usePlayoffLosersForPromotion) {
    TerceraBSeasonOutcome out;
    Team* northChampion = northRanked.size() > 0 ? northRanked[0] : nullptr;
    Team* southChampion = southRanked.size() > 0 ? southRanked[0] : nullptr;

    if (northChampion) out.directPromoted.push_back(northChampion);
    if (southChampion && find(out.directPromoted.begin(), out.directPromoted.end(), southChampion) == out.directPromoted.end()) {
        out.directPromoted.push_back(southChampion);
    }

    if (northChampion && southChampion) {
        cout << "\nFinal por el campeonato de Tercera B:" << endl;
        out.champion = simulateSingleLegKnockout(northChampion, southChampion, "Final Tercera B", false, true);
        if (out.champion) cout << "Campeon de Tercera B: " << out.champion->name << endl;
    } else {
        out.champion = northChampion ? northChampion : southChampion;
    }

    Team* south2 = southRanked.size() > 1 ? southRanked[1] : nullptr;
    Team* north3 = northRanked.size() > 2 ? northRanked[2] : nullptr;
    Team* north2 = northRanked.size() > 1 ? northRanked[1] : nullptr;
    Team* south3 = southRanked.size() > 2 ? southRanked[2] : nullptr;

    cout << "\nPlayoffs de promocion Tercera B:" << endl;
    Team* tie1Winner = simulateTwoLegAggregateTie(south2, north3, "Cruce 1 (2° Sur vs 3° Norte)");
    Team* tie2Winner = simulateTwoLegAggregateTie(north2, south3, "Cruce 2 (2° Norte vs 3° Sur)");

    Team* candidate1 = usePlayoffLosersForPromotion ? loserOfTwoTeamTie(tie1Winner, south2, north3) : tie1Winner;
    Team* candidate2 = usePlayoffLosersForPromotion ? loserOfTwoTeamTie(tie2Winner, north2, south3) : tie2Winner;

    if (candidate1) out.promotionCandidates.push_back(candidate1);
    if (candidate2 && find(out.promotionCandidates.begin(), out.promotionCandidates.end(), candidate2) == out.promotionCandidates.end()) {
        out.promotionCandidates.push_back(candidate2);
    }
    return out;
}

static TerceraBSeasonOutcome inferTerceraBSeasonByValue(Career& career) {
    auto teams = career.getDivisionTeams("tercera division b");
    vector<Team*> north;
    vector<Team*> south;
    for (size_t i = 0; i < teams.size(); ++i) {
        if (i < 14) north.push_back(teams[i]);
        else if (i < 28) south.push_back(teams[i]);
    }
    north = sortByValue(north);
    south = sortByValue(south);
    return resolveTerceraBSeason(north, south, true);
}

static TerceraARelegationOutcome getInactiveTerceraARelegationByValue(Career& career) {
    TerceraARelegationOutcome out;
    auto teams = career.getDivisionTeams("tercera division a");
    teams = bottomByValue(teams, 4);
    if (teams.size() > 0) out.directRelegated.push_back(teams[0]);
    if (teams.size() > 1) out.directRelegated.push_back(teams[1]);
    if (teams.size() > 3) {
        out.promotionTeams.push_back(teams[3]);
        out.promotionTeams.push_back(teams[2]);
    } else if (teams.size() > 2) {
        out.promotionTeams.push_back(teams[2]);
    }
    return out;
}

static Team* simulateTerceraAPlayoff(const vector<Team*>& table) {
    if (table.size() < 5) return nullptr;
    Team* s2 = table[1];
    Team* s3 = table[2];
    Team* s4 = table[3];
    Team* s5 = table[4];

    cout << "\nPlayoff de ascenso Tercera A:" << endl;
    Team* sf1 = simulateTwoLegAggregateTie(s5, s2, "Semifinal 1 (2° vs 5°)");
    Team* sf2 = simulateTwoLegAggregateTie(s4, s3, "Semifinal 2 (3° vs 4°)");
    if (!sf1) return sf2;
    if (!sf2) return sf1;

    Team* winner = simulateSingleLegKnockout(sf1, sf2, "Final playoff Tercera A", false, true);
    if (winner) cout << "Ganador playoff Tercera A: " << winner->name << endl;
    return winner;
}

static Team* simulateSegundaPlayoff(const vector<Team*>& seeds) {
    if (seeds.empty()) return nullptr;
    if (seeds.size() == 1) return seeds.front();

    Team* s1 = seeds.size() > 0 ? seeds[0] : nullptr;
    Team* s2 = seeds.size() > 1 ? seeds[1] : nullptr;
    Team* s3 = seeds.size() > 2 ? seeds[2] : nullptr;
    Team* s4 = seeds.size() > 3 ? seeds[3] : nullptr;
    Team* s5 = seeds.size() > 4 ? seeds[4] : nullptr;
    Team* s6 = seeds.size() > 5 ? seeds[5] : nullptr;
    Team* s7 = seeds.size() > 6 ? seeds[6] : nullptr;

    cout << "\n--- Playoff de Ascenso ---" << endl;
    Team* q1 = simulatePlayoffMatch(s2, s7, "Cuartos 1");
    Team* q2 = simulatePlayoffMatch(s3, s6, "Cuartos 2");
    Team* q3 = simulatePlayoffMatch(s4, s5, "Cuartos 3");

    Team* semi1 = simulatePlayoffMatch(s1, q3, "Semifinal 1");
    Team* semi2 = simulatePlayoffMatch(q1, q2, "Semifinal 2");
    Team* champion = simulatePlayoffMatch(semi1, semi2, "Final");
    return champion;
}

static void endSeasonSegundaDivision(Career& career) {
    cout << "\nFin de temporada (Segunda Division)!" << endl;
    if (career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.buildSegundaGroups();
    }
    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, "Grupo Norte");
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, "Grupo Sur");
    north.displayTable();
    south.displayTable();

    vector<Team*> playoffTeams;
    for (int pos = 1; pos <= 3; ++pos) {
        if (Team* t = teamAtPos(north, pos)) playoffTeams.push_back(t);
        if (Team* t = teamAtPos(south, pos)) playoffTeams.push_back(t);
    }

    Team* north4 = teamAtPos(north, 4);
    Team* south4 = teamAtPos(south, 4);
    Team* playoffExtra = nullptr;
    Team* descensoExtra = nullptr;
    if (north4 && south4) {
        cout << "\nPartido 4° vs 4°:" << endl;
        Team* winner = simulateSingleLegKnockout(north4, south4, "Repechaje 4°", north4 == career.myTeam || south4 == career.myTeam);
        playoffExtra = winner;
        descensoExtra = (winner == north4) ? south4 : north4;
    } else if (north4 || south4) {
        playoffExtra = north4 ? north4 : south4;
    }
    if (playoffExtra &&
        find(playoffTeams.begin(), playoffTeams.end(), playoffExtra) == playoffTeams.end()) {
        playoffTeams.push_back(playoffExtra);
    }

    vector<Team*> descensoTeams;
    for (int pos = 5; pos <= 7; ++pos) {
        if (Team* t = teamAtPos(north, pos)) descensoTeams.push_back(t);
        if (Team* t = teamAtPos(south, pos)) descensoTeams.push_back(t);
    }
    if (descensoExtra) descensoTeams.push_back(descensoExtra);

    vector<Team*> playoffSeeds = playoffTeams;
    if (!playoffSeeds.empty()) {
        LeagueTable seedTable;
        for (auto* t : playoffSeeds) seedTable.addTeam(t);
        seedTable.sortTable();
        playoffSeeds = seedTable.teams;
    }

    for (auto* team : career.activeTeams) {
        team->resetSeasonStats();
    }

    Team* champion = simulateSegundaPlayoff(playoffSeeds);
    if (champion) {
        cout << "Campeon del playoff: " << champion->name << endl;
    }

    LeagueTable descensoTable;
    if (!descensoTeams.empty()) {
        for (auto* t : descensoTeams) t->resetSeasonStats();
        auto schedule = buildRoundRobinIndexSchedule(static_cast<int>(descensoTeams.size()), false);
        cout << "\n--- Grupo de Descenso ---" << endl;
        int round = 1;
        for (const auto& roundMatches : schedule) {
            cout << "\nFecha Descenso " << round << endl;
            for (const auto& match : roundMatches) {
                Team* home = descensoTeams[match.first];
                Team* away = descensoTeams[match.second];
                bool verbose = (home == career.myTeam || away == career.myTeam);
                adjustCpuTactics(*home, *away, career.myTeam);
                adjustCpuTactics(*away, *home, career.myTeam);
                playMatch(*home, *away, verbose, true);
            }
            for (auto* team : descensoTeams) {
                healInjuries(*team, false);
                recoverFitness(*team, 7);
                applyTrainingPlan(*team);
            }
            round++;
        }
        descensoTable.title = "Grupo Descenso";
        for (auto* t : descensoTeams) descensoTable.addTeam(t);
        descensoTable.sortTable();
        descensoTable.displayTable();
    }

    vector<Team*> relegate;
    if (!descensoTable.teams.empty()) {
        int count = min(2, static_cast<int>(descensoTable.teams.size()));
        for (int i = 0; i < count; ++i) {
            relegate.push_back(descensoTable.teams[descensoTable.teams.size() - 1 - i]);
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> promote;
    if (!higher.empty() && champion) promote.push_back(champion);

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), static_cast<int>(promote.size()));
    vector<Team*> fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), static_cast<int>(relegate.size()));

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 50000;
            t->morale = 60;
        }
    }
    for (auto* t : relegate) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 20000);
            t->morale = 40;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : fromLower) {
        t->division = career.activeDivision;
        t->morale = 55;
    }

    if (!higher.empty() && !promote.empty()) {
        cout << "Ascenso: " << promote.front()->name << endl;
    }
    if (!lower.empty() && !relegate.empty()) {
        cout << "Descensos: ";
        for (size_t i = 0; i < relegate.size(); ++i) {
            if (i) cout << ", ";
            cout << relegate[i]->name;
        }
        cout << endl;
    }
    awardSeasonPrizeMoney(career, relevantCompetitionTable(career));
    recordSeasonHistory(career, champion ? champion->name : "", promote, relegate, "Temporada con playoff de ascenso y grupo de descenso.");
    advanceToNextSeason(career);
}

static void endSeasonPrimeraB(Career& career) {
    cout << "\nFin de temporada (Primera B)!" << endl;
    career.leagueTable.displayTable();

    vector<Team*> table = career.leagueTable.teams;
    vector<Team*> seeded = table;
    Team* champion = nullptr;
    if (!table.empty()) {
        if (table.size() >= 2) {
            int topPts = table[0]->points;
            int tiedCount = 0;
            for (auto* t : table) {
                if (t->points == topPts) tiedCount++;
                else break;
            }
            if (tiedCount >= 2) {
                Team* a = table[0];
                Team* b = table[1];
                cout << "\nFinal por el titulo (empate en puntos):" << endl;
                if (tiedCount > 2) {
                    cout << "Empate multiple reducido por criterios de tabla a: "
                         << a->name << " y " << b->name << endl;
                }
                champion = simulateSingleLegKnockout(a, b, "Final", a == career.myTeam || b == career.myTeam, true);
                if (champion && champion != seeded[0]) {
                    auto it = find(seeded.begin(), seeded.end(), champion);
                    if (it != seeded.end()) {
                        Team* oldFirst = seeded[0];
                        size_t idx = static_cast<size_t>(it - seeded.begin());
                        seeded[idx] = oldFirst;
                        seeded[0] = champion;
                    }
                }
            } else {
                champion = table[0];
            }
        } else {
            champion = table[0];
        }
    }
    if (champion) {
        cout << "Campeon fase regular: " << champion->name << endl;
    }

    Team* liguillaWinner = liguillaAscensoPrimeraB(seeded);

    Team* relegated = nullptr;
    if (!table.empty()) {
        int n = static_cast<int>(table.size());
        int bottomPts = table[n - 1]->points;
        int tied = 0;
        for (int i = n - 1; i >= 0; --i) {
            if (table[i]->points == bottomPts) tied++;
            else break;
        }
        if (tied >= 2 && n >= 2) {
            Team* a = table[n - 1];
            Team* b = table[n - 2];
            cout << "\nDefinicion de descenso (empate en el ultimo lugar):" << endl;
            if (tied > 2) {
                cout << "Empate multiple reducido por criterios de tabla a: "
                     << b->name << " y " << a->name << endl;
            }
            Team* winner = simulateSingleLegKnockout(a, b, "Definicion descenso", a == career.myTeam || b == career.myTeam, true);
            relegated = (winner == a) ? b : a;
        } else {
            relegated = table[n - 1];
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> promote;
    if (!higher.empty() && champion) promote.push_back(champion);
    if (!higher.empty() && liguillaWinner &&
        find(promote.begin(), promote.end(), liguillaWinner) == promote.end()) {
        promote.push_back(liguillaWinner);
    }

    vector<Team*> relegate;
    if (!lower.empty() && relegated) relegate.push_back(relegated);

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), static_cast<int>(promote.size()));
    vector<Team*> fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), static_cast<int>(relegate.size()));

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 50000;
            t->morale = 60;
        }
    }
    for (auto* t : relegate) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 20000);
            t->morale = 40;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : fromLower) {
        t->division = career.activeDivision;
        t->morale = 55;
    }

    if (!higher.empty() && !promote.empty()) {
        cout << "Ascensos: ";
        for (size_t i = 0; i < promote.size(); ++i) {
            if (i) cout << ", ";
            cout << promote[i]->name;
        }
        cout << endl;
    }
    if (!lower.empty() && !relegate.empty()) {
        cout << "Descenso: " << relegate.front()->name << endl;
    }
    awardSeasonPrizeMoney(career, career.leagueTable);
    recordSeasonHistory(career, champion ? champion->name : "", promote, relegate, "Campeon regular y liguilla de ascenso.");
    advanceToNextSeason(career);
}

static void endSeasonTerceraA(Career& career) {
    cout << "\nFin de temporada (Tercera Division A)!" << endl;
    career.leagueTable.displayTable();

    vector<Team*> table = career.leagueTable.teams;
    Team* champion = table.empty() ? nullptr : table.front();
    if (champion) {
        cout << "Ascenso directo a Segunda: " << champion->name << endl;
    }

    Team* playoffWinner = simulateTerceraAPlayoff(table);

    vector<Team*> promotionTeamsA;
    vector<Team*> directRelegated;
    if (table.size() >= 14) {
        promotionTeamsA.push_back(table[12]);
        promotionTeamsA.push_back(table[13]);
    } else if (table.size() >= 12) {
        promotionTeamsA.push_back(table[table.size() - 2]);
        promotionTeamsA.push_back(table.back());
    }
    if (table.size() >= 16) {
        directRelegated.push_back(table[14]);
        directRelegated.push_back(table[15]);
    } else if (table.size() >= 14) {
        directRelegated.push_back(table[table.size() - 2]);
        directRelegated.push_back(table.back());
    }

    TerceraBSeasonOutcome lowerOutcome = inferTerceraBSeasonByValue(career);
    vector<Team*> promotedByPromotion;
    vector<Team*> relegatedByPromotion;
    int promotionTies = min(static_cast<int>(promotionTeamsA.size()), static_cast<int>(lowerOutcome.promotionCandidates.size()));
    for (int i = 0; i < promotionTies; ++i) {
        Team* aTeam = promotionTeamsA[i];
        Team* bTeam = lowerOutcome.promotionCandidates[i];
        cout << "\nPromocion Tercera A/B " << i + 1 << ":" << endl;
        Team* winner = simulateTwoLegAggregateTie(bTeam, aTeam, "Llave de promocion");
        if (winner == bTeam) {
            promotedByPromotion.push_back(bTeam);
            relegatedByPromotion.push_back(aTeam);
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> promote;
    if (!higher.empty() && champion) promote.push_back(champion);
    if (!higher.empty() && playoffWinner &&
        find(promote.begin(), promote.end(), playoffWinner) == promote.end()) {
        promote.push_back(playoffWinner);
    }

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), static_cast<int>(promote.size()));
    vector<Team*> fromLower = lowerOutcome.directPromoted;

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 40000;
            t->morale = 60;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : directRelegated) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 15000);
            t->morale = 40;
        }
    }
    for (auto* t : fromLower) {
        if (!lower.empty()) {
            t->division = career.activeDivision;
            t->morale = 58;
        }
    }
    for (auto* t : relegatedByPromotion) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 15000);
            t->morale = 42;
        }
    }
    for (auto* t : promotedByPromotion) {
        if (!lower.empty()) {
            t->division = career.activeDivision;
            t->morale = 58;
        }
    }

    if (!promote.empty()) {
        cout << "Ascensos a Segunda: ";
        for (size_t i = 0; i < promote.size(); ++i) {
            if (i) cout << ", ";
            cout << promote[i]->name;
        }
        cout << endl;
    }
    if (!directRelegated.empty()) {
        cout << "Descensos directos a Tercera B: ";
        for (size_t i = 0; i < directRelegated.size(); ++i) {
            if (i) cout << ", ";
            cout << directRelegated[i]->name;
        }
        cout << endl;
    }
    if (!fromLower.empty()) {
        cout << "Ascensos directos desde Tercera B: ";
        for (size_t i = 0; i < fromLower.size(); ++i) {
            if (i) cout << ", ";
            cout << fromLower[i]->name;
        }
        cout << endl;
    }
    if (!promotedByPromotion.empty()) {
        cout << "Ascensos por promocion desde Tercera B: ";
        for (size_t i = 0; i < promotedByPromotion.size(); ++i) {
            if (i) cout << ", ";
            cout << promotedByPromotion[i]->name;
        }
        cout << endl;
    }
    if (!relegatedByPromotion.empty()) {
        cout << "Perdieron la promocion y bajan a Tercera B: ";
        for (size_t i = 0; i < relegatedByPromotion.size(); ++i) {
            if (i) cout << ", ";
            cout << relegatedByPromotion[i]->name;
        }
        cout << endl;
    }
    vector<Team*> allRelegated = directRelegated;
    allRelegated.insert(allRelegated.end(), relegatedByPromotion.begin(), relegatedByPromotion.end());
    vector<Team*> allPromoted = promote;
    allPromoted.insert(allPromoted.end(), promotedByPromotion.begin(), promotedByPromotion.end());
    awardSeasonPrizeMoney(career, career.leagueTable);
    recordSeasonHistory(career, champion ? champion->name : "", allPromoted, allRelegated, "Ascenso directo, playoff y promocion interdivisional.");
    advanceToNextSeason(career);
}

static void endSeasonTerceraB(Career& career) {
    cout << "\nFin de temporada (Tercera Division B)!" << endl;
    if (career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.buildRegionalGroups();
    }

    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, "Zona Norte");
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, "Zona Sur");
    north.displayTable();
    south.displayTable();

    TerceraBSeasonOutcome outcome = resolveTerceraBSeason(north.teams, south.teams, true);
    TerceraARelegationOutcome higherOutcome = getInactiveTerceraARelegationByValue(career);

    vector<Team*> promotedByPromotion;
    vector<Team*> relegatedByPromotion;
    int promotionTies = min(static_cast<int>(higherOutcome.promotionTeams.size()), static_cast<int>(outcome.promotionCandidates.size()));
    for (int i = 0; i < promotionTies; ++i) {
        Team* aTeam = higherOutcome.promotionTeams[i];
        Team* bTeam = outcome.promotionCandidates[i];
        cout << "\nPromocion Tercera A/B " << i + 1 << ":" << endl;
        Team* winner = simulateTwoLegAggregateTie(bTeam, aTeam, "Llave de promocion");
        if (winner == bTeam) {
            promotedByPromotion.push_back(bTeam);
            relegatedByPromotion.push_back(aTeam);
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";

    for (auto* t : outcome.directPromoted) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 25000;
            t->morale = 60;
        }
    }
    for (auto* t : higherOutcome.directRelegated) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : promotedByPromotion) {
        if (!higher.empty()) {
            t->division = higher;
            t->morale = 58;
        }
    }
    for (auto* t : relegatedByPromotion) {
        t->division = career.activeDivision;
        t->morale = 45;
    }

    if (!outcome.directPromoted.empty()) {
        cout << "Ascensos directos a Tercera A: ";
        for (size_t i = 0; i < outcome.directPromoted.size(); ++i) {
            if (i) cout << ", ";
            cout << outcome.directPromoted[i]->name;
        }
        cout << endl;
    }
    if (!promotedByPromotion.empty()) {
        cout << "Ascensos por promocion a Tercera A: ";
        for (size_t i = 0; i < promotedByPromotion.size(); ++i) {
            if (i) cout << ", ";
            cout << promotedByPromotion[i]->name;
        }
        cout << endl;
    }
    vector<Team*> allPromoted = outcome.directPromoted;
    allPromoted.insert(allPromoted.end(), promotedByPromotion.begin(), promotedByPromotion.end());
    awardSeasonPrizeMoney(career, relevantCompetitionTable(career));
    recordSeasonHistory(career, outcome.champion ? outcome.champion->name : "", allPromoted, {}, "Campeones zonales, final y promocion por playoff.");
    advanceToNextSeason(career);
}

void endSeason(Career& career) {
    const CompetitionConfig& config = getCompetitionConfig(career.activeDivision);
    switch (config.seasonHandler) {
        case CompetitionSeasonHandler::SegundaGroups:
            endSeasonSegundaDivision(career);
            return;
        case CompetitionSeasonHandler::PrimeraB:
            endSeasonPrimeraB(career);
            return;
        case CompetitionSeasonHandler::TerceraA:
            endSeasonTerceraA(career);
            return;
        case CompetitionSeasonHandler::TerceraB:
            endSeasonTerceraB(career);
            return;
        default:
            break;
    }
    cout << "\nFin de temporada!" << endl;
    career.leagueTable.displayTable();
    Team* champion = nullptr;
    if (!career.leagueTable.teams.empty()) {
        if (config.seasonHandler == CompetitionSeasonHandler::PrimeraDivision &&
            career.leagueTable.teams.size() >= 2) {
            int topPts = career.leagueTable.teams.front()->points;
            int tiedCount = 0;
            for (auto* t : career.leagueTable.teams) {
                if (t->points == topPts) tiedCount++;
                else break;
            }
            if (tiedCount >= 2) {
                Team* a = career.leagueTable.teams[0];
                Team* b = career.leagueTable.teams[1];
                cout << "\nFinal por el titulo (empate en puntos):" << endl;
                if (tiedCount > 2) {
                    cout << "Empate multiple reducido por criterios de tabla a: "
                         << a->name << " y " << b->name << endl;
                }
                champion = simulateSingleLegKnockout(a, b, "Final", a == career.myTeam || b == career.myTeam, true);
            } else {
                champion = career.leagueTable.teams.front();
            }
        } else {
            champion = career.leagueTable.teams.front();
        }
    }
    if (champion) {
        cout << "Campeon: " << champion->name << endl;
    }
    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> table = career.leagueTable.teams;
    int n = static_cast<int>(table.size());
    vector<Team*> promote;
    vector<Team*> relegate;
    int promoteSlots = higher.empty() ? 0 : 2;
    int relegateSlots = lower.empty() ? 0 : 2;

    if (promoteSlots > 0) {
        int count = min(promoteSlots, n);
        for (int i = 0; i < count; ++i) {
            promote.push_back(table[i]);
        }
    }

    int actualPromote = static_cast<int>(promote.size());
    int relegateCount = min(relegateSlots, max(0, n - actualPromote));
    for (int i = 0; i < relegateCount; ++i) {
        relegate.push_back(table[n - 1 - i]);
    }

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), actualPromote);
    vector<Team*> fromLower;
    if (!lower.empty() &&
        config.seasonHandler == CompetitionSeasonHandler::PrimeraDivision &&
        getCompetitionConfig(lower).seasonHandler == CompetitionSeasonHandler::PrimeraB) {
        auto pbTable = sortByValue(career.getDivisionTeams(lower));
        if (!pbTable.empty()) {
            Team* pbChampion = pbTable[0];
            Team* pbPlayoff = liguillaAscensoPrimeraB(pbTable);
            if (pbChampion) fromLower.push_back(pbChampion);
            if (pbPlayoff && pbPlayoff != pbChampion) fromLower.push_back(pbPlayoff);
            for (size_t i = 1; i < pbTable.size() && static_cast<int>(fromLower.size()) < relegateCount; ++i) {
                Team* candidate = pbTable[i];
                if (find(fromLower.begin(), fromLower.end(), candidate) == fromLower.end()) {
                    fromLower.push_back(candidate);
                }
            }
        }
    } else {
        fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), relegateCount);
    }

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 50000;
            t->morale = 60;
        }
    }
    for (auto* t : relegate) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 20000);
            t->morale = 40;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : fromLower) {
        t->division = career.activeDivision;
        t->morale = 55;
    }

    if (!higher.empty() && !promote.empty()) {
        cout << "Ascensos: ";
        for (size_t i = 0; i < promote.size(); ++i) {
            if (i) cout << ", ";
            cout << promote[i]->name;
        }
        cout << endl;
    }
    if (!lower.empty() && !relegate.empty()) {
        cout << "Descensos: ";
        for (size_t i = 0; i < relegate.size(); ++i) {
            if (i) cout << ", ";
            cout << relegate[i]->name;
        }
        cout << endl;
    }
    awardSeasonPrizeMoney(career, career.leagueTable);
    recordSeasonHistory(career, champion ? champion->name : "", promote, relegate, "Cierre de temporada regular.");
    advanceToNextSeason(career);
}

void simulateCareerWeek(Career& career) {
    if (career.activeTeams.empty() || career.schedule.empty()) {
        emitUiMessage("No hay calendario disponible.");
        return;
    }
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        endSeason(career);
        return;
    }

    emitUiMessage("");
    emitUiMessage("Simulando semana " + to_string(career.currentWeek) + "...");
    career.leagueTable.sortTable();
    LeagueTable northTable;
    LeagueTable southTable;
    bool useGroups = usesGroupFormat(career);
    if (useGroups) {
        northTable = buildGroupTable(career, career.groupNorthIdx, regionalGroupTitle(career, true));
        southTable = buildGroupTable(career, career.groupSouthIdx, regionalGroupTitle(career, false));
    }
    const auto& matches = career.schedule[career.currentWeek - 1];
    vector<vector<int>> suspensionsBefore;
    suspensionsBefore.reserve(career.activeTeams.size());
    for (auto* team : career.activeTeams) {
        vector<int> snapshot;
        snapshot.reserve(team->players.size());
        for (const auto& player : team->players) snapshot.push_back(player.matchesSuspended);
        suspensionsBefore.push_back(snapshot);
    }
    vector<int> pointsBefore;
    pointsBefore.reserve(career.activeTeams.size());
    for (auto* t : career.activeTeams) pointsBefore.push_back(t->points);
    int myTeamPointsDelta = 0;
    for (const auto& match : matches) {
        Team* home = career.activeTeams[match.first];
        Team* away = career.activeTeams[match.second];
        bool verbose = (home == career.myTeam || away == career.myTeam);
        adjustCpuTactics(*home, *away, career.myTeam);
        adjustCpuTactics(*away, *home, career.myTeam);
        bool key = false;
        if (useGroups) {
            int group = groupForTeam(career, home);
            if (group == groupForTeam(career, away) && group == 0) {
                key = isKeyMatch(northTable, home, away);
            } else if (group == groupForTeam(career, away) && group == 1) {
                key = isKeyMatch(southTable, home, away);
            } else {
                key = isKeyMatch(career.leagueTable, home, away);
            }
        } else {
            key = isKeyMatch(career.leagueTable, home, away);
        }
        if (verbose && key) {
            emitUiMessage("[Aviso] Partido clave de la semana.");
        }
        MatchResult result = playMatch(*home, *away, verbose, key);
        storeMatchAnalysis(career, *home, *away, result, false);
    }
    for (const auto& division : career.divisions) {
        simulateBackgroundDivisionWeek(career, division.id);
    }
    bool cupWeek = career.cupActive &&
                   (career.currentWeek == 1 || career.currentWeek % 4 == 0 ||
                    career.currentWeek == static_cast<int>(career.schedule.size()));
    if (cupWeek) {
        simulateSeasonCupRound(career);
    }
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        const auto& snapshot = suspensionsBefore[i];
        size_t limit = min(snapshot.size(), team->players.size());
        for (size_t j = 0; j < limit; ++j) {
            if (snapshot[j] > 0 && team->players[j].matchesSuspended > 0) {
                team->players[j].matchesSuspended--;
            }
        }
    }
    for (auto& team : career.allTeams) {
        healInjuries(team, false);
        recoverFitness(team, 7);
        applyTrainingPlan(team);
    }
    updateContracts(career);
    processIncomingOffers(career);
    processCpuTransfers(career);
    processLoanReturns(career);
    applyWeeklyFinances(career, pointsBefore);
    career.leagueTable.sortTable();
    if (career.myTeam) {
        for (size_t i = 0; i < career.activeTeams.size(); ++i) {
            if (career.activeTeams[i] == career.myTeam) {
                myTeamPointsDelta = career.myTeam->points - pointsBefore[i];
                break;
            }
        }
        if (career.boardMonthlyObjective.find("puntos") != string::npos) {
            career.boardMonthlyProgress += myTeamPointsDelta;
        }
        updateSquadDynamics(career, myTeamPointsDelta);
    }
    runMonthlyDevelopment(career);
    updateShortlistAlerts(career);
    career.updateDynamicObjectiveStatus();
    career.updateBoardConfidence();
    updateManagerReputation(career);
    weeklyDashboard(career);
    applyClubEvent(career);
    if (career.myTeam) {
        if (myTeamPointsDelta == 3) career.addNews(career.myTeam->name + " gana en la fecha " + to_string(career.currentWeek) + ".");
        else if (myTeamPointsDelta == 1) career.addNews(career.myTeam->name + " empata en la fecha " + to_string(career.currentWeek) + ".");
        else career.addNews(career.myTeam->name + " pierde en la fecha " + to_string(career.currentWeek) + ".");
        if (career.boardWarningWeeks >= 4) {
            career.addNews("La directiva aumenta la presion sobre " + career.myTeam->name + ".");
        }
        generateWeeklyNarratives(career, myTeamPointsDelta);
    }
    handleManagerStatus(career);
    career.currentWeek++;
    checkAchievements(career);
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        endSeason(career);
    }
}

void playCupMode(Career& career) {
    if (career.divisions.empty()) {
        cout << "No hay divisiones disponibles." << endl;
        return;
    }
    cout << "\nSelecciona la division para Copa:" << endl;
    for (size_t i = 0; i < career.divisions.size(); ++i) {
        cout << i + 1 << ". " << career.divisions[i].display << endl;
    }
    int divisionChoice = readInt("Elige una division: ", 1, static_cast<int>(career.divisions.size()));
    string divisionId = career.divisions[divisionChoice - 1].id;
    auto teams = career.getDivisionTeams(divisionId);
    if (teams.size() < 2) {
        cout << "No hay suficientes equipos para una copa." << endl;
        return;
    }
    cout << "\nElige un equipo para seguir (0 para ninguno):" << endl;
    for (size_t i = 0; i < teams.size(); ++i) {
        cout << i + 1 << ". " << teams[i]->name << endl;
    }
    int followChoice = readInt("Equipo: ", 0, static_cast<int>(teams.size()));
    int followIdx = (followChoice == 0) ? -1 : (followChoice - 1);

    vector<Team> cupTeams;
    cupTeams.reserve(teams.size());
    for (auto* t : teams) cupTeams.push_back(*t);

    vector<int> alive;
    for (int i = 0; i < static_cast<int>(cupTeams.size()); ++i) alive.push_back(i);
    int round = 1;
    while (alive.size() > 1) {
        cout << "\n--- Copa: Ronda " << round << " ---" << endl;
        vector<int> next;
        if (alive.size() % 2 == 1) {
            int bye = alive.back();
            alive.pop_back();
            next.push_back(bye);
            cout << "Pase directo: " << cupTeams[bye].name << endl;
        }
        for (size_t i = 0; i < alive.size(); i += 2) {
            int aIdx = alive[i];
            int bIdx = alive[i + 1];
            Team& a = cupTeams[aIdx];
            Team& b = cupTeams[bIdx];
            bool verbose = (followIdx == aIdx || followIdx == bIdx);
            cout << a.name << " vs " << b.name << endl;
            MatchResult r = playMatch(a, b, verbose, true);
            int winner = aIdx;
            if (r.homeGoals < r.awayGoals) winner = bIdx;
            else if (r.homeGoals == r.awayGoals) {
                winner = (randInt(0, 1) == 0) ? aIdx : bIdx;
                cout << "Gana por penales: " << cupTeams[winner].name << endl;
            }
            next.push_back(winner);
        }
        alive.swap(next);
        round++;
    }
    cout << "\nCampeon de la Copa: " << cupTeams[alive.front()].name << endl;
}
