#include "gui/gui_internal.h"

#ifdef _WIN32

#include "ai/ai_squad_planner.h"
#include "career/analytics_service.h"
#include "career/inbox_service.h"
#include "ai/ai_transfer_manager.h"
#include "career/dressing_room_service.h"
#include "career/match_analysis_store.h"
#include "career/match_center_service.h"
#include "career/career_support.h"
#include "development/training_impact_system.h"
#include "finance/finance_system.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace gui_win32 {

namespace {

struct TransferPreviewItem {
    std::string player;
    std::string position;
    int age = 0;
    int skill = 0;
    int potential = 0;
    long long expectedFee = 0;
    long long expectedWage = 0;
    long long expectedAgentFee = 0;
    std::string club;
    std::string scoutingLabel;
    std::string marketLabel;
    std::string expectedRole;
    std::string scoutingNote;
    bool onShortlist = false;
    bool urgentNeed = false;
    int scoutingConfidence = 0;
    int affordabilityScore = 0;
    double totalScore = 0.0;
};

std::string pageTitleFor(GuiPage page) {
    switch (page) {
        case GuiPage::Dashboard: return "Resumen del club";
        case GuiPage::Squad: return "Plantilla";
        case GuiPage::Tactics: return "Tacticas";
        case GuiPage::Calendar: return "Calendario";
        case GuiPage::League: return "Liga";
        case GuiPage::Transfers: return "Fichajes";
        case GuiPage::Finances: return "Finanzas";
        case GuiPage::Youth: return "Cantera";
        case GuiPage::Board: return "Directiva";
        case GuiPage::News: return "Noticias";
    }
    return "Resumen del club";
}

std::string breadcrumbFor(GuiPage page) {
    return "Club > " + pageTitleFor(page);
}

std::string friendlyPanelTitle(const std::string& title) {
    if (title == "DashboardPanel") return "Centro del club";
    if (title == "LeagueTableView") return "Clasificacion";
    if (title == "MatchSummaryPanel") return "Proximo encuentro";
    if (title == "InjuryListWidget") return "Lesiones";
    if (title == "FixtureListView") return "Calendario";
    if (title == "UpcomingMatchWidget") return "Proximo encuentro";
    if (title == "LastResultPanel") return "Ultimo partido";
    if (title == "TeamStatusPanel") return "Estado del plantel";
    if (title == "LeaguePositionWidget") return "Posicion competitiva";
    if (title == "TeamMoraleWidget") return "Estado del vestuario";
    if (title == "PlayerTableView") return "Plantilla";
    if (title == "YouthPlayerTableView") return "Cantera";
    if (title == "PlayerProfilePanel") return "Ficha del jugador";
    if (title == "PlayerComparisonPanel") return "Comparador";
    if (title == "SquadSummaryPanel") return "Resumen de plantilla";
    if (title == "YouthSummaryPanel") return "Resumen de cantera";
    if (title == "PlayerDevelopmentPanel") return "Desarrollo";
    if (title == "SquadUnitOverview") return "Balance por lineas";
    if (title == "TacticalSummary") return "Resumen tactico";
    if (title == "TacticsBoard") return "Disposicion en el campo";
    if (title == "FormationSelector") return "Once titular";
    if (title == "TacticalImpactSummary") return "Impacto tactico";
    if (title == "MatchAnalysisPanel") return "Informe tactico";
    if (title == "CalendarSummary") return "Resumen del calendario";
    if (title == "CupAndSeasonNotes") return "Competiciones";
    if (title == "SeasonHistory") return "Historial de temporada";
    if (title == "SeasonFlowController") return "Resumen semanal";
    if (title == "CompetitionSummary") return "Resumen competitivo";
    if (title == "RaceContext") return "Contexto de la tabla";
    if (title == "SeasonRecords") return "Historial reciente";
    if (title == "CompetitionReport") return "Informe de liga";
    if (title == "TransferSearchPanel") return "Radar de mercado";
    if (title == "TransferMarketView") return "Mercado de fichajes";
    if (title == "TransferTargetCard") return "Objetivo observado";
    if (title == "TransferPipeline") return "Negociaciones y movimientos";
    if (title == "FinanceSummary") return "Resumen financiero";
    if (title == "FinanceBreakdown") return "Balance economico";
    if (title == "SalaryTable") return "Masa salarial";
    if (title == "Infrastructure") return "Infraestructura";
    if (title == "ClubFinancePanel") return "Panel financiero";
    if (title == "BoardObjectives") return "Objetivos de directiva";
    if (title == "BoardObjectiveTable") return "Seguimiento de objetivos";
    if (title == "ClubProfile") return "Perfil del club";
    if (title == "CoachHistory") return "Historial del entrenador";
    if (title == "BoardReport") return "Informe de directiva";
    if (title == "NewsFeedPanel") return "Titulares del mundo";
    if (title == "NewsCardList") return "Titulares";
    if (title == "ScoutingInbox") return "Centro del manager";
    if (title == "NewsDetail") return "Detalle";
    if (title == "AlertPanel") return "Alertas";
    if (title == "AlertPanel / NewsFeedPanel") return "Pulso del club";
    return title;
}

std::vector<std::string> filterOptionsForPage(GuiPage page) {
    switch (page) {
        case GuiPage::Dashboard: return {"Todo", "Alertas", "Lesiones", "Contratos"};
        case GuiPage::Squad: return {"Todos", "XI", "ARQ", "DEF", "MED", "DEL", "Lesionados"};
        case GuiPage::Tactics: return {"Balanceado", "Presion alta", "Bloque bajo"};
        case GuiPage::Calendar: return {"Proximos 5", "Proximos 10", "Toda la fase"};
        case GuiPage::League: return {"Grupo actual", "Tabla general"};
        case GuiPage::Transfers: return {"Todos", "ARQ", "DEF", "MED", "DEL", "Sub-23", "Potencial 75+", "Precio accesible", "Salario bajo"};
        case GuiPage::Finances: return {"Resumen", "Salarios", "Infraestructura"};
        case GuiPage::Youth: return {"Todos", "Sub-21", "Elite", "Cedidos"};
        case GuiPage::Board: return {"Resumen", "Objetivos", "Historial"};
        case GuiPage::News: return {"Todo", "Alertas", "Mercado", "Resultados"};
    }
    return {"Todo"};
}

bool isCongestedWeek(const Career& career) {
    return career.cupActive && career.currentWeek >= 1 &&
           (career.currentWeek == 1 || career.currentWeek % 4 == 0 ||
            career.currentWeek == static_cast<int>(career.schedule.size()));
}

std::string trainingSchedulePreview(const Team& team, bool congestedWeek, size_t limit = 3) {
    std::ostringstream out;
    const auto schedule = development::buildWeeklyTrainingSchedule(team, congestedWeek);
    for (size_t i = 0; i < schedule.size() && i < limit; ++i) {
        if (i) out << "\r\n";
        out << schedule[i].day << ": " << schedule[i].focus << " | " << schedule[i].note;
    }
    return out.str();
}

std::string boardStatusLabel(int confidence) {
    if (confidence >= 75) return "Muy alta";
    if (confidence >= 55) return "Estable";
    if (confidence >= 35) return "En observacion";
    if (confidence >= 20) return "Bajo presion";
    return "Critica";
}

std::vector<DashboardMetric> buildMetrics(const Career& career, const std::vector<std::string>& alerts) {
    if (!career.myTeam) {
        return {
            {"Club", "Sin carrera", kThemeAccentBlue},
            {"Estado", "Listo para empezar", kThemeAccentGreen},
            {"Presupuesto", "--", kThemeAccent},
            {"Reputacion", "Manager nuevo", kThemeAccentBlue},
            {"Alertas", "Base lista", alerts.empty() ? kThemeAccentGreen : kThemeWarning}
        };
    }

    return {
        {"Club", career.myTeam->name, kThemeAccentBlue},
        {"Fecha", "Temp " + std::to_string(career.currentSeason) + " / Sem " + std::to_string(career.currentWeek), kThemeAccent},
        {"Presupuesto", formatMoneyValue(career.myTeam->budget), kThemeAccentGreen},
        {"Reputacion", std::to_string(career.managerReputation), kThemeAccentBlue},
        {"Alertas", std::to_string(alerts.size()), alerts.empty() ? kThemeAccentGreen : kThemeWarning}
    };
}

LeagueTable selectedLeagueTable(const AppState& state) {
    if (state.currentFilter == "Tabla general") {
        LeagueTable table = state.career.leagueTable;
        table.sortTable();
        return table;
    }
    return buildRelevantCompetitionTable(state.career);
}

std::string findNextMatchLine(const Career& career) {
    if (!career.myTeam) return "Sin partido programado.";
    if (career.currentWeek < 1 || career.currentWeek > static_cast<int>(career.schedule.size())) {
        return "No quedan fechas programadas en la division activa.";
    }
    const auto& fixtures = career.schedule[static_cast<size_t>(career.currentWeek - 1)];
    for (const auto& match : fixtures) {
        if (match.first < 0 || match.second < 0 ||
            match.first >= static_cast<int>(career.activeTeams.size()) ||
            match.second >= static_cast<int>(career.activeTeams.size())) {
            continue;
        }
        Team* home = career.activeTeams[static_cast<size_t>(match.first)];
        Team* away = career.activeTeams[static_cast<size_t>(match.second)];
        if (home != career.myTeam && away != career.myTeam) continue;
        Team* opponent = home == career.myTeam ? away : home;
        return std::string(home == career.myTeam ? "Local vs " : "Visita vs ") + opponent->name +
               " | " + divisionDisplay(career.activeDivision);
    }
    return "Tu club no tiene partido visible en la proxima fecha.";
}

std::vector<std::vector<std::string> > buildFixtureRows(const Career& career, int limit) {
    std::vector<std::vector<std::string> > rows;
    if (!career.myTeam) return rows;
    int added = 0;
    for (int week = std::max(1, career.currentWeek); week <= static_cast<int>(career.schedule.size()) && added < limit; ++week) {
        for (const auto& match : career.schedule[static_cast<size_t>(week - 1)]) {
            if (match.first < 0 || match.second < 0 ||
                match.first >= static_cast<int>(career.activeTeams.size()) ||
                match.second >= static_cast<int>(career.activeTeams.size())) {
                continue;
            }
            Team* home = career.activeTeams[static_cast<size_t>(match.first)];
            Team* away = career.activeTeams[static_cast<size_t>(match.second)];
            if (home != career.myTeam && away != career.myTeam) continue;
            rows.push_back({
                std::to_string(week),
                divisionDisplay(career.activeDivision),
                home == career.myTeam ? away->name : home->name,
                home == career.myTeam ? "Local" : "Visita",
                week == career.currentWeek ? "Proximo" : "Pendiente"
            });
            ++added;
        }
    }
    return rows;
}

std::vector<std::string> buildAlertLines(const Career& career) {
    std::vector<std::string> alerts;
    if (!career.myTeam) {
        alerts.push_back("Crea o carga una carrera para activar alertas.");
        return alerts;
    }

    for (const auto& player : career.myTeam->players) {
        if (player.injured) {
            alerts.push_back("[Lesion] " + player.name + " - " + player.injuryType + " (" + std::to_string(player.injuryWeeks) + " sem)");
        }
        if (player.fitness < 60) {
            alerts.push_back("[Fatiga] " + player.name + " llega con condicion " + std::to_string(player.fitness));
        }
        if (player.happiness < 45) {
            alerts.push_back("[Moral] " + player.name + " baja a " + std::to_string(player.happiness));
        }
        if (player.contractWeeks <= 12) {
            alerts.push_back("[Contrato] " + player.name + " termina en " + std::to_string(player.contractWeeks) + " semanas");
        }
        if (player.wantsToLeave) {
            alerts.push_back("[Salida] " + player.name + " quiere salir del club");
        }
    }

    if (career.boardConfidence < 35) {
        alerts.push_back("[Directiva] La confianza cae a " + std::to_string(career.boardConfidence) + "/100");
    }
    if (!career.boardMonthlyObjective.empty() && career.boardMonthlyProgress < career.boardMonthlyTarget &&
        career.boardMonthlyDeadlineWeek - career.currentWeek <= 2) {
        alerts.push_back("[Objetivo] " + career.boardMonthlyObjective + " queda bajo presion");
    }

    DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(*career.myTeam, career.currentWeek);
    if (dressing.socialTension >= 5) {
        alerts.push_back("[Vestuario] La tension interna sube a " + std::to_string(dressing.socialTension));
    }
    if (dressing.promiseRiskCount > 0) {
        alerts.push_back("[Promesas] " + std::to_string(dressing.promiseRiskCount) + " promesa(s) bajo presion");
    }
    if (alerts.empty()) {
        alerts.push_back("[Estado] No hay alertas criticas esta semana");
    }
    return alerts;
}

std::string teamConditionLabel(int averageFitness) {
    if (averageFitness >= 84) return "Alta";
    if (averageFitness >= 68) return "Media";
    return "Baja";
}

std::string teamMoraleLabel(int morale) {
    if (morale >= 72) return "Alta";
    if (morale >= 52) return "Media";
    return "Baja";
}

std::string severityLabel(int value, int high, int medium) {
    if (value >= high) return "Alta";
    if (value >= medium) return "Media";
    return "Baja";
}

bool newsMatchesFilter(const std::string& line, const std::string& filter) {
    if (filter == "Todo" || filter.empty()) return true;
    std::string lower = toLower(line);
    if (filter == "Alertas") return lower.find("alerta") != std::string::npos || lower.find("riesgo") != std::string::npos;
    if (filter == "Mercado") return lower.find("fich") != std::string::npos || lower.find("contrat") != std::string::npos || lower.find("shortlist") != std::string::npos;
    if (filter == "Resultados") return lower.find("copa") != std::string::npos || lower.find("campeon") != std::string::npos || lower.find("gana") != std::string::npos;
    if (filter == "Lesiones") return lower.find("lesion") != std::string::npos;
    if (filter == "Contratos") return lower.find("contrato") != std::string::npos || lower.find("renov") != std::string::npos;
    return true;
}

std::string lastMatchPanelText(const Career& career, size_t maxReportLines, size_t maxEvents) {
    return match_center_service::formatLastMatchCenter(career, maxReportLines, maxEvents);
}

std::string dressingRoomPanelText(const Career& career, size_t maxAlerts) {
    return dressing_room_service::formatSnapshot(career, maxAlerts);
}

std::vector<std::string> buildFeedLines(const Career& career, const std::string& filter, size_t limit = 18) {
    std::vector<std::string> lines;
    if (!career.lastMatchAnalysis.empty()) {
        lines.push_back("[Ultimo partido] " + career.lastMatchAnalysis);
    }
    for (auto it = career.newsFeed.rbegin(); it != career.newsFeed.rend() && lines.size() < limit; ++it) {
        if (!newsMatchesFilter(*it, filter)) continue;
        lines.push_back(*it);
    }
    if (lines.empty()) lines.push_back("No hay entradas para el filtro actual.");
    return lines;
}

std::string playerStatus(const Player& player) {
    std::vector<std::string> flags;
    if (player.injured) flags.push_back("Les");
    if (player.matchesSuspended > 0) flags.push_back("Susp");
    if (player.wantsToLeave) flags.push_back("Salida");
    if (player.onLoan) flags.push_back("Prestamo");
    if (flags.empty()) return "OK";
    return joinStringValues(flags, " | ");
}

int playerSortMetric(const Player& player, int column) {
    switch (column) {
        case 1: return player.age;
        case 3: return player.skill;
        case 4: return player.potential;
        case 5: return player.happiness;
        case 6: return player.currentForm;
        case 7: return player.fitness;
        case 9: return player.contractWeeks;
        default: return player.skill;
    }
}

bool playerMatchesFilter(const Team& team, const Player& player, const std::string& filter) {
    if (filter == "Todos" || filter == "Todo" || filter.empty()) return true;
    if (filter == "Lesionados") return player.injured;
    if (filter == "XI") {
        std::vector<int> xi = team.getStartingXIIndices();
        for (int idx : xi) {
            if (idx >= 0 && idx < static_cast<int>(team.players.size()) && &team.players[static_cast<size_t>(idx)] == &player) return true;
        }
        return false;
    }
    return normalizePosition(player.position) == filter;
}

bool youthMatchesFilter(const Player& player, const std::string& filter) {
    if (filter == "Todos" || filter.empty()) return player.age <= 23;
    if (filter == "Sub-21") return player.age <= 21;
    if (filter == "Elite") return player.age <= 21 && player.potential >= 78;
    if (filter == "Cedidos") return player.onLoan;
    return player.age <= 23;
}

std::string expectedRoleLabel(const Team& team, const Player& player) {
    int avg = team.getAverageSkill();
    if (player.skill >= avg + 6) return "Titular esperado";
    if (player.potential >= player.skill + 8) return "Proyecto";
    if (player.skill >= avg - 2) return "Rotacion fuerte";
    return "Rotacion";
}

std::string playerInterestLabel(const Career& career, const Team& seller, const Player& player) {
    if (!career.myTeam) return "Indefinido";
    int score = teamPrestigeScore(*career.myTeam) - teamPrestigeScore(seller);
    score += career.managerReputation / 10;
    score += player.wage < career.myTeam->budget / 35 ? 8 : -6;
    if (player.skill > career.myTeam->getAverageSkill() + 4) score += 5;
    if (score >= 18) return "Alto";
    if (score >= 6) return "Medio";
    return "Bajo";
}

std::string sellerInterestLabel(const Team& seller, const Player& player) {
    int pressure = 0;
    if (player.contractWeeks <= 18) pressure += 2;
    if (seller.players.size() > 24) pressure += 2;
    if (player.wantsToLeave) pressure += 3;
    if (pressure >= 5) return "Vendible";
    if (pressure >= 2) return "Escuchan";
    return "Resistiran";
}

std::string transferRadarLabel(const TransferTarget& target) {
    std::string prefix = target.onShortlist ? "Shortlist" : (target.urgentNeed ? "Prioridad" : "Seguimiento");
    return prefix + " " + std::to_string(target.scoutingConfidence) + "%";
}

std::string transferMarketLabel(const TransferTarget& target) {
    if (target.contractRunningOut) return "Contrato corto";
    if (target.availableForLoan && target.affordabilityScore < 12) return "Cesion viable";
    if (target.affordabilityScore >= 20) return "Viable";
    if (target.affordabilityScore >= 8) return "Coste tenso";
    return "Coste alto";
}

const Player* findPlayerByName(const Team& team, const std::string& name) {
    for (const auto& player : team.players) {
        if (player.name == name) return &player;
    }
    return nullptr;
}

std::string buildPlayerProfile(const Team& team, const Player* player) {
    if (!player) return "Selecciona un jugador para abrir su ficha completa.";
    std::ostringstream out;
    out << "Resumen\r\n";
    out << player->name << " | " << normalizePosition(player->position)
        << " | " << player->age << " anos"
        << " | Media " << player->skill
        << " | Potencial " << player->potential << "\r\n";
    out << "Rol " << player->role << " (" << player->roleDuty << ")"
        << " | Promesa " << player->promisedRole
        << " | Posicion prometida " << player->promisedPosition
        << " | Pie " << player->preferredFoot << "\r\n\r\n";
    out << "Atributos\r\n";
    out << "Ataque " << player->attack
        << " | Defensa " << player->defense
        << " | Resistencia " << player->stamina
        << " | Disciplina " << player->tacticalDiscipline
        << " | Versatilidad " << player->versatility << "\r\n";
    out << "Liderazgo " << player->leadership
        << " | Profesionalismo " << player->professionalism
        << " | Ambicion " << player->ambition
        << " | Consistencia " << player->consistency << "\r\n\r\n";
    out << "Contrato\r\n";
    out << "Salario " << formatMoneyValue(player->wage)
        << " | Clausula " << formatMoneyValue(player->releaseClause)
        << " | Semanas restantes " << player->contractWeeks << "\r\n";
    out << "Estado " << playerStatus(*player)
        << " | Desea salir: " << (player->wantsToLeave ? "Si" : "No") << "\r\n\r\n";
    out << "Estadisticas\r\n";
    out << "PJ " << player->matchesPlayed
        << " | Titularidades " << player->startsThisSeason
        << " | Goles " << player->goals
        << " | Asistencias " << player->assists << "\r\n";
    out << "Forma " << player->currentForm
        << " (" << playerFormLabel(*player) << ")"
        << " | Moral " << player->happiness
        << " | Momento " << player->moraleMomentum
        << " | Quimica " << player->chemistry
        << " | Fisico " << player->fitness
        << " | Carga " << player->fatigueLoad << "\r\n\r\n";
    out << "Desarrollo\r\n";
    out << "Plan " << player->developmentPlan
        << " | Grupo " << player->socialGroup
        << " | Posiciones secundarias " << (player->secondaryPositions.empty() ? "-" : joinStringValues(player->secondaryPositions, ", ")) << "\r\n";
    out << "Rasgos " << (player->traits.empty() ? "-" : joinStringValues(player->traits, ", ")) << "\r\n\r\n";
    out << "Historial\r\n";
    out << "Lesiones previas " << player->injuryHistory
        << " | Amarillas " << player->seasonYellowCards
        << " | Rojas " << player->seasonRedCards << "\r\n";
    out << "Aporte relativo: " << expectedRoleLabel(team, *player);
    return out.str();
}

ListPanelModel buildTeamStatusModel(const Career& career) {
    ListPanelModel model;
    model.title = "TeamStatusPanel";
    model.columns = {{L"Indicador", 160}, {L"Estado", 100}, {L"Detalle", 300}};
    if (!career.myTeam) {
        model.rows.push_back({"Estado general", "Sin datos", "No hay carrera activa. Crea una nueva partida para comenzar."});
        return model;
    }

    const Team& team = *career.myTeam;
    const DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(team, career.currentWeek);
    int injured = 0;
    int suspended = 0;
    int lowMorale = 0;
    int lowFitness = 0;
    int fitnessTotal = 0;
    int fatigueLoad = 0;
    for (const auto& player : team.players) {
        fitnessTotal += player.fitness;
        fatigueLoad += player.fatigueLoad;
        if (player.injured) ++injured;
        if (player.matchesSuspended > 0) ++suspended;
        if (player.happiness < 45) ++lowMorale;
        if (player.fitness < 62 || player.fatigueLoad >= 60) ++lowFitness;
    }
    int squadSize = std::max(1, static_cast<int>(team.players.size()));
    int averageFitness = fitnessTotal / squadSize;
    int averageLoad = fatigueLoad / squadSize;

    model.rows.push_back({"Fatiga", teamConditionLabel(averageFitness), "Promedio fisico " + std::to_string(averageFitness) + " / 100"});
    model.rows.push_back({"Moral", teamMoraleLabel(team.morale), "Moral colectiva " + std::to_string(team.morale) + " / 100"});
    model.rows.push_back({"Lesionados", severityLabel(injured, 3, 1), std::to_string(injured) + " jugadores fuera"});
    model.rows.push_back({"Suspendidos", severityLabel(suspended, 2, 1), std::to_string(suspended) + " jugadores sancionados"});
    model.rows.push_back({"Carga alta", severityLabel(lowFitness, 4, 2), std::to_string(lowFitness) + " jugadores con fisico bajo"});
    model.rows.push_back({"Vestuario", severityLabel(lowMorale, 4, 2), std::to_string(lowMorale) + " jugadores con moral baja"});
    model.rows.push_back({"Carga acumulada", severityLabel(averageLoad, 65, 40), "Promedio de carga " + std::to_string(averageLoad) + " / 100"});
    model.rows.push_back({"Promesas", severityLabel(static_cast<int>(career.activePromises.size()), 3, 1), std::to_string(career.activePromises.size()) + " compromisos activos"});
    model.rows.push_back({"Conflictos", severityLabel(dressing.conflictCount, 3, 1), std::to_string(dressing.conflictCount) + " focos de tension interna"});
    return model;
}

ListPanelModel buildLeagueTableModel(const Career& career, const std::string& filter) {
    ListPanelModel model;
    model.title = "LeagueTableView";
    model.columns = {
        {L"Pos", 46}, {L"Equipo", 220}, {L"PJ", 48}, {L"G", 40}, {L"E", 40},
        {L"P", 40}, {L"GF", 46}, {L"GC", 46}, {L"DG", 48}, {L"PTS", 52}, {L"Forma", 96}
    };
    if (!career.myTeam) return model;

    LeagueTable table = filter == "Tabla general" ? career.leagueTable : buildRelevantCompetitionTable(career);
    table.sortTable();
    for (size_t i = 0; i < table.teams.size(); ++i) {
        Team* team = table.teams[i];
        if (!team) continue;
        int played = team->wins + team->draws + team->losses;
        int diff = team->goalsFor - team->goalsAgainst;
        std::string name = team->name;
        if (team == career.myTeam) name += " *";
        std::string form = team->morale >= 70 ? "Alta" : (team->morale >= 50 ? "Mixta" : "Baja");
        model.rows.push_back({
            std::to_string(i + 1), name, std::to_string(played), std::to_string(team->wins), std::to_string(team->draws),
            std::to_string(team->losses), std::to_string(team->goalsFor), std::to_string(team->goalsAgainst),
            std::to_string(diff), std::to_string(team->points), form
        });
    }
    return model;
}

ListPanelModel buildInjuryModel(const Career& career) {
    ListPanelModel model;
    model.title = "InjuryListWidget";
    model.columns = {{L"Jugador", 220}, {L"Tipo", 120}, {L"Sem", 50}, {L"Fisico", 60}, {L"Nota", 160}};
    if (!career.myTeam) return model;
    for (const auto& player : career.myTeam->players) {
        if (!player.injured && player.fitness >= 65) continue;
        model.rows.push_back({
            player.name,
            player.injured ? player.injuryType : "Fatiga",
            player.injured ? std::to_string(player.injuryWeeks) : "-",
            std::to_string(player.fitness),
            player.injured ? "No disponible" : "Reducir cargas"
        });
    }
    if (model.rows.empty()) {
        model.rows.push_back({"No hay lesiones actualmente", "-", "-", "-", "Plantilla completa y disponible"});
    }
    return model;
}

ListPanelModel buildFixtureModel(const Career& career, int limit) {
    ListPanelModel model;
    model.title = "FixtureListView";
    model.columns = {{L"Sem", 48}, {L"Comp", 120}, {L"Rival", 220}, {L"Sede", 70}, {L"Estado", 100}};
    model.rows = buildFixtureRows(career, limit);
    if (model.rows.empty()) {
        model.rows.push_back({"-", "-", "No hay partidos programados", "-", "-"});
    }
    return model;
}

ListPanelModel buildSquadUnitModel(const Team& team) {
    ListPanelModel model;
    model.title = "SquadUnitOverview";
    model.columns = {{L"Linea", 90}, {L"Jug.", 50}, {L"Media", 60}, {L"Pot", 60}, {L"Fisico", 70}, {L"Estado", 120}};

    struct Acc {
        int count = 0;
        int skill = 0;
        int potential = 0;
        int fitness = 0;
        int warnings = 0;
    };
    std::map<std::string, Acc> groups;
    for (const auto& player : team.players) {
        std::string key = normalizePosition(player.position);
        Acc& acc = groups[key];
        acc.count++;
        acc.skill += player.skill;
        acc.potential += player.potential;
        acc.fitness += player.fitness;
        if (player.injured || player.fitness < 60 || player.happiness < 45) acc.warnings++;
    }
    for (const auto& entry : groups) {
        const Acc& acc = entry.second;
        if (acc.count == 0) continue;
        model.rows.push_back({
            entry.first,
            std::to_string(acc.count),
            std::to_string(acc.skill / acc.count),
            std::to_string(acc.potential / acc.count),
            std::to_string(acc.fitness / acc.count),
            acc.warnings > 0 ? std::to_string(acc.warnings) + " alertas" : "Estable"
        });
    }
    return model;
}

ListPanelModel buildPlayerTableModel(AppState& state, bool youthOnly) {
    ListPanelModel model;
    model.title = youthOnly ? "YouthPlayerTableView" : "PlayerTableView";
    model.columns = {
        {L"Nombre", 208}, {L"Edad", 54}, {L"Pos", 54}, {L"Media", 58}, {L"Pot", 58},
        {L"Moral", 60}, {L"Forma", 60}, {L"Fisico", 60}, {L"Salario", 92}, {L"Contrato", 66}, {L"Estado", 120}
    };
    if (!state.career.myTeam) return model;

    Team& team = *state.career.myTeam;
    std::vector<const Player*> players;
    for (const auto& player : team.players) {
        bool visible = youthOnly ? youthMatchesFilter(player, state.currentFilter) : playerMatchesFilter(team, player, state.currentFilter);
        if (visible) players.push_back(&player);
    }
    std::sort(players.begin(), players.end(), [&](const Player* left, const Player* right) {
        if (state.squadSort.column == 0) {
            return state.squadSort.ascending ? left->name < right->name : left->name > right->name;
        }
        if (state.squadSort.column == 2) {
            return state.squadSort.ascending ? normalizePosition(left->position) < normalizePosition(right->position)
                                             : normalizePosition(left->position) > normalizePosition(right->position);
        }
        if (state.squadSort.column == 8) {
            return state.squadSort.ascending ? left->wage < right->wage : left->wage > right->wage;
        }
        int lv = playerSortMetric(*left, state.squadSort.column);
        int rv = playerSortMetric(*right, state.squadSort.column);
        return state.squadSort.ascending ? lv < rv : lv > rv;
    });

    if (state.selectedPlayerName.empty() && !players.empty()) {
        state.selectedPlayerName = players.front()->name;
    }

    for (const Player* player : players) {
        model.rows.push_back({
            player->name,
            std::to_string(player->age),
            normalizePosition(player->position),
            std::to_string(player->skill),
            std::to_string(player->potential),
            std::to_string(player->happiness),
            std::to_string(player->currentForm),
            std::to_string(player->fitness),
            formatMoneyValue(player->wage),
            std::to_string(player->contractWeeks),
            playerStatus(*player)
        });
    }
    if (model.rows.empty()) {
        model.rows.push_back({"Sin jugadores", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-"});
    }
    return model;
}

ListPanelModel buildComparisonModel(const Career& career, const Player* selected) {
    ListPanelModel model;
    model.title = "PlayerComparisonPanel";
    model.columns = {{L"Campo", 120}, {L"Seleccionado", 120}, {L"Referencia", 120}, {L"Mercado", 120}};
    if (!career.myTeam || !selected) {
        model.rows.push_back({"Comparador", "-", "-", "-"});
        return model;
    }

    const Player* reference = nullptr;
    const Player* market = nullptr;
    for (const auto& player : career.myTeam->players) {
        if (&player == selected) continue;
        if (normalizePosition(player.position) != normalizePosition(selected->position)) continue;
        if (!reference || player.skill > reference->skill) reference = &player;
    }
    for (const auto& team : career.allTeams) {
        if (&team == career.myTeam) continue;
        for (const auto& player : team.players) {
            if (normalizePosition(player.position) != normalizePosition(selected->position)) continue;
            if (!market || player.skill > market->skill) market = &player;
        }
    }

    model.rows.push_back({"Edad", std::to_string(selected->age), reference ? std::to_string(reference->age) : "-", market ? std::to_string(market->age) : "-"});
    model.rows.push_back({"Media", std::to_string(selected->skill), reference ? std::to_string(reference->skill) : "-", market ? std::to_string(market->skill) : "-"});
    model.rows.push_back({"Potencial", std::to_string(selected->potential), reference ? std::to_string(reference->potential) : "-", market ? std::to_string(market->potential) : "-"});
    model.rows.push_back({"Forma", std::to_string(selected->currentForm), reference ? std::to_string(reference->currentForm) : "-", market ? std::to_string(market->currentForm) : "-"});
    model.rows.push_back({"Valor", formatMoneyValue(selected->value), reference ? formatMoneyValue(reference->value) : "-", market ? formatMoneyValue(market->value) : "-"});
    model.rows.push_back({"Salario", formatMoneyValue(selected->wage), reference ? formatMoneyValue(reference->wage) : "-", market ? formatMoneyValue(market->wage) : "-"});
    return model;
}

ListPanelModel buildTransferPipelineModel(const Career& career) {
    ListPanelModel model;
    model.title = "TransferPipeline";
    model.columns = {{L"Tipo", 100}, {L"Jugador", 180}, {L"Club", 180}, {L"Detalle", 260}, {L"Monto", 120}};
    if (!career.myTeam) return model;

    for (const auto& move : career.pendingTransfers) {
        std::string type = move.loan ? "Prestamo" : (move.preContract ? "Precontrato" : "Pendiente");
        std::ostringstream detail;
        detail << "T" << move.effectiveSeason;
        if (move.contractWeeks > 0) detail << " | " << move.contractWeeks << " sem";
        if (!move.promisedRole.empty()) detail << " | " << move.promisedRole;
        model.rows.push_back({type, move.playerName, move.fromTeam + " -> " + move.toTeam, detail.str(),
                              formatMoneyValue(move.fee > 0 ? move.fee : move.wage)});
    }
    if (model.rows.empty()) {
        model.rows.push_back({"Info", "-", career.myTeam->name, "No hay movimientos pendientes.", "-"});
    }
    return model;
}

std::vector<TransferPreviewItem> buildTransferTargets(const Career& career, const std::string& filter) {
    std::vector<TransferPreviewItem> rows;
    if (!career.myTeam) return rows;
    const Team& buyer = *career.myTeam;
    const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, buyer);

    for (const auto& seller : career.allTeams) {
        if (&seller == career.myTeam) continue;
        for (const auto& player : seller.players) {
            std::string position = normalizePosition(player.position);
            if (filter == "Sub-23" && player.age > 23) continue;
            if (filter == "Potencial 75+" && player.potential < 75) continue;
            if (filter == "ARQ" || filter == "DEF" || filter == "MED" || filter == "DEL") {
                if (position != filter) continue;
            }
            if (player.age > 35 || player.onLoan) continue;

            const TransferTarget target = ai_transfer_manager::evaluateTarget(career, buyer, seller, player, strategy);
            const long long upfrontPackage = target.expectedFee + target.expectedAgentFee;
            if (filter == "Precio accesible" && upfrontPackage > strategy.maxTransferBudget) continue;
            if (filter == "Salario bajo" && target.expectedWage > strategy.maxWageBudget) continue;

            rows.push_back({
                player.name,
                position,
                player.age,
                player.skill,
                player.potential,
                target.expectedFee,
                target.expectedWage,
                target.expectedAgentFee,
                seller.name,
                transferRadarLabel(target),
                transferMarketLabel(target),
                expectedRoleLabel(buyer, player),
                target.scoutingNote,
                target.onShortlist,
                target.urgentNeed,
                target.scoutingConfidence,
                target.affordabilityScore,
                target.totalScore
            });
        }
    }

    std::sort(rows.begin(), rows.end(), [](const TransferPreviewItem& left, const TransferPreviewItem& right) {
        if (left.onShortlist != right.onShortlist) return left.onShortlist > right.onShortlist;
        if (left.urgentNeed != right.urgentNeed) return left.urgentNeed > right.urgentNeed;
        if (left.totalScore != right.totalScore) return left.totalScore > right.totalScore;
        return left.expectedFee + left.expectedAgentFee < right.expectedFee + right.expectedAgentFee;
    });
    if (rows.size() > 20) rows.resize(20);
    return rows;
}

GuiPageModel buildDashboardModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = state.career.myTeam
        ? "Panorama inmediato del club: proximo rival, forma del plantel, clasificacion y focos de gestion."
        : "No hay carrera activa. Crea una nueva partida para abrir el centro del club.";
    model.summary.title = "UpcomingMatchWidget";
    model.primary = buildLeagueTableModel(state.career, "Grupo actual");
    model.secondary = buildTeamStatusModel(state.career);
    model.footer = buildInjuryModel(state.career);
    model.detail.title = "LastResultPanel";
    model.feed.title = "NewsFeedPanel";
    model.feed.lines = state.currentFilter == "Alertas" ? alerts : buildFeedLines(state.career, state.currentFilter == "Todo" ? "" : state.currentFilter);

    if (!state.career.myTeam) {
        model.summary.content =
            "No hay carrera activa.\r\n\r\n"
            "Crea una nueva partida o carga un guardado para abrir el centro del club.\r\n\r\n"
            "Cuando empieces tendras acceso inmediato a:\r\n"
            "- proximo partido\r\n"
            "- estado del equipo\r\n"
            "- tabla y noticias\r\n"
            "- mercado, finanzas y cantera";
        model.detail.content =
            "Empieza aqui\r\n\r\n"
            "1. Elige una division.\r\n"
            "2. Selecciona un club.\r\n"
            "3. Escribe el nombre del manager.\r\n"
            "4. Usa una de las acciones inferiores.";
        model.feed.lines.clear();
        model.feed.lines.push_back("Sin noticias por ahora. Inicia una carrera para activar el mundo del club.");
        model.feed.lines.push_back("Sugerencia: un club pequeno deja ver mejor cantera, sueldos y rotacion.");
        model.feed.lines.push_back("Sugerencia: usa Validar si cambiaste datos externos o reglas de competicion.");
        return model;
    }

    Team& team = *state.career.myTeam;
    const DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(team, state.career.currentWeek);
    const bool congestedWeek = isCongestedWeek(state.career);
    int injured = 0;
    int lowMorale = 0;
    for (const auto& player : team.players) {
        if (player.injured) injured++;
        if (player.happiness < 45) lowMorale++;
    }

    std::ostringstream out;
    out << findNextMatchLine(state.career) << "\r\n\r\n";
    out << "Posicion " << state.career.currentCompetitiveRank() << "/" << state.career.currentCompetitiveFieldSize()
        << " | Puntos " << team.points << " | DG " << (team.goalsFor - team.goalsAgainst) << "\r\n";
    out << "Moral " << team.morale << " | Lesionados " << injured << " | Tension " << dressing.socialTension << "\r\n";
    out << "Plan semanal: " << team.trainingFocus << (congestedWeek ? " | semana congestionada" : " | semana regular") << "\r\n";
    out << "Objetivo: " << (state.career.boardMonthlyObjective.empty() ? "Sin objetivo mensual activo" : state.career.boardMonthlyObjective) << "\r\n\r\n";
    out << "Microciclo\r\n" << trainingSchedulePreview(team, congestedWeek) << "\r\n\r\n";
    out << "Lectura del rival\r\n" << buildOpponentReport(state.career);
    model.summary.content = out.str();

    const auto analyticsLines = analytics_service::buildTeamAnalyticsLines(state.career, team);
    std::ostringstream detail;
    detail << "Ultimo resultado\r\n";
    detail << lastMatchPanelText(state.career, 6, 6) << "\r\n\r\n";
    detail << "Impacto inmediato\r\n";
    detail << "Confianza " << state.career.boardConfidence << "/100"
           << " | Progreso " << state.career.boardMonthlyProgress << "/" << state.career.boardMonthlyTarget << "\r\n";
    detail << "Apoyo de lideres " << dressing.leadershipSupport
           << " | Aislados " << dressing.isolatedPlayers
           << " | Promesas en riesgo " << dressing.promiseRiskCount << "\r\n";
    detail << "DT del club " << team.headCoachName
           << " | Seguridad " << team.jobSecurity
           << " | Politica " << team.transferPolicy << "\r\n";
    if (!analyticsLines.empty()) {
        detail << analyticsLines.front() << "\r\n";
        if (analyticsLines.size() > 1) detail << analyticsLines[1] << "\r\n";
        if (analyticsLines.size() > 2) detail << analyticsLines[2] << "\r\n";
    }
    detail << "\r\n" << dressingRoomPanelText(state.career, 4);
    model.detail.content = detail.str();
    return model;
}

GuiPageModel buildSquadModel(AppState& state, bool youthOnly) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = youthOnly ? "Cantera con filtro por edad y potencial." : "Plantilla completa con orden por columnas y ficha detallada.";
    model.summary.title = youthOnly ? "YouthSummaryPanel" : "SquadSummaryPanel";
    model.primary = state.career.myTeam ? buildSquadUnitModel(*state.career.myTeam) : ListPanelModel{};
    model.secondary = buildPlayerTableModel(state, youthOnly);
    model.feed.title = "AlertPanel";
    model.feed.lines = alerts;

