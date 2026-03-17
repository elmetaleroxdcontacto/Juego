#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "utils/utils.h"

namespace gui_win32 {

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

}  // namespace gui_win32

#endif




