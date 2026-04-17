#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "ai/ai_transfer_manager.h"
#include "engine/team_personality.h"
#include "career/inbox_service.h"
#include "career/manager_advice.h"
#include "finance/finance_system.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <sstream>

namespace {

bool lineMatchesAnyKeyword(const std::string& line, const std::vector<std::string>& keywords) {
    const std::string lower = toLower(line);
    for (const std::string& keyword : keywords) {
        if (lower.find(keyword) != std::string::npos) return true;
    }
    return false;
}

std::vector<std::string> selectFeedLines(const std::vector<std::string>& source,
                                         const std::vector<std::string>& keywords,
                                         std::size_t limit,
                                         const std::vector<std::string>& fallback) {
    std::vector<std::string> out;
    for (const std::string& line : source) {
        if (!lineMatchesAnyKeyword(line, keywords)) continue;
        out.push_back(line);
        if (out.size() >= limit) return out;
    }
    if (!fallback.empty()) {
        const std::size_t copyCount = std::min(limit, fallback.size());
        out.insert(out.end(), fallback.begin(), fallback.begin() + static_cast<long long>(copyCount));
    }
    if (out.empty()) out.push_back("No hay entradas para el filtro actual.");
    return out;
}

}  // namespace

namespace gui_win32 {

GuiPageModel buildTransfersModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    const auto actionLines = manager_advice::buildManagerActionLines(state.career, 3);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state, alerts);
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
    model.feed.lines = buildFeedLines(state.career, "Mercado");

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
                            "Red scouting " + (team.scoutingRegions.empty() ? std::string("-") : joinStringValues(team.scoutingRegions, ", ")) +
                            (actionLines.empty() ? std::string() : "\r\nPlan corto: " + actionLines.front());
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
            detail << "Competencia " << selectedPreview->competitionLabel
                   << " | Recomendacion " << selectedPreview->actionLabel << "\r\n";
            detail << "Plan economico: " << selectedPreview->packageLabel << "\r\n";
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
    const auto actionLines = manager_advice::buildManagerActionLines(state.career, 4);
    const auto storyLines = manager_advice::buildCareerStorylines(state.career, 2);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state, alerts);
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
    model.summary.content = buildClubSummaryService(state.career) + "\r\nFiltro actual: " + state.currentFilter;

    const std::vector<std::vector<std::string> > financeRows = {
        {"Presupuesto", formatMoneyValue(team.budget), team.budget >= 0 ? "Caja operativa positiva" : "Caja comprometida"},
        {"Deuda", formatMoneyValue(team.debt), team.debt > 0 ? "Vigilar amortizacion" : "Sin deuda relevante"},
        {"Sponsor semanal", formatMoneyValue(finance.sponsorIncome), "Ingreso fijo"},
        {"Taquilla estimada", formatMoneyValue(finance.matchdayIncome), "Proyeccion de partido local"},
        {"Merchandising", formatMoneyValue(finance.merchandisingIncome), "Pulso comercial del club"},
        {"Bonos variables", formatMoneyValue(finance.bonusIncome), "Premios por rendimiento"},
        {"Masa salarial", formatMoneyValue(finance.wageBill), "Costo semanal proyectado"},
        {"Buffer de mercado", formatMoneyValue(finance.transferBuffer), finance.riskLevel}
    };

    std::vector<const Player*> players;
    for (const auto& player : team.players) players.push_back(&player);
    std::sort(players.begin(), players.end(), [](const Player* left, const Player* right) { return left->wage > right->wage; });
    std::vector<std::vector<std::string> > salaryRows;
    for (size_t i = 0; i < players.size() && i < 14; ++i) {
        const Player& player = *players[i];
        std::string risk = player.contractWeeks <= 12 ? "Vence pronto" : (player.wantsToLeave ? "Quiere salir" : "Controlado");
        salaryRows.push_back({
            player.name, formatMoneyValue(player.wage), std::to_string(player.contractWeeks), player.promisedRole, risk
        });
    }
    std::vector<std::vector<std::string> > infrastructureRows = {
        {"Cantera", std::to_string(team.youthFacilityLevel), team.youthCoachName, "Coach " + std::to_string(team.youthCoach) + " | impacta intake y potencial"},
        {"Entrenamiento", std::to_string(team.trainingFacilityLevel), team.fitnessCoachName, "Coach " + std::to_string(team.fitnessCoach) + " | progreso y recuperacion"},
        {"Arqueros", std::to_string(team.goalkeepingCoach), team.goalkeepingCoachName, "Sostiene rendimiento del portero"},
        {"Scouting", std::to_string(team.scoutingChief), team.scoutingChiefName, "Mercado, cobertura e informes"},
        {"Analisis", std::to_string(team.performanceAnalyst), team.performanceAnalystName, "Microciclo y preparacion rival"},
        {"Estadio", std::to_string(team.stadiumLevel), std::to_string(team.fanBase), "Impacta recaudacion"}
    };

    std::vector<std::string> infrastructureFeed = {
        "Cantera nivel " + std::to_string(team.youthFacilityLevel) + " con " + team.youthCoachName + " al mando.",
        "Entrenamiento nivel " + std::to_string(team.trainingFacilityLevel) + " | coach " + team.fitnessCoachName + ".",
        "Scouting " + std::to_string(team.scoutingChief) + " | cobertura " +
            (team.scoutingRegions.empty() ? std::string("local") : joinStringValues(team.scoutingRegions, ", ")),
        "Analisis " + std::to_string(team.performanceAnalyst) + " | microciclo y rival."
    };
    const std::vector<std::string> salaryFeed =
        selectFeedLines(alerts, {"contrato", "salida"}, 8, alerts);

    model.detail.content = "Presupuesto " + formatMoneyValue(team.budget) +
                           "\r\nDeuda " + formatMoneyValue(team.debt) +
                           "\r\nSponsor " + formatMoneyValue(finance.sponsorIncome) +
                           "\r\nTaquilla " + formatMoneyValue(finance.matchdayIncome) +
                           "\r\nMerchandising " + formatMoneyValue(finance.merchandisingIncome) +
                           "\r\nBonos variables " + formatMoneyValue(finance.bonusIncome) +
                           "\r\nBuffer de mercado " + formatMoneyValue(finance.transferBuffer) +
                           "\r\nRiesgo " + finance.riskLevel;

    if (state.currentFilter == "Salarios") {
        model.infoLine = "Masa salarial, contratos cortos y riesgo de salida.";
        model.primary.title = "SalaryTable";
        model.primary.columns = {{L"Jugador", 200}, {L"Salario", 110}, {L"Contrato", 70}, {L"Rol", 110}, {L"Riesgo", 160}};
        model.primary.rows = salaryRows;
        model.secondary.title = "FinanceBreakdown";
        model.secondary.columns = {{L"Cuenta", 180}, {L"Valor", 140}, {L"Lectura", 280}};
        model.secondary.rows = financeRows;
        model.footer.title = "Infrastructure";
        model.footer.columns = {{L"Area", 160}, {L"Nivel", 70}, {L"Staff", 80}, {L"Impacto", 260}};
        model.footer.rows = infrastructureRows;
        model.feed.lines = salaryFeed;
        model.detail.content =
            "Masa salarial " + formatMoneyValue(finance.wageBill) +
            "\r\nJugadores monitorizados " + std::to_string(salaryRows.size()) +
            "\r\nContratos cortos " + std::to_string(std::count_if(team.players.begin(),
                                                                     team.players.end(),
                                                                     [](const Player& player) { return player.contractWeeks <= 12; })) +
            "\r\nFiltro salarial activo";
    } else if (state.currentFilter == "Infraestructura") {
        model.infoLine = "Instalaciones, staff y estructura de soporte del club.";
        model.primary.title = "Infrastructure";
        model.primary.columns = {{L"Area", 160}, {L"Nivel", 70}, {L"Staff", 80}, {L"Impacto", 260}};
        model.primary.rows = infrastructureRows;
        model.secondary.title = "FinanceBreakdown";
        model.secondary.columns = {{L"Cuenta", 180}, {L"Valor", 140}, {L"Lectura", 280}};
        model.secondary.rows = financeRows;
        model.footer.title = "SalaryTable";
        model.footer.columns = {{L"Jugador", 200}, {L"Salario", 110}, {L"Contrato", 70}, {L"Rol", 110}, {L"Riesgo", 160}};
        model.footer.rows.assign(salaryRows.begin(),
                                 salaryRows.begin() + static_cast<long long>(std::min<std::size_t>(salaryRows.size(), 8)));
        model.feed.lines = infrastructureFeed;
        model.detail.content =
            "Cantera nivel " + std::to_string(team.youthFacilityLevel) +
            "\r\nEntrenamiento nivel " + std::to_string(team.trainingFacilityLevel) +
            "\r\nScouting " + std::to_string(team.scoutingChief) +
            "\r\nAnalisis " + std::to_string(team.performanceAnalyst) +
            "\r\nFiltro de infraestructura activo";
    } else {
        model.primary.rows = financeRows;
        model.secondary.rows = salaryRows;
        model.footer.rows = infrastructureRows;
        model.feed.lines = alerts;
    }

    if (!actionLines.empty()) {
        model.detail.content += "\r\n\r\nAcciones sugeridas";
        for (const auto& line : actionLines) model.detail.content += "\r\n- " + line;
    }
    if (!storyLines.empty()) {
        model.detail.content += "\r\n\r\nNarrativa";
        for (const auto& line : storyLines) model.detail.content += "\r\n- " + line;
    }
    return model;
}