    const Player* selected = nullptr;
    if (state.career.myTeam) {
        selected = findPlayerByName(*state.career.myTeam, state.selectedPlayerName);
    }
    model.footer = buildComparisonModel(state.career, selected);
    model.detail.title = "PlayerProfilePanel";
        model.detail.content = state.career.myTeam ? buildPlayerProfile(*state.career.myTeam, selected)
                                               : "Selecciona una carrera para mostrar jugadores.";

    if (!state.career.myTeam) {
        model.summary.content = "No hay plantilla disponible.";
        return model;
    }

    Team& team = *state.career.myTeam;
    int avgAge = 0;
    int avgFitness = 0;
    int avgPotential = 0;
    int count = 0;
    for (const auto& player : team.players) {
        if (youthOnly && player.age > 23) continue;
        avgAge += player.age;
        avgFitness += player.fitness;
        avgPotential += player.potential;
        count++;
    }
    count = std::max(1, count);
    const SquadNeedReport planner = ai_squad_planner::analyzeSquad(team);
    std::ostringstream out;
    out << "Jugadores visibles " << count
        << " | Edad media " << (avgAge / count)
        << " | Potencial medio " << (avgPotential / count)
        << " | Fisico medio " << (avgFitness / count) << "\r\n";
    out << "Filtro actual: " << state.currentFilter << "\r\n";
    out << "Orden: columna " << state.squadSort.column + 1
        << (state.squadSort.ascending ? " asc" : " desc") << "\r\n";
    out << "Planner: necesidad " << planner.weakestPosition
        << " | exceso " << planner.surplusPosition
        << " | riesgo de rotacion " << planner.rotationRisk << "\r\n";
    out << "Cobertura juvenil: " << (planner.youthCoverPositions.empty() ? std::string("-") : joinStringValues(planner.youthCoverPositions, ", ")) << "\r\n";
    out << "Ventas probables: " << (planner.saleCandidates.empty() ? std::string("-") : joinStringValues(planner.saleCandidates, ", ")) << "\r\n";
    model.summary.content = out.str();
    return model;
}

