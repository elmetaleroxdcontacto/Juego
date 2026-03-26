#include "gui/gui_internal.h"

#include "gui/gui_audio.h"
#include "gui/gui_view_builders.h"

#ifdef _WIN32

#include "utils/utils.h"

#include <algorithm>
#include <sstream>

namespace gui_win32 {

GuiPageModel buildModel(AppState& state);

namespace {

std::vector<HWND> pageRefreshTargets(const AppState& state) {
    return {
        state.pageTitleLabel, state.breadcrumbLabel, state.infoLabel, state.filterLabel, state.filterCombo,
        state.summaryLabel, state.summaryEdit, state.tableLabel, state.tableList,
        state.squadLabel, state.squadList, state.transferLabel, state.transferList,
        state.detailLabel, state.detailEdit, state.newsLabel, state.newsList, state.statusLabel,
        state.scoutActionButton, state.shortlistButton, state.followShortlistButton, state.buyButton,
        state.preContractButton, state.renewButton, state.sellButton, state.planButton, state.instructionButton,
        state.youthUpgradeButton, state.trainingUpgradeButton, state.scoutingUpgradeButton, state.stadiumUpgradeButton,
        state.emptyNewButton, state.emptyLoadButton, state.emptyValidateButton
    };
}

std::string pageCacheKey(const AppState& state, GuiPage page) {
    std::ostringstream out;
    out << static_cast<int>(page) << "|" << state.currentFilter;
    return out.str();
}

std::string pageCacheSignature(const AppState& state, GuiPage page) {
    const Career& career = state.career;
    std::ostringstream out;
    out << static_cast<int>(page)
        << "|" << state.currentFilter
        << "|" << career.currentSeason
        << "|" << career.currentWeek
        << "|" << career.newsFeed.size()
        << "|" << career.pendingTransfers.size()
        << "|" << career.scoutingShortlist.size()
        << "|" << career.boardConfidence
        << "|" << state.selectedPlayerName
        << "|" << state.selectedTransferPlayer
        << "|" << state.selectedTransferClub;
    if (career.myTeam) {
        out << "|" << career.myTeam->name
            << "|" << career.myTeam->budget
            << "|" << career.myTeam->morale
            << "|" << career.myTeam->players.size()
            << "|" << career.myTeam->points;
    }
    return out.str();
}

bool canUseCachedModel(const AppState& state) {
    return isHeavyPage(state.currentPage);
}

class ScopedRefreshRedrawLock {
public:
    explicit ScopedRefreshRedrawLock(AppState& state) : state_(state), targets_(pageRefreshTargets(state)) {
        if (state_.window) SendMessageW(state_.window, WM_SETREDRAW, FALSE, 0);
        for (HWND target : targets_) {
            if (target && IsWindow(target)) SendMessageW(target, WM_SETREDRAW, FALSE, 0);
        }
    }