GuiPageModel buildBoardModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    const auto actionLines = manager_advice::buildManagerActionLines(state.career, 4);
    const auto storyLines = manager_advice::buildCareerStorylines(state.career, 3);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state, alerts);
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
    if (!actionLines.empty()) {
        model.detail.content += "\r\n\r\nAcciones sugeridas";
        for (const auto& line : actionLines) model.detail.content += "\r\n- " + line;
    }
    if (!storyLines.empty()) {
        model.detail.content += "\r\n\r\nNarrativa de la semana";
        for (const auto& line : storyLines) model.detail.content += "\r\n- " + line;
    }

    std::vector<std::vector<std::string> > objectiveRows;
    std::vector<std::vector<std::string> > profileRows;
    std::vector<std::vector<std::string> > historyRows;

    if (state.career.myTeam) {
        objectiveRows.push_back({"Puesto esperado", std::to_string(state.career.boardExpectedFinish), "Meta de temporada"});
        objectiveRows.push_back({"Objetivo mensual", state.career.boardMonthlyObjective.empty() ? "Sin objetivo" : state.career.boardMonthlyObjective,
                                 std::to_string(state.career.boardMonthlyProgress) + "/" + std::to_string(state.career.boardMonthlyTarget)});
        objectiveRows.push_back({"Confianza", std::to_string(state.career.boardConfidence) + "/100", boardStatusLabel(state.career.boardConfidence)});
        objectiveRows.push_back({"Advertencias", std::to_string(state.career.boardWarningWeeks), "Semanas bajo revision"});

        Team& team = *state.career.myTeam;
        profileRows.push_back({"Expectativa", teamExpectationLabel(team), "Perfil de club"});
        profileRows.push_back({"Prestigio", std::to_string(team.clubPrestige), "Influye en fichajes"});
        profileRows.push_back({"DT del club", team.headCoachName, team.headCoachStyle});
        profileRows.push_back({"Antiguedad DT", std::to_string(team.headCoachTenureWeeks) + " sem", "Tiempo del proyecto actual"});
        profileRows.push_back({"Seguridad", std::to_string(team.jobSecurity), "Estabilidad del banquillo"});
        profileRows.push_back({"Politica", team.transferPolicy, "Mercado del club"});
        profileRows.push_back({"Perfil competitivo", teamPersonalityHeadline(team), "Como se refleja la identidad en decisiones CPU"});
        profileRows.push_back({"Red scouting", team.scoutingRegions.empty() ? std::string("-") : joinStringValues(team.scoutingRegions, ", "), "Cobertura activa"});
        profileRows.push_back({"Identidad cantera", team.youthIdentity, "Condiciona objetivos"});
        profileRows.push_back({"Estilo", team.clubStyle, "Contexto institucional"});
    }
    for (auto it = state.career.history.rbegin(); it != state.career.history.rend() && historyRows.size() < 8; ++it) {
        historyRows.push_back({
            std::to_string(it->season), it->club, std::to_string(it->finish), it->champion, it->note
        });
    }

    const std::vector<std::string> objectiveFeed =
        selectFeedLines(alerts, {"directiva", "objetivo", "promesa"}, 8, alerts);
    std::vector<std::string> historyFeed;
    for (auto it = state.career.history.rbegin(); it != state.career.history.rend() && historyFeed.size() < 8; ++it) {
        historyFeed.push_back("T" + std::to_string(it->season) + " | " + it->club + " | puesto " +
                              std::to_string(it->finish) + " | campeon " + it->champion);
    }
    if (historyFeed.empty()) historyFeed = alerts;

    if (state.currentFilter == "Objetivos") {
        model.infoLine = "Seguimiento fino de metas, confianza y riesgo institucional.";
        model.primary.rows = objectiveRows;
        model.secondary.rows = profileRows;
        model.footer.rows = historyRows;
        model.feed.lines = objectiveFeed;
        model.detail.content = buildBoardSummaryService(state.career) + "\r\n\r\nVista priorizada: objetivos y riesgo de directiva.";
    } else if (state.currentFilter == "Historial") {
        model.infoLine = "Memoria del proyecto: temporadas previas y contexto institucional.";
        model.primary.title = "CoachHistory";
        model.primary.columns = {{L"Temp", 60}, {L"Club", 170}, {L"Puesto", 60}, {L"Campeon", 180}, {L"Nota", 240}};
        model.primary.rows = historyRows;
        model.secondary.title = "BoardObjectiveTable";
        model.secondary.columns = {{L"Objetivo", 220}, {L"Estado", 120}, {L"Contexto", 220}};
        model.secondary.rows = objectiveRows;
        model.footer.title = "ClubProfile";
        model.footer.columns = {{L"Perfil", 160}, {L"Valor", 120}, {L"Lectura", 220}};
        model.footer.rows = profileRows;
        model.feed.lines = historyFeed;
        model.detail.content = buildBoardSummaryService(state.career) + "\r\n\r\nVista historica priorizada.";
    } else {
        model.primary.rows = objectiveRows;
        model.secondary.rows = profileRows;
        model.footer.rows = historyRows;
    }

    model.summary.content += "\r\nFiltro actual: " + state.currentFilter;
    return model;
}