GuiPageModel buildTacticsModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Lee el plan tactico, el once inicial y el impacto esperado de cada ajuste.";
    model.summary.title = "TacticalSummary";
    model.primary.title = "TacticsBoard";
    model.primary.columns = {{L"Variable", 120}, {L"Valor", 90}, {L"Efecto estimado", 260}};
    model.secondary.title = "FormationSelector";
    model.secondary.columns = {{L"Linea", 90}, {L"Jugador", 200}, {L"Pos", 60}, {L"Hab", 50}, {L"Fisico", 60}, {L"Estado", 120}};
    model.footer.title = "TacticalImpactSummary";
    model.footer.columns = {{L"Area", 130}, {L"Lectura", 220}, {L"Riesgo", 220}};
    model.detail.title = "MatchAnalysisPanel";
    model.feed.title = "AlertPanel";
    model.feed.lines = alerts;

    if (!state.career.myTeam) {
        model.summary.content = "No hay equipo para editar tactica.";
        model.detail.content = "Inicia una carrera.";
        return model;
    }

    Team& team = *state.career.myTeam;
    const bool congestedWeek = isCongestedWeek(state.career);
    model.summary.content =
        "Formacion " + team.formation + " | Mentalidad " + team.tactics +
        "\r\nPresion " + std::to_string(team.pressingIntensity) +
        " | Ritmo " + std::to_string(team.tempo) +
        " | Anchura " + std::to_string(team.width) +
        " | Linea " + std::to_string(team.defensiveLine) +
        "\r\nInstruccion actual: " + team.matchInstruction +
        " | Plan semanal: " + team.trainingFocus;

    model.primary.rows.push_back({"Presion", std::to_string(team.pressingIntensity),
                                  team.pressingIntensity >= 4 ? "Recupera arriba, sube fatiga" : "Presion mas contenida"});
    model.primary.rows.push_back({"Ritmo", std::to_string(team.tempo),
                                  team.tempo >= 4 ? "Mas llegadas, mas perdida" : "Mas control del juego"});
    model.primary.rows.push_back({"Anchura", std::to_string(team.width),
                                  team.width >= 4 ? "Abre carriles y centros" : "Compacta por dentro"});
    model.primary.rows.push_back({"Linea", std::to_string(team.defensiveLine),
                                  team.defensiveLine >= 4 ? "Recupera alto, riesgo al balon largo" : "Protege espalda y concede campo"});
    model.primary.rows.push_back({"Instruccion", team.matchInstruction,
                                  team.matchInstruction == "Juego directo" ? "Acelera llegada a ultimo tercio" : "Ajuste situacional"});
    model.primary.rows.push_back({"Plan semanal", team.trainingFocus,
                                  congestedWeek ? "Microciclo corto y conservador" : "Carga completa de preparacion"});

    std::map<std::string, std::vector<const Player*> > byLine;
    for (int index : team.getStartingXIIndices()) {
        if (index < 0 || index >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[static_cast<size_t>(index)];
        byLine[normalizePosition(player.position)].push_back(&player);
    }
    for (const auto& entry : byLine) {
        for (const Player* player : entry.second) {
            model.secondary.rows.push_back({
                entry.first,
                player->name,
                normalizePosition(player->position),
                std::to_string(player->skill),
                std::to_string(player->fitness),
                playerStatus(*player)
            });
        }
    }

    model.footer.rows.push_back({"Posesion", team.tempo >= 4 ? "Vertical" : "Controlada", team.pressingIntensity >= 4 ? "Transiciones largas" : "Menor recuperacion alta"});
    model.footer.rows.push_back({"Ocasiones", team.width >= 4 ? "Ataque por fuera" : "Juego interior", team.tempo >= 4 ? "Mas error tecnico" : "Menos volumen"});
    model.footer.rows.push_back({"Defensa", team.defensiveLine >= 4 ? "Bloque adelantado" : "Bloque medio/bajo", team.defensiveLine >= 4 ? "Espalda expuesta" : "Mayor asedio rival"});
    model.footer.rows.push_back({"Fatiga", congestedWeek ? "Gestion conservadora" : "Carga normal", team.pressingIntensity >= 4 ? "El XI llega mas exigido" : "Riesgo medio"});

    std::ostringstream detail;
    detail << "Presion alta aumenta recuperaciones, pero castiga a jugadores con fisico bajo.\r\n";
    detail << "Ritmo alto acelera la llegada, aunque empeora la precision en equipos con forma baja.\r\n";
    detail << "Bloque bajo reduce espacio interior y aumenta probabilidad de centros rivales.\r\n\r\n";
    detail << "Microciclo semanal\r\n" << trainingSchedulePreview(team, congestedWeek, 6) << "\r\n\r\n";
    detail << "Informe rival: " << buildOpponentReport(state.career) << "\r\n\r\n";
    detail << dressingRoomPanelText(state.career, 4) << "\r\n";
    detail << lastMatchPanelText(state.career, 4, 5);
    model.detail.content = detail.str();
    return model;
}

