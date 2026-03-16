#include "ui/career_reports_ui.h"

#include "career/app_services.h"
#include "career/career_reports.h"
#include "career/career_support.h"
#include "utils/utils.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace {

void printMessages(const ServiceResult& result) {
    for (const auto& message : result.messages) {
        cout << message << endl;
    }
}

void printReport(const CareerReport& report) {
    cout << "\n" << formatCareerReport(report);
}

}  // namespace

namespace ui_reports {

void displayCompetitionCenter(Career& career) {
    if (!career.myTeam) return;
    printReport(buildCompetitionReport(career));
}

void displayBoardStatus(Career& career) {
    if (!career.myTeam) return;

    printReport(buildBoardReport(career));
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
             << " | Valor plantel " << formatMoneyValue(jobs[i]->getSquadValue()) << endl;
    }
    int choice = readInt("Club (0 para cancelar): ", 0, static_cast<int>(jobs.size()));
    if (choice == 0) return;

    ServiceResult result = takeManagerJobService(career, jobs[static_cast<size_t>(choice - 1)]->name);
    printMessages(result);
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

    printReport(buildClubReport(career));
    cout << "\n1. Mejorar estadio" << endl;
    cout << "2. Mejorar cantera" << endl;
    cout << "3. Mejorar centro de entrenamiento" << endl;
    cout << "4. Contratar asistente tecnico" << endl;
    cout << "5. Contratar preparador fisico" << endl;
    cout << "6. Contratar jefe de scouting" << endl;
    cout << "7. Contratar jefe de juveniles" << endl;
    cout << "8. Mejorar cuerpo medico" << endl;
    cout << "9. Contratar entrenador de arqueros" << endl;
    cout << "10. Contratar analista de rendimiento" << endl;
    cout << "11. Cambiar region juvenil" << endl;
    cout << "12. Volver" << endl;
    int choice = readInt("Elige opcion: ", 1, 12);
    if (choice == 12) return;

    ServiceResult result;
    switch (choice) {
        case 1: result = upgradeClubService(career, ClubUpgrade::Stadium); break;
        case 2: result = upgradeClubService(career, ClubUpgrade::Youth); break;
        case 3: result = upgradeClubService(career, ClubUpgrade::Training); break;
        case 4: result = upgradeClubService(career, ClubUpgrade::AssistantCoach); break;
        case 5: result = upgradeClubService(career, ClubUpgrade::FitnessCoach); break;
        case 6: result = upgradeClubService(career, ClubUpgrade::Scouting); break;
        case 7: result = upgradeClubService(career, ClubUpgrade::YouthCoach); break;
        case 8: result = upgradeClubService(career, ClubUpgrade::Medical); break;
        case 9: result = upgradeClubService(career, ClubUpgrade::GoalkeepingCoach); break;
        case 10: result = upgradeClubService(career, ClubUpgrade::PerformanceAnalyst); break;
        case 11: {
            vector<string> regions = listYouthRegionsService();
            for (size_t i = 0; i < regions.size(); ++i) {
                cout << i + 1 << ". " << regions[i] << endl;
            }
            int regionChoice = readInt("Nueva region juvenil: ", 1, static_cast<int>(regions.size()));
            result = changeYouthRegionService(career, regions[static_cast<size_t>(regionChoice - 1)]);
            break;
        }
    }
    printMessages(result);
}

}  // namespace ui_reports