GuiPageModel buildNewsModel(AppState& state) {
    GuiPageModel model;
    std::vector<std::string> alerts = buildAlertLines(state.career);
    const auto storyLines = manager_advice::buildCareerStorylines(state.career, 4);
    const auto hubLines = inbox_service::buildPriorityInboxLines(state.career, 12);
    const auto weeklyDecisionOptions = buildWeeklyDecisionOptions(state.career);
    model.title = pageTitleFor(state.currentPage);
    model.breadcrumb = breadcrumbFor(state.currentPage);
    model.metrics = buildMetrics(state, alerts);
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

    for (const auto& line : hubLines) {
        std::string channel = "Centro";
        std::string detail = line;
        const std::size_t separator = line.find(" | ");
        if (separator != std::string::npos) {
            channel = line.substr(0, separator);
            detail = line.substr(separator + 3);
        }
        model.secondary.rows.push_back({channel, detail});
    }
    if (model.secondary.rows.empty()) model.secondary.rows.push_back({"Inbox", "Sin novedades recientes"});

    for (const auto& alert : alerts) {
        model.footer.rows.push_back({"Alerta", alert});
    }
    model.summary.content = "Entradas visibles: " + std::to_string(model.feed.lines.size()) +
                            "\r\nInbox manager: " + std::to_string(state.career.managerInbox.size()) +
                            "\r\nScouting: " + std::to_string(state.career.scoutInbox.size()) +
                            "\r\nAsignaciones: " + std::to_string(state.career.scoutingAssignments.size()) +
                            "\r\nFiltro actual: " + state.currentFilter;
    if (!weeklyDecisionOptions.empty()) {
        model.summary.content += "\r\nDecision semanal: " + weeklyDecisionOptions.front();
        model.summary.content += "\r\nBoton Decidir: aplica la recomendacion del staff.";
    }
    if (!hubLines.empty()) {
        model.summary.content += "\r\nPrioridad: " + hubLines.front();
    }
    if (!storyLines.empty()) {
        model.summary.content += "\r\nNarrativa activa: " + storyLines.front();
    }
    model.detail.content = inbox_service::buildManagerHubDigest(state.career, 8) + "\r\n" +
                           lastMatchPanelText(state.career, 5, 8) + "\r\n" + dressingRoomPanelText(state.career, 4);
    if (!weeklyDecisionOptions.empty()) {
        model.detail.content += "\r\n\r\nCentro semanal";
        for (size_t i = 0; i < weeklyDecisionOptions.size() && i < 6; ++i) {
            model.detail.content += "\r\n- " + weeklyDecisionOptions[i];
        }
    }
    if (!storyLines.empty()) {
        model.detail.content += "\r\n\r\nNarrativa de la semana";
        for (const auto& line : storyLines) model.detail.content += "\r\n- " + line;
    }
    return model;
}

}  // namespace gui_win32

#endif