GuiPageModel buildCalendarModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Calendario competitivo con proximos partidos, copa y contexto de temporada.";
    model.summary.title = "CalendarSummary";
    model.primary = buildFixtureModel(state.career, state.currentFilter == "Toda la fase" ? 24 : (state.currentFilter == "Proximos 10" ? 10 : 5));
    model.secondary.title = "CupAndSeasonNotes";
    model.secondary.columns = {{L"Item", 180}, {L"Estado", 280}, {L"Contexto", 180}};
    model.footer.title = "SeasonHistory";
    model.footer.columns = {{L"Temp", 60}, {L"Club", 170}, {L"Puesto", 60}, {L"Campeon", 180}, {L"Nota", 280}};
    model.detail.title = "SeasonFlowController";
    model.feed.title = "NewsFeedPanel";
    model.feed.lines = buildFeedLines(state.career, "");

    if (!state.career.myTeam) {
        model.summary.content = "No hay calendario cargado.";
        model.detail.content = "Inicia una carrera.";
        return model;
    }

    model.summary.content = findNextMatchLine(state.career) +
                            "\r\nPartidos visibles: " + std::to_string(model.primary.rows.size());
    model.secondary.rows.push_back({"Division activa", divisionDisplay(state.career.activeDivision), "Sem " + std::to_string(state.career.currentWeek)});
    model.secondary.rows.push_back({"Copa", state.career.cupActive ? "Activa" : "Cerrada", state.career.cupChampion.empty() ? "Sin campeon" : state.career.cupChampion});
    model.secondary.rows.push_back({"Transicion", state.career.currentWeek > static_cast<int>(state.career.schedule.size()) ? "Cierre de temporada" : "Fase regular", "SeasonService"});

    for (auto it = state.career.history.rbegin(); it != state.career.history.rend() && model.footer.rows.size() < 6; ++it) {
        model.footer.rows.push_back({
            std::to_string(it->season), it->club, std::to_string(it->finish), it->champion, it->note
        });
    }
    if (model.footer.rows.empty()) model.footer.rows.push_back({"-", "-", "-", "-", "Sin historial"});
    model.detail.content = "El flujo semanal sincroniza calendario, finanzas, desarrollo y copa.\r\n" +
                           std::string("Filtro actual: ") + state.currentFilter;
    return model;
}

