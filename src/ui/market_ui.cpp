#include "ui.h"

#include "career/app_services.h"
#include "competition.h"
#include "transfers/negotiation_system.h"
#include "utils.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace {

vector<pair<Team*, int>> buildTransferPool(Career& career, const string& filterPos, bool includeClauseTargets) {
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
        const Player& pa = a.first->players[static_cast<size_t>(a.second)];
        const Player& pb = b.first->players[static_cast<size_t>(b.second)];
        if (pa.skill != pb.skill) return pa.skill > pb.skill;
        if (pa.value != pb.value) return pa.value < pb.value;
        return pa.name < pb.name;
    });
    if (pool.size() > 25) pool.resize(25);
    return pool;
}

void printServiceResult(const ServiceResult& result) {
    for (const auto& message : result.messages) {
        cout << message << endl;
    }
}

NegotiationProfile promptNegotiationProfile() {
    cout << "Perfil de negociacion:" << endl;
    cout << "1. Seguro" << endl;
    cout << "2. Balanceado" << endl;
    cout << "3. Agresivo" << endl;
    int choice = readInt("Elige perfil: ", 1, 3);
    if (choice == 1) return NegotiationProfile::Safe;
    if (choice == 3) return NegotiationProfile::Aggressive;
    return NegotiationProfile::Balanced;
}

NegotiationPromise promptNegotiationPromise() {
    cout << "Promesa al jugador:" << endl;
    cout << "1. Sin promesa" << endl;
    cout << "2. Titular" << endl;
    cout << "3. Rotacion" << endl;
    cout << "4. Proyecto" << endl;
    int choice = readInt("Elige promesa: ", 1, 4);
    if (choice == 2) return NegotiationPromise::Starter;
    if (choice == 3) return NegotiationPromise::Rotation;
    if (choice == 4) return NegotiationPromise::Prospect;
    return NegotiationPromise::None;
}

void buyFromClub(Career& career) {
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
        const Player& player = seller->players[static_cast<size_t>(pool[i].second)];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << seller->name << ")"
             << " Hab " << player.skill << " | Valor $" << player.value
             << " | Pie " << player.preferredFoot
             << " | Forma " << playerFormLabel(player)
             << " | Clausula $" << player.releaseClause << endl;
    }
    int choice = readInt("Elige jugador (0 para cancelar): ", 0, static_cast<int>(pool.size()));
    if (choice == 0) return;

    Team* seller = pool[static_cast<size_t>(choice - 1)].first;
    const Player& target = seller->players[static_cast<size_t>(pool[static_cast<size_t>(choice - 1)].second)];
    NegotiationProfile profile = promptNegotiationProfile();
    NegotiationPromise promise = promptNegotiationPromise();
    printServiceResult(buyTransferTargetService(career, seller->name, target.name, profile, promise));
}

void triggerReleaseClause(Career& career) {
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
        const Player& player = seller->players[static_cast<size_t>(pool[i].second)];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << seller->name << ")"
             << " Hab " << player.skill << " | Clausula $" << player.releaseClause
             << " | Salario $" << player.wage << endl;
    }
    int choice = readInt("Ejecutar clausula de (0 para cancelar): ", 0, static_cast<int>(pool.size()));
    if (choice == 0) return;

    Team* seller = pool[static_cast<size_t>(choice - 1)].first;
    const Player& target = seller->players[static_cast<size_t>(pool[static_cast<size_t>(choice - 1)].second)];
    NegotiationProfile profile = promptNegotiationProfile();
    NegotiationPromise promise = promptNegotiationPromise();
    printServiceResult(triggerReleaseClauseService(career, seller->name, target.name, profile, promise));
}

