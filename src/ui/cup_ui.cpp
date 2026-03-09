#include "ui.h"

#include "simulation.h"
#include "utils.h"

#include <iostream>

using namespace std;

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
    string divisionId = career.divisions[static_cast<size_t>(divisionChoice - 1)].id;
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
    for (auto* team : teams) cupTeams.push_back(*team);

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
            cout << "Pase directo: " << cupTeams[static_cast<size_t>(bye)].name << endl;
        }
        for (size_t i = 0; i < alive.size(); i += 2) {
            int homeIndex = alive[i];
            int awayIndex = alive[i + 1];
            Team& home = cupTeams[static_cast<size_t>(homeIndex)];
            Team& away = cupTeams[static_cast<size_t>(awayIndex)];
            bool verbose = (followIdx == homeIndex || followIdx == awayIndex);
            cout << home.name << " vs " << away.name << endl;
            MatchResult result = playMatch(home, away, verbose, true);
            int winner = homeIndex;
            if (result.homeGoals < result.awayGoals) {
                winner = awayIndex;
            } else if (result.homeGoals == result.awayGoals) {
                winner = (randInt(0, 1) == 0) ? homeIndex : awayIndex;
                cout << "Gana por penales: " << cupTeams[static_cast<size_t>(winner)].name << endl;
            }
            next.push_back(winner);
        }
        alive.swap(next);
        round++;
    }

    cout << "\nCampeon de la Copa: " << cupTeams[static_cast<size_t>(alive.front())].name << endl;
}