GuiPageModel buildLeagueModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Tabla de liga, zonas de objetivo y contexto competitivo del club.";
    model.summary.title = "CompetitionSummary";
    model.primary = buildLeagueTableModel(state.career, state.currentFilter);
    model.secondary.title = "RaceContext";
    model.secondary.columns = {{L"Zona", 110}, {L"Club", 180}, {L"Pts", 60}, {L"Dato", 200}};
    model.footer.title = "SeasonRecords";
    model.footer.columns = {{L"Temp", 60}, {L"Club", 170}, {L"Puesto", 60}, {L"Ascenso/Descenso", 240}, {L"Nota", 200}};
    model.detail.title = "CompetitionReport";
    model.feed.title = "NewsFeedPanel";
    model.feed.lines = buildFeedLines(state.career, state.currentFilter == "Grupo actual" ? "" : "Resultados");

    model.summary.content = buildCompetitionSummaryService(state.career);
    model.detail.content = buildCompetitionSummaryService(state.career);

    if (state.career.myTeam && !model.primary.rows.empty()) {
        model.secondary.rows.push_back({"Tu club", state.career.myTeam->name, std::to_string(state.career.myTeam->points),
                                        "Puesto " + std::to_string(state.career.currentCompetitiveRank())});
        LeagueTable table = selectedLeagueTable(state);
        Team* leader = table.teams.empty() ? nullptr : table.teams.front();
        Team* bottom = table.teams.empty() ? nullptr : table.teams.back();
        if (leader) {
            model.secondary.rows.push_back({"Lider", leader->name, std::to_string(leader->points),
                                            "Media " + std::to_string(leader->getAverageSkill())});
        }
        if (bottom) {
            model.secondary.rows.push_back({"Descenso", bottom->name, std::to_string(bottom->points),
                                            "Moral " + std::to_string(bottom->morale)});
        }
    }
    for (auto it = state.career.history.rbegin(); it != state.career.history.rend() && model.footer.rows.size() < 6; ++it) {
        model.footer.rows.push_back({
            std::to_string(it->season), it->club, std::to_string(it->finish),
            it->promoted + " / " + it->relegated, it->note
        });
    }
    return model;
}