void signPreContractUi(Career& career) {
    if (!career.myTeam) return;
    vector<pair<Team*, int>> pool = buildTransferPool(career, "", true);
    vector<pair<Team*, int>> eligible;
    for (const auto& entry : pool) {
        const Player& player = entry.first->players[static_cast<size_t>(entry.second)];
        if (player.contractWeeks > 12) continue;
        if (player.onLoan) continue;
        eligible.push_back(entry);
    }
    if (eligible.empty()) {
        cout << "No hay jugadores elegibles para precontrato." << endl;
        return;
    }

    cout << "\nJugadores elegibles para precontrato:" << endl;
    for (size_t i = 0; i < eligible.size(); ++i) {
        const Player& player = eligible[i].first->players[static_cast<size_t>(eligible[i].second)];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << eligible[i].first->name << ")"
             << " Hab " << player.skill << " | Contrato restante " << player.contractWeeks << " sem" << endl;
    }
    int choice = readInt("Jugador (0 para cancelar): ", 0, static_cast<int>(eligible.size()));
    if (choice == 0) return;

    Team* seller = eligible[static_cast<size_t>(choice - 1)].first;
    const Player& target = seller->players[static_cast<size_t>(eligible[static_cast<size_t>(choice - 1)].second)];
    NegotiationProfile profile = promptNegotiationProfile();
    NegotiationPromise promise = promptNegotiationPromise();
    printServiceResult(signPreContractService(career, seller->name, target.name, profile, promise));
}

void loanInPlayerUi(Career& career) {
    if (!career.myTeam) return;
    auto pool = buildTransferPool(career, "", false);
    vector<pair<Team*, int>> loanable;
    for (const auto& entry : pool) {
        const Player& player = entry.first->players[static_cast<size_t>(entry.second)];
        if (player.contractWeeks <= 12) continue;
        loanable.push_back(entry);
    }
    if (loanable.empty()) {
        cout << "No hay jugadores disponibles para prestamo." << endl;
        return;
    }

    cout << "\nJugadores disponibles a prestamo:" << endl;
    for (size_t i = 0; i < loanable.size(); ++i) {
        const Player& player = loanable[i].first->players[static_cast<size_t>(loanable[i].second)];
        cout << i + 1 << ". " << player.name << " (" << player.position << ", " << loanable[i].first->name << ")"
             << " Hab " << player.skill << " | Valor $" << player.value << endl;
    }
    int choice = readInt("Jugador (0 para cancelar): ", 0, static_cast<int>(loanable.size()));
    if (choice == 0) return;

    Team* seller = loanable[static_cast<size_t>(choice - 1)].first;
    const Player& target = seller->players[static_cast<size_t>(loanable[static_cast<size_t>(choice - 1)].second)];
    int loanWeeks = readInt("Duracion del prestamo (8-26 semanas): ", 8, 26);
    printServiceResult(loanInPlayerService(career, seller->name, target.name, loanWeeks));
}

void loanOutPlayerUi(Career& career) {
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
        const Player& player = career.myTeam->players[static_cast<size_t>(candidates[i])];
        cout << i + 1 << ". " << player.name << " (" << player.position << ")"
             << " Hab " << player.skill << " | Edad " << player.age << endl;
    }
    int playerChoice = readInt("Jugador (0 para cancelar): ", 0, static_cast<int>(candidates.size()));
    if (playerChoice == 0) return;

    vector<Team*> destinations;
    for (auto* team : career.activeTeams) {
        if (!team || team == career.myTeam) continue;
        int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        if (maxSquad <= 0 || static_cast<int>(team->players.size()) < maxSquad) destinations.push_back(team);
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
    const Player& target = career.myTeam->players[static_cast<size_t>(candidates[static_cast<size_t>(playerChoice - 1)])];
    printServiceResult(loanOutPlayerService(career,
                                            target.name,
                                            destinations[static_cast<size_t>(clubChoice - 1)]->name,
                                            loanWeeks));
}

void sellPlayerUi(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->players.size() <= 18) {
        cout << "No puedes vender. Debes mantener al menos 18 jugadores." << endl;
        return;
    }
    cout << "Selecciona jugador para vender:" << endl;
    for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
        const Player& player = career.myTeam->players[i];
        cout << i + 1 << ". " << player.name << " (" << player.position << ")"
             << " Valor $" << player.value << " | Clausula $" << player.releaseClause
             << " | Salario $" << player.wage << endl;
    }
    int idx = readInt("Elige jugador: ", 1, static_cast<int>(career.myTeam->players.size())) - 1;
    printServiceResult(sellPlayerService(career, career.myTeam->players[static_cast<size_t>(idx)].name));
}

}  // namespace

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

    if (choice == 1) buyFromClub(career);
    else if (choice == 2) triggerReleaseClause(career);
    else if (choice == 3) signPreContractUi(career);
    else if (choice == 4) loanInPlayerUi(career);
    else if (choice == 5) loanOutPlayerUi(career);
    else if (choice == 6) sellPlayerUi(career);
}

