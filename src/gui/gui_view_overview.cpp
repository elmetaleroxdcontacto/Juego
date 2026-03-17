#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "ai/ai_squad_planner.h"
#include "career/analytics_service.h"
#include "career/dressing_room_service.h"
#include "career/career_support.h"
#include "utils/utils.h"

#include <map>
#include <sstream>

namespace gui_win32 {

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

}  // namespace gui_win32

#endif