GuiPageModel buildTransfersModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Mercado de fichajes con filtros, lectura del plantel y objetivos prioritarios.";
    model.summary.title = "TransferSearchPanel";
    model.primary.title = "TransferSearchPanel";
    model.primary.columns = {{L"Area", 120}, {L"Valor", 120}, {L"Lectura", 280}};
    model.secondary.title = "TransferMarketView";
    model.secondary.columns = {
        {L"Jugador", 190}, {L"Pos", 54}, {L"Edad", 54}, {L"Media", 58}, {L"Pot", 58},
        {L"Costo", 92}, {L"Salario", 92}, {L"Radar", 92}, {L"Mercado", 100}, {L"Rol", 120}, {L"Club", 180}
    };
    model.footer = buildTransferPipelineModel(state.career);
    model.detail.title = "TransferTargetCard";
    model.feed.title = "NewsFeedPanel";
    model.feed.lines = buildFeedLines(state.career, state.currentFilter == "Todos" ? "Mercado" : "");

    if (!state.career.myTeam) {
        model.summary.content = "Sin carrera activa.";
        model.detail.content = "Inicia una carrera para abrir el mercado.";
        return model;
    }

    Team& team = *state.career.myTeam;
    const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(state.career, team);
    model.summary.content = "Presupuesto actual " + formatMoneyValue(team.budget) + "\r\n"
                            "Filtro " + state.currentFilter + "\r\n"
                            "Pendientes " + std::to_string(state.career.pendingTransfers.size()) + "\r\n"
                            "Shortlist " + std::to_string(state.career.scoutingShortlist.size()) + "\r\n"
                            "Red scouting " + (team.scoutingRegions.empty() ? std::string("-") : joinStringValues(team.scoutingRegions, ", "));
    model.primary.rows.push_back({"Necesidad", detectScoutingNeed(team), "Lectura del plantel actual"});
    model.primary.rows.push_back({"Presupuesto", formatMoneyValue(team.budget), "Define agresividad de mercado"});
    model.primary.rows.push_back({"Contratos cortos", std::to_string(std::count_if(team.players.begin(), team.players.end(), [](const Player& p) { return p.contractWeeks <= 12; })),
                                  "Renovar antes de fichar profundidad"});
    model.primary.rows.push_back({"Scouting", std::to_string(team.scoutingChief), "Mejora precision de objetivos"});
    model.primary.rows.push_back({"Politica", team.transferPolicy, "Define si el club compra valor, cantera o urgencia"});
    model.primary.rows.push_back({"Venta IA", std::to_string(strategy.salePressure), "Presion para liberar suplentes marginales"});

    std::vector<TransferPreviewItem> targets = buildTransferTargets(state.career, state.currentFilter);
    if (state.selectedTransferPlayer.empty() && !targets.empty()) {
        state.selectedTransferPlayer = targets.front().player;
        state.selectedTransferClub = targets.front().club;
    }
    for (const auto& target : targets) {
        model.secondary.rows.push_back({
            target.player,
            target.position,
            std::to_string(target.age),
            std::to_string(target.skill),
            std::to_string(target.potential),
            formatMoneyValue(target.expectedFee),
            formatMoneyValue(target.expectedWage),
            target.scoutingLabel,
            target.marketLabel,
            target.expectedRole,
            target.club
        });
    }
    if (model.secondary.rows.empty()) {
        model.secondary.rows.push_back({"Sin objetivos", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-"});
    }

    const Team* seller = state.career.findTeamByName(state.selectedTransferClub);
    const Player* player = seller ? findPlayerByName(*seller, state.selectedTransferPlayer) : nullptr;
    auto selectedPreview = std::find_if(targets.begin(), targets.end(), [&](const TransferPreviewItem& item) {
        return item.player == state.selectedTransferPlayer && item.club == state.selectedTransferClub;
    });
    if (!player) {
        model.detail.content = "Selecciona un objetivo para ver detalle.";
    } else {
        std::ostringstream detail;
        detail << player->name << " | " << seller->name << " | " << normalizePosition(player->position) << "\r\n";
        detail << "Media " << player->skill << " | Potencial " << player->potential << " | Edad " << player->age << "\r\n";
        if (selectedPreview != targets.end()) {
            detail << "Costo estimado " << formatMoneyValue(selectedPreview->expectedFee)
                   << " | Agente " << formatMoneyValue(selectedPreview->expectedAgentFee)
                   << " | Salario esperado " << formatMoneyValue(selectedPreview->expectedWage) << "\r\n";
            detail << "Radar " << selectedPreview->scoutingLabel
                   << " | Mercado " << selectedPreview->marketLabel << "\r\n";
            detail << "Lectura scouting: " << selectedPreview->scoutingNote << "\r\n";
        } else {
            detail << "Valor " << formatMoneyValue(player->value) << " | Salario " << formatMoneyValue(player->wage) << "\r\n";
        }
        detail << "Interes jugador " << playerInterestLabel(state.career, *seller, *player)
               << " | Interes vendedor " << sellerInterestLabel(*seller, *player) << "\r\n";
        detail << "Rol esperado " << expectedRoleLabel(*state.career.myTeam, *player) << "\r\n";
        detail << "Disponibilidad " << (player->contractWeeks <= 12 ? "Contrato corto" : (player->wantsToLeave ? "Abierto a salir" : "Negociacion dura"))
               << " | Agente " << (agentDifficulty(*player) >= 72 ? "duro" : (agentDifficulty(*player) >= 58 ? "exigente" : "manejable")) << "\r\n";
        detail << "Rasgos: " << (player->traits.empty() ? "-" : joinStringValues(player->traits, ", "));
        model.detail.content = detail.str();
    }
    return model;
}

GuiPageModel buildFinancesModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Finanzas, salarios e infraestructura del club.";
    model.summary.title = "FinanceSummary";
    model.primary.title = "FinanceBreakdown";
    model.primary.columns = {{L"Cuenta", 180}, {L"Valor", 140}, {L"Lectura", 280}};
    model.secondary.title = "SalaryTable";
    model.secondary.columns = {{L"Jugador", 200}, {L"Salario", 110}, {L"Contrato", 70}, {L"Rol", 110}, {L"Riesgo", 160}};
    model.footer.title = "Infrastructure";
    model.footer.columns = {{L"Area", 160}, {L"Nivel", 70}, {L"Staff", 80}, {L"Impacto", 260}};
    model.detail.title = "ClubFinancePanel";
    model.feed.title = "AlertPanel";
    model.feed.lines = alerts;

    if (!state.career.myTeam) {
        model.summary.content = "Sin club activo.";
        return model;
    }

    Team& team = *state.career.myTeam;
    WeeklyFinanceReport finance = finance_system::projectWeeklyReport(team);
    model.summary.content = buildClubSummaryService(state.career);
    model.primary.rows.push_back({"Presupuesto", formatMoneyValue(team.budget), team.budget >= 0 ? "Caja operativa positiva" : "Caja comprometida"});
    model.primary.rows.push_back({"Deuda", formatMoneyValue(team.debt), team.debt > 0 ? "Vigilar amortizacion" : "Sin deuda relevante"});
    model.primary.rows.push_back({"Sponsor semanal", formatMoneyValue(finance.sponsorIncome), "Ingreso fijo"});
    model.primary.rows.push_back({"Taquilla estimada", formatMoneyValue(finance.matchdayIncome), "Proyeccion de partido local"});
    model.primary.rows.push_back({"Merchandising", formatMoneyValue(finance.merchandisingIncome), "Pulso comercial del club"});
    model.primary.rows.push_back({"Bonos variables", formatMoneyValue(finance.bonusIncome), "Premios por rendimiento"});
    model.primary.rows.push_back({"Masa salarial", formatMoneyValue(finance.wageBill), "Costo semanal proyectado"});
    model.primary.rows.push_back({"Buffer de mercado", formatMoneyValue(finance.transferBuffer), finance.riskLevel});

    std::vector<const Player*> players;
    for (const auto& player : team.players) players.push_back(&player);
    std::sort(players.begin(), players.end(), [](const Player* left, const Player* right) { return left->wage > right->wage; });
    for (size_t i = 0; i < players.size() && i < 14; ++i) {
        const Player& player = *players[i];
        std::string risk = player.contractWeeks <= 12 ? "Vence pronto" : (player.wantsToLeave ? "Quiere salir" : "Controlado");
        model.secondary.rows.push_back({
            player.name, formatMoneyValue(player.wage), std::to_string(player.contractWeeks), player.promisedRole, risk
        });
    }
    model.footer.rows.push_back({"Cantera", std::to_string(team.youthFacilityLevel), team.youthCoachName, "Coach " + std::to_string(team.youthCoach) + " | impacta intake y potencial"});
    model.footer.rows.push_back({"Entrenamiento", std::to_string(team.trainingFacilityLevel), team.fitnessCoachName, "Coach " + std::to_string(team.fitnessCoach) + " | progreso y recuperacion"});
    model.footer.rows.push_back({"Arqueros", std::to_string(team.goalkeepingCoach), team.goalkeepingCoachName, "Sostiene rendimiento del portero"});
    model.footer.rows.push_back({"Scouting", std::to_string(team.scoutingChief), team.scoutingChiefName, "Mercado, cobertura e informes"});
    model.footer.rows.push_back({"Analisis", std::to_string(team.performanceAnalyst), team.performanceAnalystName, "Microciclo y preparacion rival"});
    model.footer.rows.push_back({"Estadio", std::to_string(team.stadiumLevel), std::to_string(team.fanBase), "Impacta recaudacion"});

    model.detail.content = "Presupuesto " + formatMoneyValue(team.budget) +
                           "\r\nDeuda " + formatMoneyValue(team.debt) +
                           "\r\nSponsor " + formatMoneyValue(finance.sponsorIncome) +
                           "\r\nTaquilla " + formatMoneyValue(finance.matchdayIncome) +
                           "\r\nMerchandising " + formatMoneyValue(finance.merchandisingIncome) +
                           "\r\nBonos variables " + formatMoneyValue(finance.bonusIncome) +
                           "\r\nBuffer de mercado " + formatMoneyValue(finance.transferBuffer) +
                           "\r\nRiesgo " + finance.riskLevel;
    return model;
}