void scoutPlayers(Career& career) {
    if (!career.myTeam) return;
    cout << "\n=== Ojeo de Jugadores ===" << endl;
    cout << "Presupuesto: $" << career.myTeam->budget << endl;
    cout << "Jefe de scouting: " << career.myTeam->scoutingChief << "/99" << endl;
    cout << "Asignaciones activas: " << career.scoutingAssignments.size() << endl;
    cout << "1. Otear jugadores ahora" << endl;
    cout << "2. Crear asignacion de scouting" << endl;
    cout << "3. Ver asignaciones activas" << endl;
    cout << "4. Ver informes guardados" << endl;
    cout << "5. Volver" << endl;
    int choice = readInt("Elige una opcion: ", 1, 5);

    if (choice == 3) {
        if (career.scoutingAssignments.empty()) {
            cout << "No hay asignaciones activas." << endl;
            return;
        }
        for (const auto& assignment : career.scoutingAssignments) {
            cout << "- " << assignment.region
                 << " | foco " << assignment.focusPosition
                 << " | prioridad " << assignment.priority
                 << " | conocimiento " << assignment.knowledgeLevel << "%"
                 << " | resta " << assignment.weeksRemaining << " sem" << endl;
        }
        return;
    }
    if (choice == 4) {
        if (career.scoutInbox.empty()) {
            cout << "No hay informes guardados." << endl;
            return;
        }
        for (const auto& note : career.scoutInbox) {
            cout << "- " << note << endl;
        }
        return;
    }
    if (choice == 5) return;

    cout << "Regiones: 1. Metropolitana 2. Norte 3. Centro 4. Sur 5. Patagonia 6. Todas" << endl;
    vector<string> regionOptions = {"Metropolitana", "Norte", "Centro", "Sur", "Patagonia", "Todas"};
    int regionChoice = readInt("Elegir region: ", 1, static_cast<int>(regionOptions.size()));
    string focusPos = normalizePosition(readLine("Foco de posicion (ARQ/DEF/MED/DEL o Enter para necesidad): "));
    if (focusPos == "N/A") focusPos.clear();

    if (choice == 2) {
        int durationWeeks = readInt("Duracion de seguimiento (2-6 semanas): ", 2, 6);
        printServiceResult(createScoutingAssignmentService(career,
                                                           regionOptions[static_cast<size_t>(regionChoice - 1)],
                                                           focusPos,
                                                           durationWeeks));
        return;
    }

    ScoutingSessionResult session = runScoutingSessionService(career, regionOptions[static_cast<size_t>(regionChoice - 1)], focusPos);
    printServiceResult(session.service);
    if (!session.service.ok || session.candidates.empty()) return;

    cout << "\n1. Agregar objetivo a shortlist" << endl;
    cout << "2. Intentar fichar un objetivo" << endl;
    cout << "3. Volver" << endl;
    int nextAction = readInt("Elige opcion: ", 1, 3);
    if (nextAction == 3) return;

    int targetIndex = readInt("Elige objetivo: ", 1, static_cast<int>(session.candidates.size())) - 1;
    const ScoutingCandidate& candidate = session.candidates[static_cast<size_t>(targetIndex)];
    if (nextAction == 1) {
        printServiceResult(shortlistPlayerService(career, candidate.clubName, candidate.playerName));
        return;
    }

    NegotiationProfile profile = promptNegotiationProfile();
    NegotiationPromise promise = promptNegotiationPromise();
    printServiceResult(buyTransferTargetService(career, candidate.clubName, candidate.playerName, profile, promise));
}