    ~ScopedRefreshRedrawLock() {
        for (HWND target : targets_) {
            if (!target || !IsWindow(target)) continue;
            SendMessageW(target, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(target, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
        }
        if (state_.window) {
            SendMessageW(state_.window, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(state_.window,
                         nullptr,
                         nullptr,
                         RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        state_.pageRefreshInProgress = false;
    }

private:
    AppState& state_;
    std::vector<HWND> targets_;
};

std::string friendlyPanelTitle(const std::string& title) {
    if (title == "FrontMenuOverview") return "Panorama de arranque";
    if (title == "FrontMenuProfile") return "Perfil del manager";
    if (title == "FrontMenuRoadmap") return "Hoja de ruta";
    if (title == "SettingsDesk") return "Cabina de ajustes";
    if (title == "SettingsImpact") return "Impacto";
    if (title == "SettingsReturn") return "Retorno y estado";
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
    if (title == "LaunchChecklistPanel") return "Paso a paso";
    if (title == "GameSetupStatusPanel") return "Estado actual de la partida";
    if (title == "GameSetupChecklistPanel") return "Checklist dinamico";
    return title;
}

std::vector<std::string> filterOptionsForPage(GuiPage page) {
    switch (page) {
        case GuiPage::MainMenu:
        case GuiPage::Settings:
        case GuiPage::Credits:
            return {};
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

void autosizeVisibleLists(AppState& state) {
    if (state.tableList && IsWindowVisible(state.tableList)) {
        autosizeListViewColumns(state, state.tableList, state.currentModel.primary.columns);
    }
    if (state.squadList && IsWindowVisible(state.squadList)) {
        autosizeListViewColumns(state, state.squadList, state.currentModel.secondary.columns);
    }
    if (state.transferList && IsWindowVisible(state.transferList)) {
        autosizeListViewColumns(state, state.transferList, state.currentModel.footer.columns);
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

bool isKnownDivision(const AppState& state, const std::string& divisionId) {
    for (const auto& division : state.career.divisions) {
        if (division.id == divisionId) return true;
    }
    return false;
}

std::string resolveKnownDivisionId(const AppState& state, const std::string& rawValue) {
    const std::string normalized = toLower(trim(rawValue));
    if (normalized.empty()) return std::string();
    for (const auto& division : state.career.divisions) {
        if (division.id == normalized || toLower(trim(division.display)) == normalized) {
            return division.id;
        }
    }
    return std::string();
}

std::string inferHeaderDivisionId(const AppState& state) {
    std::vector<std::string> candidates;
    if (!state.gameSetup.division.empty()) candidates.push_back(state.gameSetup.division);
    if (!state.career.activeDivision.empty()) candidates.push_back(state.career.activeDivision);
    if (state.career.myTeam) {
        candidates.push_back(state.career.myTeam->division);
        candidates.push_back(state.career.myTeam->name);
    }
    if (!state.gameSetup.club.empty()) candidates.push_back(state.gameSetup.club);

    for (const auto& candidate : candidates) {
        const std::string resolved = resolveKnownDivisionId(state, candidate);
        if (!resolved.empty()) return resolved;

        for (const auto& team : state.career.allTeams) {
            if (toLower(trim(team.name)) != toLower(trim(candidate))) continue;
            const std::string teamDivision = resolveKnownDivisionId(state, team.division);
            if (!teamDivision.empty()) return teamDivision;
        }
    }
    return std::string();
}

bool clubBelongsToDivision(const AppState& state, const std::string& divisionId, const std::string& clubName) {
    if (divisionId.empty() || clubName.empty()) return false;
    return std::any_of(state.career.allTeams.begin(), state.career.allTeams.end(), [&](const Team& team) {
        return team.division == divisionId && team.name == clubName;
    });
}

bool hasAvailableTeams(const AppState& state, const std::string& divisionId) {
    if (divisionId.empty()) return false;
    return std::any_of(state.career.allTeams.begin(), state.career.allTeams.end(), [&](const Team& team) {
        return team.division == divisionId;
    });
}

bool hasPersistedCareer(const AppState& state) {
    const std::string savePath = state.career.saveFile.empty() ? std::string("saves/career_save.txt") : state.career.saveFile;
    return pathExists(savePath) || (savePath == "saves/career_save.txt" && pathExists("career_save.txt"));
}

}  // namespace

bool isFrontMenuPage(GuiPage page) {
    return page == GuiPage::MainMenu || page == GuiPage::Settings || page == GuiPage::Credits;
}

bool isHeavyPage(GuiPage page) {
    return page == GuiPage::Transfers || page == GuiPage::Finances || page == GuiPage::News;
}

namespace {

void syncSetupButtonsAndHints(AppState& state) {
    const bool hasCareer = state.career.myTeam != nullptr;
    const bool hasSavedCareer = hasPersistedCareer(state);
    EnableWindow(state.divisionCombo, !hasCareer);
    EnableWindow(state.teamCombo, !hasCareer && hasAvailableTeams(state, state.gameSetup.division));
    EnableWindow(state.newCareerButton, !hasCareer && state.gameSetup.ready);
    EnableWindow(state.emptyNewButton, state.gameSetup.ready);
    EnableWindow(state.saveButton, hasCareer);
    EnableWindow(state.simulateButton, hasCareer);
    EnableWindow(state.menuContinueButton, hasCareer || hasSavedCareer);
    EnableWindow(state.menuLoadButton, hasSavedCareer);

    if (state.divisionLabel) {
        std::string badge = "Division";
        const std::string resolved = resolveKnownDivisionId(state, state.gameSetup.division.empty() ? state.career.activeDivision : state.gameSetup.division);
        if (hasCareer && !resolved.empty()) {
            badge += " [" + divisionDisplay(resolved) + "]";
        }
        setWindowTextUtf8(state.divisionLabel, badge);
    }

    if (state.managerHelpLabel) {
        std::string helper;
        if (hasCareer) {
            const std::string resolved = resolveKnownDivisionId(state, state.career.activeDivision);
            helper = "Carrera activa.";
            if (!resolved.empty()) helper += " Division resuelta: " + divisionDisplay(resolved) + ".";
            helper += " Guardar y simular ya estan disponibles.";
        } else {
            helper = state.gameSetup.managerError;
            if (helper.empty()) {
                helper = state.gameSetup.manager.empty()
                    ? "Completa el nombre del manager."
                    : (state.gameSetup.ready ? "Manager valido. Puedes iniciar la partida."
                                             : "Manager listo. Completa los pasos pendientes.");
            }
        }
        setWindowTextUtf8(state.managerHelpLabel, helper);
        InvalidateRect(state.managerHelpLabel, nullptr, TRUE);
    }
    if (state.newCareerButton) InvalidateRect(state.newCareerButton, nullptr, TRUE);
    if (state.emptyNewButton) InvalidateRect(state.emptyNewButton, nullptr, TRUE);
}

}  // namespace

std::string selectedDivisionId(const AppState& state) {
    if (!state.gameSetup.division.empty()) return state.gameSetup.division;
    int index = comboIndex(state.divisionCombo);
    if (index < 0 || index >= static_cast<int>(state.career.divisions.size())) return std::string();
    return state.career.divisions[static_cast<size_t>(index)].id;
}

void fillDivisionCombo(AppState& state, const std::string& selectedId) {
    state.suppressComboEvents = true;
    SendMessageW(state.divisionCombo, CB_RESETCONTENT, 0, 0);
    int selectedIndex = -1;
    for (size_t i = 0; i < state.career.divisions.size(); ++i) {
        addComboItem(state.divisionCombo, state.career.divisions[i].display);
        if (!selectedId.empty() && state.career.divisions[i].id == selectedId) selectedIndex = static_cast<int>(i);
    }
    SendMessageW(state.divisionCombo, CB_SETCURSEL, selectedIndex, 0);
    state.suppressComboEvents = false;
}

void fillTeamCombo(AppState& state, const std::string& divisionId, const std::string& selectedTeam) {
    state.suppressComboEvents = true;
    SendMessageW(state.teamCombo, CB_RESETCONTENT, 0, 0);
    std::vector<Team*> teams = state.career.getDivisionTeams(divisionId);
    int selectedIndex = -1;
    for (size_t i = 0; i < teams.size(); ++i) {
        addComboItem(state.teamCombo, teams[i]->name);
        if (!selectedTeam.empty() && teams[i]->name == selectedTeam) selectedIndex = static_cast<int>(i);
    }
    SendMessageW(state.teamCombo, CB_SETCURSEL, selectedIndex, 0);
    state.suppressComboEvents = false;
    EnableWindow(state.teamCombo, !state.career.myTeam && !divisionId.empty() && !teams.empty());
}

void syncManagerNameFromUi(AppState& state) {
    std::string name = getWindowTextUtf8(state.managerEdit);
    state.gameSetup.manager = trim(name);
    if (state.career.myTeam) {
        state.career.managerName = state.gameSetup.manager.empty() ? "Manager" : state.gameSetup.manager;
    }
}

void syncCombosFromCareer(AppState& state) {
    const std::string resolvedDivision = inferHeaderDivisionId(state);

    if (!resolvedDivision.empty() && state.career.activeDivision != resolvedDivision) {
        state.career.setActiveDivision(resolvedDivision);
    }
    if (state.career.myTeam) {
        state.gameSetup.division = !resolvedDivision.empty() ? resolvedDivision : state.career.myTeam->division;
        state.gameSetup.club = state.career.myTeam->name;
        state.gameSetup.manager = trim(state.career.managerName);
    } else if (!resolvedDivision.empty()) {
        state.gameSetup.division = resolvedDivision;
    }
    fillDivisionCombo(state, state.gameSetup.division);
    fillTeamCombo(state, state.gameSetup.division, state.gameSetup.club);
    if (state.managerEdit && trim(getWindowTextUtf8(state.managerEdit)) != state.gameSetup.manager) {
        setWindowTextUtf8(state.managerEdit, state.gameSetup.manager);
    }
    check_game_ready(state);
}

void set_division(AppState& state, const std::string& divisionId) {
    state.gameSetup.division = isKnownDivision(state, divisionId) ? divisionId : std::string();
    if (!clubBelongsToDivision(state, state.gameSetup.division, state.gameSetup.club)) {
        state.gameSetup.club.clear();
    }
    fillDivisionCombo(state, state.gameSetup.division);
    fillTeamCombo(state, state.gameSetup.division, state.gameSetup.club);
    check_game_ready(state);
    setStatus(state, state.gameSetup.inlineMessage);
    refreshCurrentPage(state);
}

void set_club(AppState& state, const std::string& clubName) {
    state.gameSetup.club = clubBelongsToDivision(state, state.gameSetup.division, clubName) ? clubName : std::string();
    fillTeamCombo(state, state.gameSetup.division, state.gameSetup.club);
    check_game_ready(state);
    setStatus(state, state.gameSetup.inlineMessage);
    refreshCurrentPage(state);
}

void set_manager(AppState& state, const std::string& managerName) {
    state.gameSetup.manager = trim(managerName);
    check_game_ready(state);
    setStatus(state, state.gameSetup.ready ? "Checklist completa. Nueva partida habilitada." : state.gameSetup.inlineMessage);
    refreshCurrentPage(state);
}

bool check_game_ready(AppState& state) {
    state.gameSetup.manager = trim(state.gameSetup.manager);
    state.gameSetup.ready = false;
    state.gameSetup.managerError.clear();
    state.gameSetup.inlineMessage.clear();

    if (state.gameSetup.division.empty()) {
        state.gameSetup.currentStep = 1;
        state.gameSetup.inlineMessage = "Paso 1 pendiente: elige una division para cargar clubes.";
    } else if (!hasAvailableTeams(state, state.gameSetup.division)) {
        state.gameSetup.currentStep = 2;
        state.gameSetup.inlineMessage = "Paso 2 bloqueado: esta division no tiene clubes disponibles.";
    } else if (state.gameSetup.club.empty()) {
        state.gameSetup.currentStep = 2;
        state.gameSetup.inlineMessage = "Paso 2 pendiente: selecciona el club que quieres dirigir.";
    } else if (state.gameSetup.manager.empty()) {
        state.gameSetup.currentStep = 3;
        state.gameSetup.managerError = "Ingresa nombre del manager.";
        state.gameSetup.inlineMessage = "Paso 3 pendiente: escribe el nombre del manager para iniciar.";
    } else {
        state.gameSetup.currentStep = 3;
        state.gameSetup.ready = true;
        state.gameSetup.inlineMessage = "Checklist completa. La partida esta lista para empezar.";
    }

    syncSetupButtonsAndHints(state);
    return state.gameSetup.ready;
}

void setStatus(AppState& state, const std::string& text) {
    setWindowTextUtf8(state.statusLabel, text);
    if (state.statusLabel) InvalidateRect(state.statusLabel, nullptr, TRUE);
}

void refreshCurrentPage(AppState& state) {
    if (state.pageRefreshInProgress) return;
    state.pageRefreshInProgress = true;
    ScopedRefreshRedrawLock redrawLock(state);
    const DWORD refreshStart = GetTickCount();

    if (state.career.myTeam || (state.gameSetup.division.empty() && !state.gameSetup.club.empty())) {
        syncCombosFromCareer(state);
    }
    check_game_ready(state);
    refreshFilterComboOptions(state);
    const std::string cacheKey = pageCacheKey(state, state.currentPage);
    const std::string cacheSignature = pageCacheSignature(state, state.currentPage);
    if (canUseCachedModel(state) &&
        state.modelCacheSignatures.count(cacheKey) &&
        state.modelCacheSignatures[cacheKey] == cacheSignature &&
        state.modelCache.count(cacheKey)) {
        state.currentModel = state.modelCache[cacheKey];
    } else {
        state.currentModel = buildModel(state);
        if (canUseCachedModel(state)) {
            state.modelCache[cacheKey] = state.currentModel;
            state.modelCacheSignatures[cacheKey] = cacheSignature;
        }
    }
    syncMenuMusicForPage(state);
    state.insightHotspots.clear();
    bool dashboardEmptyState = state.currentPage == GuiPage::Dashboard && !state.career.myTeam;
    const bool frontMenuPage = isFrontMenuPage(state.currentPage);

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
        else if (state.currentPage == GuiPage::Board) actionLabel = "Staff";
        else if (state.currentPage == GuiPage::News) actionLabel = "Decidir";
        setWindowTextUtf8(state.instructionButton, actionLabel);
    }
    if (state.planButton) {
        std::string planLabel = "Plan";
        if (state.currentPage == GuiPage::Squad) planLabel = "Instrucc.";
        setWindowTextUtf8(state.planButton, planLabel);
    }
    if (state.scoutActionButton) {
        setWindowTextUtf8(state.scoutActionButton, state.currentPage == GuiPage::News ? "Asignar" : "Otear");
    }

    renderListPanel(state.tableLabel, state.tableList, state.currentModel.primary);
    renderListPanel(state.squadLabel, state.squadList, state.currentModel.secondary);
    renderListPanel(state.transferLabel, state.transferList, state.currentModel.footer);
    renderFeed(state.newsList, state.currentModel.feed.lines);

    bool showTable = !frontMenuPage && state.currentPage != GuiPage::Tactics && !dashboardEmptyState;
    bool showTableLabel = showTable;
    bool showSquad = !frontMenuPage && !dashboardEmptyState;
    bool showSquadLabel = showSquad;
    bool showFooter = !frontMenuPage && !dashboardEmptyState;
    bool showFooterLabel = showFooter;
    bool showFilter = !frontMenuPage && !dashboardEmptyState;
    ShowWindow(state.tableList, showTable ? SW_SHOW : SW_HIDE);
    ShowWindow(state.tableLabel, showTableLabel ? SW_SHOW : SW_HIDE);
    ShowWindow(state.squadList, showSquad ? SW_SHOW : SW_HIDE);
    ShowWindow(state.squadLabel, showSquadLabel ? SW_SHOW : SW_HIDE);
    ShowWindow(state.transferList, showFooter ? SW_SHOW : SW_HIDE);
    ShowWindow(state.transferLabel, showFooterLabel ? SW_SHOW : SW_HIDE);
    ShowWindow(state.filterLabel, showFilter ? SW_SHOW : SW_HIDE);
    ShowWindow(state.filterCombo, showFilter ? SW_SHOW : SW_HIDE);

    layoutWindow(state);
    autosizeVisibleLists(state);
    const long long elapsed = static_cast<long long>(GetTickCount() - refreshStart);
    state.pageTraceMs[static_cast<int>(state.currentPage)] = elapsed;
    state.lastPageTrace = pageTitleFor(state.currentPage) + " render " + std::to_string(elapsed) + " ms";
    if (isHeavyPage(state.currentPage) && state.infoLabel) {
        setWindowTextUtf8(state.infoLabel, state.currentModel.infoLine + " | " + state.lastPageTrace);
    }
}

void autosizeCurrentLists(AppState& state) {
    autosizeVisibleLists(state);
}

void refreshAll(AppState& state) {
    check_game_ready(state);
    refreshCurrentPage(state);
}

void setCurrentPage(AppState& state, GuiPage page) {
    state.currentPage = page;
    std::vector<std::string> options = filterOptionsForPage(page);
    state.currentFilter = options.empty() ? std::string() : options.front();
    refreshCurrentPage(state);
}

void queuePageTransition(AppState& state, GuiPage page) {
    if (!state.window || !IsWindow(state.window)) {
        setCurrentPage(state, page);
        return;
    }
    if (state.pageChangeQueued && state.queuedPage == page) return;
    state.queuedPage = page;
    state.pageChangeQueued = true;
    PostMessageW(state.window, kGuiPageTransitionMessage, static_cast<WPARAM>(static_cast<int>(page)), 0);
}

void handleFilterChange(AppState& state) {
    state.currentFilter = comboText(state.filterCombo);
    refreshCurrentPage(state);
}

void handleListSelectionChange(AppState& state, int controlId) {
    if (state.pageRefreshInProgress) return;
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
    if (state.pageRefreshInProgress) return;
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