GuiPageModel buildBoardModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Objetivos, confianza y memoria institucional.";
    model.summary.title = "BoardObjectives";
    model.primary.title = "BoardObjectiveTable";
    model.primary.columns = {{L"Objetivo", 220}, {L"Estado", 120}, {L"Contexto", 220}};
    model.secondary.title = "ClubProfile";
    model.secondary.columns = {{L"Perfil", 160}, {L"Valor", 120}, {L"Lectura", 220}};
    model.footer.title = "CoachHistory";
    model.footer.columns = {{L"Temp", 60}, {L"Club", 170}, {L"Puesto", 60}, {L"Campeon", 180}, {L"Nota", 240}};
    model.detail.title = "BoardReport";
    model.feed.title = "AlertPanel / NewsFeedPanel";
    model.feed.lines = alerts;

    model.summary.content = buildBoardSummaryService(state.career);
    model.detail.content = buildBoardSummaryService(state.career);

    if (state.career.myTeam) {
        model.primary.rows.push_back({"Puesto esperado", std::to_string(state.career.boardExpectedFinish), "Meta de temporada"});
        model.primary.rows.push_back({"Objetivo mensual", state.career.boardMonthlyObjective.empty() ? "Sin objetivo" : state.career.boardMonthlyObjective,
                                      std::to_string(state.career.boardMonthlyProgress) + "/" + std::to_string(state.career.boardMonthlyTarget)});
        model.primary.rows.push_back({"Confianza", std::to_string(state.career.boardConfidence) + "/100", boardStatusLabel(state.career.boardConfidence)});
        model.primary.rows.push_back({"Advertencias", std::to_string(state.career.boardWarningWeeks), "Semanas bajo revision"});

        Team& team = *state.career.myTeam;
        model.secondary.rows.push_back({"Expectativa", teamExpectationLabel(team), "Perfil de club"});
        model.secondary.rows.push_back({"Prestigio", std::to_string(team.clubPrestige), "Influye en fichajes"});
        model.secondary.rows.push_back({"DT del club", team.headCoachName, team.headCoachStyle});
        model.secondary.rows.push_back({"Antiguedad DT", std::to_string(team.headCoachTenureWeeks) + " sem", "Tiempo del proyecto actual"});
        model.secondary.rows.push_back({"Seguridad", std::to_string(team.jobSecurity), "Estabilidad del banquillo"});
        model.secondary.rows.push_back({"Politica", team.transferPolicy, "Mercado del club"});
        model.secondary.rows.push_back({"Red scouting", team.scoutingRegions.empty() ? std::string("-") : joinStringValues(team.scoutingRegions, ", "), "Cobertura activa"});
        model.secondary.rows.push_back({"Identidad cantera", team.youthIdentity, "Condiciona objetivos"});
        model.secondary.rows.push_back({"Estilo", team.clubStyle, "Contexto institucional"});
    }
    for (auto it = state.career.history.rbegin(); it != state.career.history.rend() && model.footer.rows.size() < 8; ++it) {
        model.footer.rows.push_back({
            std::to_string(it->season), it->club, std::to_string(it->finish), it->champion, it->note
        });
    }
    return model;
}

GuiPageModel buildNewsModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state.career, alerts);
    model.infoLine = "Centro del manager: inbox, scouting, rumores y alertas para seguir el pulso del mundo.";
    model.summary.title = "ScoutingInbox";
    model.primary.title = "NewsCardList";
    model.primary.columns = {{L"Tipo", 120}, {L"Titular", 620}};
    model.secondary.title = "ScoutingInbox";
    model.secondary.columns = {{L"Tipo", 140}, {L"Detalle", 540}};
    model.footer.title = "AlertPanel";
    model.footer.columns = {{L"Nivel", 120}, {L"Detalle", 560}};
    model.detail.title = "NewsDetail";
    model.feed.title = "NewsFeedPanel";
    model.feed.lines = buildFeedLines(state.career, state.currentFilter == "Todo" ? "" : state.currentFilter, 24);

    for (const auto& line : model.feed.lines) {
        std::string type = "Info";
        std::string lower = toLower(line);
        if (lower.find("lesion") != std::string::npos) type = "Lesiones";
        else if (lower.find("fich") != std::string::npos || lower.find("contrat") != std::string::npos) type = "Mercado";
        else if (lower.find("copa") != std::string::npos || lower.find("campeon") != std::string::npos) type = "Resultados";
        else if (lower.find("directiva") != std::string::npos || lower.find("objetivo") != std::string::npos) type = "Directiva";
        model.primary.rows.push_back({type, line});
    }

    for (const auto& entry : inbox_service::buildCombinedInbox(state.career, 18)) {
        model.secondary.rows.push_back({entry.channel, entry.text});
    }
    if (model.secondary.rows.empty()) model.secondary.rows.push_back({"Inbox", "Sin novedades recientes"});

    for (const auto& alert : alerts) {
        model.footer.rows.push_back({"Alerta", alert});
    }
    model.summary.content = "Entradas visibles: " + std::to_string(model.feed.lines.size()) +
                            "\r\nInbox manager: " + std::to_string(state.career.managerInbox.size()) +
                            "\r\nScouting: " + std::to_string(state.career.scoutInbox.size()) +
                            "\r\nFiltro actual: " + state.currentFilter;
    model.detail.content = inbox_service::buildInboxDigest(state.career, 8) + "\r\n" +
                           lastMatchPanelText(state.career, 5, 8) + "\r\n" + dressingRoomPanelText(state.career, 4);
    return model;
}

GuiPageModel buildModel(AppState& state) {
    switch (state.currentPage) {
        case GuiPage::Dashboard: return buildDashboardModel(state);
        case GuiPage::Squad: return buildSquadModel(state, false);
        case GuiPage::Tactics: return buildTacticsModel(state);
        case GuiPage::Calendar: return buildCalendarModel(state);
        case GuiPage::League: return buildLeagueModel(state);
        case GuiPage::Transfers: return buildTransfersModel(state);
        case GuiPage::Finances: return buildFinancesModel(state);
        case GuiPage::Youth: return buildSquadModel(state, true);
        case GuiPage::Board: return buildBoardModel(state);
        case GuiPage::News: return buildNewsModel(state);
    }
    return buildDashboardModel(state);
}

void renderFeed(HWND list, const std::vector<std::string>& lines) {
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    std::vector<std::string> safeLines = lines;
    if (safeLines.empty()) safeLines.push_back("No hay noticias recientes.");
    for (const auto& line : safeLines) {
        std::wstring wide = utf8ToWide(line);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
    }
}

void renderListPanel(HWND label, HWND list, const ListPanelModel& model) {
    setWindowTextUtf8(label, friendlyPanelTitle(model.title));
    resetListViewColumns(list, model.columns);
    clearListView(list);
    for (const auto& row : model.rows) {
        addListViewRow(list, row);
    }
}

void refreshFilterComboOptions(AppState& state) {
    if (!state.filterCombo) return;
    std::vector<std::string> options = filterOptionsForPage(state.currentPage);
    if (std::find(options.begin(), options.end(), state.currentFilter) == options.end()) {
        state.currentFilter = options.empty() ? std::string() : options.front();
    }
    state.suppressFilterEvents = true;
    SendMessageW(state.filterCombo, CB_RESETCONTENT, 0, 0);
    int selected = 0;
    for (size_t i = 0; i < options.size(); ++i) {
        addComboItem(state.filterCombo, options[i]);
        if (options[i] == state.currentFilter) selected = static_cast<int>(i);
    }
    if (!options.empty()) SendMessageW(state.filterCombo, CB_SETCURSEL, selected, 0);
    state.suppressFilterEvents = false;
}

}  // namespace

std::string selectedDivisionId(const AppState& state) {
    int index = comboIndex(state.divisionCombo);
    if (index < 0 || index >= static_cast<int>(state.career.divisions.size())) return std::string();
    return state.career.divisions[static_cast<size_t>(index)].id;
}

void fillDivisionCombo(AppState& state, const std::string& selectedId) {
    state.suppressComboEvents = true;
    SendMessageW(state.divisionCombo, CB_RESETCONTENT, 0, 0);
    int selectedIndex = 0;
    for (size_t i = 0; i < state.career.divisions.size(); ++i) {
        addComboItem(state.divisionCombo, state.career.divisions[i].display);
        if (!selectedId.empty() && state.career.divisions[i].id == selectedId) selectedIndex = static_cast<int>(i);
    }
    if (!state.career.divisions.empty()) SendMessageW(state.divisionCombo, CB_SETCURSEL, selectedIndex, 0);
    state.suppressComboEvents = false;
}

void fillTeamCombo(AppState& state, const std::string& divisionId, const std::string& selectedTeam) {
    state.suppressComboEvents = true;
    SendMessageW(state.teamCombo, CB_RESETCONTENT, 0, 0);
    std::vector<Team*> teams = state.career.getDivisionTeams(divisionId);
    int selectedIndex = 0;
    for (size_t i = 0; i < teams.size(); ++i) {
        addComboItem(state.teamCombo, teams[i]->name);
        if (!selectedTeam.empty() && teams[i]->name == selectedTeam) selectedIndex = static_cast<int>(i);
    }
    if (!teams.empty()) SendMessageW(state.teamCombo, CB_SETCURSEL, selectedIndex, 0);
    state.suppressComboEvents = false;
}

void syncManagerNameFromUi(AppState& state) {
    std::string name = getWindowTextUtf8(state.managerEdit);
    state.career.managerName = name.empty() ? "Manager" : name;
}

void syncCombosFromCareer(AppState& state) {
    fillDivisionCombo(state, state.career.activeDivision);
    fillTeamCombo(state, state.career.activeDivision, state.career.myTeam ? state.career.myTeam->name : std::string());
    if (state.career.myTeam) {
        setWindowTextUtf8(state.managerEdit, state.career.managerName);
    }
}

void setStatus(AppState& state, const std::string& text) {
    setWindowTextUtf8(state.statusLabel, text);
    if (state.statusLabel) InvalidateRect(state.statusLabel, nullptr, TRUE);
}

void refreshCurrentPage(AppState& state) {
    refreshFilterComboOptions(state);
    state.currentModel = buildModel(state);
    bool dashboardEmptyState = state.currentPage == GuiPage::Dashboard && !state.career.myTeam;

    setWindowTextUtf8(state.pageTitleLabel, state.currentModel.title);
    setWindowTextUtf8(state.breadcrumbLabel, state.currentModel.breadcrumb);
    setWindowTextUtf8(state.infoLabel, state.currentModel.infoLine);
    setWindowTextUtf8(state.summaryLabel, friendlyPanelTitle(state.currentModel.summary.title));
    setWindowTextUtf8(state.summaryEdit, state.currentModel.summary.content);
    setWindowTextUtf8(state.detailLabel, friendlyPanelTitle(state.currentModel.detail.title));
    setWindowTextUtf8(state.detailEdit, state.currentModel.detail.content);
    setWindowTextUtf8(state.newsLabel, friendlyPanelTitle(state.currentModel.feed.title));
    if (state.instructionButton) {
        std::string actionLabel = "Instruccion";
        if (state.currentPage == GuiPage::Dashboard) actionLabel = "Reunion";
        else if (state.currentPage == GuiPage::Squad || state.currentPage == GuiPage::Youth) actionLabel = "Hablar";
        setWindowTextUtf8(state.instructionButton, actionLabel);
    }

    renderListPanel(state.tableLabel, state.tableList, state.currentModel.primary);
    renderListPanel(state.squadLabel, state.squadList, state.currentModel.secondary);
    renderListPanel(state.transferLabel, state.transferList, state.currentModel.footer);
    renderFeed(state.newsList, state.currentModel.feed.lines);

    bool showTable = state.currentPage != GuiPage::Tactics && !dashboardEmptyState;
    bool showTableLabel = showTable;
    bool showSquad = !dashboardEmptyState;
    bool showSquadLabel = showSquad;
    bool showFooter = !dashboardEmptyState;
    bool showFooterLabel = showFooter;
    bool showFilter = !dashboardEmptyState;
    ShowWindow(state.tableList, showTable ? SW_SHOW : SW_HIDE);
    ShowWindow(state.tableLabel, showTableLabel ? SW_SHOW : SW_HIDE);
    ShowWindow(state.squadList, showSquad ? SW_SHOW : SW_HIDE);
    ShowWindow(state.squadLabel, showSquadLabel ? SW_SHOW : SW_HIDE);
    ShowWindow(state.transferList, showFooter ? SW_SHOW : SW_HIDE);
    ShowWindow(state.transferLabel, showFooterLabel ? SW_SHOW : SW_HIDE);
    ShowWindow(state.filterLabel, showFilter ? SW_SHOW : SW_HIDE);
    ShowWindow(state.filterCombo, showFilter ? SW_SHOW : SW_HIDE);

    layoutWindow(state);
    if (state.window) InvalidateRect(state.window, nullptr, FALSE);
}

void refreshAll(AppState& state) {
    bool hasCareer = state.career.myTeam != nullptr;
    EnableWindow(state.saveButton, hasCareer);
    EnableWindow(state.simulateButton, hasCareer);
    refreshCurrentPage(state);
}

void setCurrentPage(AppState& state, GuiPage page) {
    state.currentPage = page;
    std::vector<std::string> options = filterOptionsForPage(page);
    state.currentFilter = options.empty() ? std::string() : options.front();
    refreshCurrentPage(state);
}

void handleFilterChange(AppState& state) {
    state.currentFilter = comboText(state.filterCombo);
    refreshCurrentPage(state);
}

void handleListSelectionChange(AppState& state, int controlId) {
    int row = -1;
    if (controlId == IDC_SQUAD_LIST) row = selectedListViewRow(state.squadList);
    if (controlId == IDC_TRANSFER_LIST) row = selectedListViewRow(state.transferList);
    if (controlId == IDC_TABLE_LIST) row = selectedListViewRow(state.tableList);
    if (row < 0) return;

    if ((state.currentPage == GuiPage::Squad || state.currentPage == GuiPage::Youth) && controlId == IDC_SQUAD_LIST) {
        state.selectedPlayerName = listViewText(state.squadList, row, 0);
        refreshCurrentPage(state);
        return;
    }
    if (state.currentPage == GuiPage::Transfers && controlId == IDC_SQUAD_LIST) {
        state.selectedTransferPlayer = listViewText(state.squadList, row, 0);
        state.selectedTransferClub = listViewText(state.squadList, row, 10);
        refreshCurrentPage(state);
    }
}

void handleListColumnClick(AppState& state, const NMLISTVIEW& view) {
    if ((state.currentPage == GuiPage::Squad || state.currentPage == GuiPage::Youth) && view.hdr.idFrom == IDC_SQUAD_LIST) {
        if (state.squadSort.column == view.iSubItem) {
            state.squadSort.ascending = !state.squadSort.ascending;
        } else {
            state.squadSort.column = view.iSubItem;
            state.squadSort.ascending = true;
        }
        refreshCurrentPage(state);
    }
}

}  // namespace gui_win32

#endif
