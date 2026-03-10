#include "gui/gui_internal.h"

#ifdef _WIN32

#include "utils/utils.h"

#include <algorithm>
#include <array>
#include <map>
#include <sstream>

namespace gui_win32 {

namespace {

struct ActionButtonRef {
    HWND hwnd;
    int width;
};

std::vector<DashboardMetric> defaultMetrics() {
    return {
        {"Club", "Sin carrera", kThemeAccentBlue},
        {"Fecha", "Temporada 0 / Semana 0", kThemeAccent},
        {"Presupuesto", "$0", kThemeAccentGreen},
        {"Reputacion", "0", kThemeAccentBlue},
        {"Alertas", "0", kThemeWarning}
    };
}

std::wstring shortPlayerLabel(const std::string& name) {
    std::vector<std::string> parts = splitByDelimiter(trim(name), ' ');
    if (parts.empty()) return L"--";
    if (parts.size() == 1) return utf8ToWide(parts.front().substr(0, std::min<size_t>(parts.front().size(), 8)));
    std::string label = parts.back();
    if (!parts.front().empty()) {
        label = parts.front().substr(0, 1) + "." + label;
    }
    return utf8ToWide(label.substr(0, std::min<size_t>(label.size(), 10)));
}

std::vector<const Player*> playersForLine(const Team& team, const std::string& position) {
    std::vector<const Player*> players;
    std::vector<int> xi = team.getStartingXIIndices();
    for (int index : xi) {
        if (index < 0 || index >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[static_cast<size_t>(index)];
        if (normalizePosition(player.position) == position) players.push_back(&player);
    }
    return players;
}

void drawPlayerDots(HDC hdc,
                    const RECT& rect,
                    const std::vector<const Player*>& players,
                    double xRatio,
                    COLORREF fill) {
    if (players.empty()) return;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int x = rect.left + static_cast<int>(width * xRatio);
    int spacing = height / static_cast<int>(players.size() + 1);
    HBRUSH brush = CreateSolidBrush(fill);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kThemeText);
    for (size_t i = 0; i < players.size(); ++i) {
        int y = rect.top + spacing * static_cast<int>(i + 1);
        Ellipse(hdc, x - 13, y - 13, x + 13, y + 13);
        std::wstring label = shortPlayerLabel(players[i]->name);
        RECT textRect{x + 18, y - 10, rect.right - 10, y + 10};
        DrawTextW(hdc, label.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
}

void drawTopMetrics(AppState& state, HDC hdc, const RECT& client) {
    std::vector<DashboardMetric> metrics = state.currentModel.metrics.empty() ? defaultMetrics() : state.currentModel.metrics;
    const int left = 18;
    const int top = 58;
    const int gap = 10;
    const int width = std::max(150, static_cast<int>((client.right - left * 2 - gap * 4) / 5));
    const int height = 48;

    for (size_t i = 0; i < metrics.size() && i < 5; ++i) {
        RECT card{left + static_cast<int>(i) * (width + gap), top, left + static_cast<int>(i) * (width + gap) + width, top + height};
        drawRoundedPanel(hdc, card, RGB(11, 24, 33), RGB(35, 58, 74), 14);

        RECT accent = card;
        accent.bottom = accent.top + 5;
        HBRUSH accentBrush = CreateSolidBrush(metrics[i].accent);
        FillRect(hdc, &accent, accentBrush);
        DeleteObject(accentBrush);

        RECT labelRect{card.left + 12, card.top + 8, card.right - 12, card.top + 24};
        RECT valueRect{card.left + 12, card.top + 20, card.right - 12, card.bottom - 8};
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kThemeMuted);
        HGDIOBJ oldFont = SelectObject(hdc, state.font ? state.font : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
        DrawTextW(hdc, utf8ToWide(metrics[i].label).c_str(), -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, state.sectionFont ? state.sectionFont : state.font);
        SetTextColor(hdc, kThemeText);
        DrawTextW(hdc, utf8ToWide(metrics[i].value).c_str(), -1, &valueRect, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, oldFont);
    }
}

void drawTacticsBoard(AppState& state, HDC hdc, const RECT& rect) {
    drawRoundedPanel(hdc, rect, RGB(14, 31, 26), RGB(44, 86, 67), 16);
    RECT inner = rect;
    InflateRect(&inner, -14, -14);
    drawPitchOverlay(hdc, inner);

    if (!state.career.myTeam) return;
    const Team& team = *state.career.myTeam;
    drawPlayerDots(hdc, inner, playersForLine(team, "ARQ"), 0.12, RGB(66, 116, 186));
    drawPlayerDots(hdc, inner, playersForLine(team, "DEF"), 0.31, RGB(46, 142, 96));
    drawPlayerDots(hdc, inner, playersForLine(team, "MED"), 0.55, RGB(226, 191, 92));
    drawPlayerDots(hdc, inner, playersForLine(team, "DEL"), 0.79, RGB(204, 108, 74));

    RECT titleRect{inner.left + 12, inner.top + 10, inner.right - 12, inner.top + 32};
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(228, 241, 236));
    HGDIOBJ oldFont = SelectObject(hdc, state.sectionFont ? state.sectionFont : state.font);
    std::wstring title = utf8ToWide(team.formation + " | " + team.tactics);
    DrawTextW(hdc, title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);

    RECT barArea{inner.left + 12, inner.bottom - 116, inner.right - 12, inner.bottom - 12};
    int barWidth = (barArea.right - barArea.left - 18) / 2;
    std::array<RECT, 5> bars{
        RECT{barArea.left, barArea.top, barArea.left + barWidth, barArea.top + 28},
        RECT{barArea.left + barWidth + 18, barArea.top, barArea.right, barArea.top + 28},
        RECT{barArea.left, barArea.top + 34, barArea.left + barWidth, barArea.top + 62},
        RECT{barArea.left + barWidth + 18, barArea.top + 34, barArea.right, barArea.top + 62},
        RECT{barArea.left, barArea.top + 68, barArea.right, barArea.top + 96}
    };
    drawStatBar(hdc, bars[0], L"Presion", team.pressingIntensity, 5, kThemeAccentGreen);
    drawStatBar(hdc, bars[1], L"Ritmo", team.tempo, 5, kThemeAccentBlue);
    drawStatBar(hdc, bars[2], L"Anchura", team.width, 5, kThemeAccent);
    drawStatBar(hdc, bars[3], L"Linea", team.defensiveLine, 5, kThemeWarning);
    drawStatBar(hdc, bars[4], L"Moral equipo", team.morale, 100, kThemeAccentGreen);
}

void setLabelFont(HWND hwnd, HFONT font) {
    if (hwnd && font) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

void showActionButtonsForPage(AppState& state) {
    struct Mapping {
        HWND hwnd;
        std::vector<GuiPage> pages;
    };
    const std::vector<Mapping> mappings = {
        {state.scoutActionButton, {GuiPage::Transfers, GuiPage::Dashboard, GuiPage::Youth}},
        {state.shortlistButton, {GuiPage::Transfers}},
        {state.followShortlistButton, {GuiPage::Transfers, GuiPage::News}},
        {state.buyButton, {GuiPage::Transfers}},
        {state.preContractButton, {GuiPage::Transfers}},
        {state.renewButton, {GuiPage::Squad, GuiPage::Finances}},
        {state.sellButton, {GuiPage::Squad, GuiPage::Transfers}},
        {state.planButton, {GuiPage::Squad, GuiPage::Youth}},
        {state.instructionButton, {GuiPage::Tactics, GuiPage::Dashboard}},
        {state.youthUpgradeButton, {GuiPage::Youth, GuiPage::Finances, GuiPage::Board}},
        {state.trainingUpgradeButton, {GuiPage::Tactics, GuiPage::Finances, GuiPage::Board}},
        {state.scoutingUpgradeButton, {GuiPage::Transfers, GuiPage::Finances, GuiPage::Board}},
        {state.stadiumUpgradeButton, {GuiPage::Finances, GuiPage::Board}}
    };

    bool hasCareer = state.career.myTeam != nullptr;
    for (const auto& mapping : mappings) {
        bool visible = hasCareer && std::find(mapping.pages.begin(), mapping.pages.end(), state.currentPage) != mapping.pages.end();
        ShowWindow(mapping.hwnd, visible ? SW_SHOW : SW_HIDE);
        EnableWindow(mapping.hwnd, visible);
    }
}

}  // namespace

void layoutWindow(AppState& state) {
    RECT client{};
    GetClientRect(state.window, &client);

    const int padding = 12;
    const int sideWidth = 164;
    const int topBarHeight = 116;
    const int contentLeft = padding + sideWidth + 16;
    const int contentTop = topBarHeight + 56;
    const int infoWidth = 318;
    const int contentWidth = std::max(560, static_cast<int>(client.right - contentLeft - infoWidth - padding * 3));
    const int infoLeft = contentLeft + contentWidth + padding;
    const bool dashboardLayout = state.currentPage == GuiPage::Dashboard;
    const bool dashboardEmptyState = dashboardLayout && !state.career.myTeam;
    const int summaryWidth = std::max(360, dashboardLayout ? contentWidth * 58 / 100 : contentWidth * 36 / 100);
    const int tableWidth = contentWidth - summaryWidth - padding;
    const int topPanelHeight = dashboardLayout ? 214 : 182;
    const int midPanelHeight = dashboardLayout ? 152 : 238;
    const int footerHeight = std::max(dashboardLayout ? 152 : 140,
                                      static_cast<int>(client.bottom - (contentTop + topPanelHeight + midPanelHeight + 136)));

    MoveWindow(state.divisionLabel, 18, 18, 70, 22, TRUE);
    MoveWindow(state.divisionCombo, 92, 14, 290, 400, TRUE);
    MoveWindow(state.teamLabel, 398, 18, 50, 22, TRUE);
    MoveWindow(state.teamCombo, 452, 14, 332, 400, TRUE);
    MoveWindow(state.managerLabel, 800, 18, 70, 22, TRUE);
    MoveWindow(state.managerEdit, 874, 14, 176, 28, TRUE);

    const int primaryButtonHeight = 30;
    MoveWindow(state.newCareerButton, client.right - 608, 13, 152, primaryButtonHeight, TRUE);
    MoveWindow(state.loadButton, client.right - 446, 13, 106, primaryButtonHeight, TRUE);
    MoveWindow(state.saveButton, client.right - 330, 13, 106, primaryButtonHeight, TRUE);
    MoveWindow(state.simulateButton, client.right - 214, 13, 118, primaryButtonHeight, TRUE);
    MoveWindow(state.validateButton, client.right - 96, 13, 88, primaryButtonHeight, TRUE);

    const int sideX = padding;
    int navY = topBarHeight + 8;
    std::array<HWND, 10> pages = {
        state.dashboardButton, state.squadButton, state.tacticsButton, state.calendarButton, state.leagueButton,
        state.transfersButton, state.financesButton, state.youthButton, state.boardButton, state.newsButton
    };
    for (HWND button : pages) {
        MoveWindow(button, sideX, navY, sideWidth, 38, TRUE);
        navY += 44;
    }

    MoveWindow(state.breadcrumbLabel, contentLeft, topBarHeight + 8, contentWidth, 20, TRUE);
    MoveWindow(state.pageTitleLabel, contentLeft, topBarHeight + 30, contentWidth, 30, TRUE);
    MoveWindow(state.infoLabel, contentLeft, topBarHeight + 64, contentWidth, 22, TRUE);
    MoveWindow(state.filterLabel, infoLeft + 6, topBarHeight + 32, 56, 18, TRUE);
    MoveWindow(state.filterCombo, infoLeft + 66, topBarHeight + 28, infoWidth - 66, 320, TRUE);

    showActionButtonsForPage(state);
    std::vector<ActionButtonRef> visibleButtons = {
        {state.scoutActionButton, 92}, {state.shortlistButton, 92}, {state.followShortlistButton, 98},
        {state.buyButton, 92}, {state.preContractButton, 102}, {state.renewButton, 92},
        {state.sellButton, 92}, {state.planButton, 92}, {state.instructionButton, 112},
        {state.youthUpgradeButton, 94}, {state.trainingUpgradeButton, 96},
        {state.scoutingUpgradeButton, 94}, {state.stadiumUpgradeButton, 96}
    };

    int actionX = contentLeft;
    int actionY = topBarHeight + 84;
    for (const auto& action : visibleButtons) {
        if (!action.hwnd || !IsWindowVisible(action.hwnd)) continue;
        if (actionX + action.width > contentLeft + contentWidth) {
            actionX = contentLeft;
            actionY += 34;
        }
        MoveWindow(action.hwnd, actionX, actionY, action.width, 26, TRUE);
        actionX += action.width + 8;
    }

    int panelsTop = contentTop + 30;

    if (dashboardEmptyState) {
        int summaryHeight = std::max(360, static_cast<int>(client.bottom) - panelsTop - 74);
        int sideHeight = std::max(160, (summaryHeight - 30) / 2);
        int buttonTop = panelsTop + summaryHeight - 58;
        int buttonWidth = 176;

        MoveWindow(state.summaryLabel, contentLeft, panelsTop, contentWidth, 18, TRUE);
        MoveWindow(state.summaryEdit, contentLeft, panelsTop + 18, contentWidth, summaryHeight, TRUE);

        MoveWindow(state.detailLabel, infoLeft, panelsTop, infoWidth, 18, TRUE);
        MoveWindow(state.detailEdit, infoLeft, panelsTop + 18, infoWidth, sideHeight, TRUE);

        int newsTop = panelsTop + sideHeight + 42;
        MoveWindow(state.newsLabel, infoLeft, newsTop, infoWidth, 18, TRUE);
        MoveWindow(state.newsList, infoLeft, newsTop + 18, infoWidth, summaryHeight - sideHeight - 24, TRUE);
        MoveWindow(state.emptyNewButton, contentLeft + 24, buttonTop, buttonWidth, 32, TRUE);
        MoveWindow(state.emptyLoadButton, contentLeft + 24 + buttonWidth + 12, buttonTop, buttonWidth, 32, TRUE);
        MoveWindow(state.emptyValidateButton, contentLeft + 24 + (buttonWidth + 12) * 2, buttonTop, buttonWidth, 32, TRUE);
        ShowWindow(state.emptyNewButton, SW_SHOW);
        ShowWindow(state.emptyLoadButton, SW_SHOW);
        ShowWindow(state.emptyValidateButton, SW_SHOW);

        MoveWindow(state.statusLabel, padding, client.bottom - 26, client.right - padding * 2, 18, TRUE);
        return;
    }

    ShowWindow(state.emptyNewButton, SW_HIDE);
    ShowWindow(state.emptyLoadButton, SW_HIDE);
    ShowWindow(state.emptyValidateButton, SW_HIDE);

    MoveWindow(state.summaryLabel, contentLeft, panelsTop, summaryWidth, 18, TRUE);
    MoveWindow(state.summaryEdit, contentLeft, panelsTop + 18, summaryWidth, topPanelHeight, TRUE);
    MoveWindow(state.tableLabel, contentLeft + summaryWidth + padding, panelsTop, tableWidth, 18, TRUE);
    MoveWindow(state.tableList, contentLeft + summaryWidth + padding, panelsTop + 18, tableWidth, topPanelHeight, TRUE);

    int secondTop = panelsTop + topPanelHeight + 42;
    MoveWindow(state.squadLabel, contentLeft, secondTop, contentWidth, 18, TRUE);
    MoveWindow(state.squadList, contentLeft, secondTop + 18, contentWidth, midPanelHeight, TRUE);

    int footerTop = secondTop + midPanelHeight + 42;
    MoveWindow(state.transferLabel, contentLeft, footerTop, contentWidth, 18, TRUE);
    MoveWindow(state.transferList, contentLeft, footerTop + 18, contentWidth, footerHeight, TRUE);

    if (dashboardLayout) {
        MoveWindow(state.detailLabel, infoLeft, secondTop, infoWidth, 18, TRUE);
        MoveWindow(state.detailEdit, infoLeft, secondTop + 18, infoWidth, 132, TRUE);
        MoveWindow(state.newsLabel, infoLeft, secondTop + 18 + 132 + 24, infoWidth, 18, TRUE);
        MoveWindow(state.newsList, infoLeft, secondTop + 18 + 132 + 42, infoWidth, client.bottom - (secondTop + 18 + 132 + 42) - 56, TRUE);
    } else {
        MoveWindow(state.detailLabel, infoLeft, panelsTop, infoWidth, 18, TRUE);
        MoveWindow(state.detailEdit, infoLeft, panelsTop + 18, infoWidth, topPanelHeight + 120, TRUE);
        int feedTop = panelsTop + topPanelHeight + 162;
        MoveWindow(state.newsLabel, infoLeft, feedTop, infoWidth, 18, TRUE);
        MoveWindow(state.newsList, infoLeft, feedTop + 18, infoWidth, client.bottom - feedTop - 56, TRUE);
    }

    MoveWindow(state.statusLabel, padding, client.bottom - 26, client.right - padding * 2, 18, TRUE);
}

void initializeInterface(AppState& state) {
    state.font = CreateFontW(-17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift");
    state.titleFont = CreateFontW(-28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift SemiBold");
    state.sectionFont = CreateFontW(-19, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift SemiBold");
    state.monoFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    state.backgroundBrush = CreateSolidBrush(kThemeBg);
    state.panelBrush = CreateSolidBrush(kThemePanel);
    state.headerBrush = CreateSolidBrush(kThemeHeader);
    state.inputBrush = CreateSolidBrush(kThemeInput);

    const DWORD buttonStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;

    state.divisionLabel = createControl(state, 0, L"STATIC", L"Division", WS_CHILD | WS_VISIBLE, 18, 18, 82, 20, state.window, 0);
    state.teamLabel = createControl(state, 0, L"STATIC", L"Club", WS_CHILD | WS_VISIBLE, 354, 18, 72, 20, state.window, 0);
    state.managerLabel = createControl(state, 0, L"STATIC", L"Manager", WS_CHILD | WS_VISIBLE, 702, 18, 82, 20, state.window, 0);
    state.filterLabel = createControl(state, 0, L"STATIC", L"Filtro", WS_CHILD | WS_VISIBLE, 0, 0, 50, 20, state.window, 0);

    state.divisionCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 92, 14, 184, 300, state.window, IDC_DIVISION_COMBO);
    state.teamCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 360, 14, 224, 300, state.window, IDC_TEAM_COMBO);
    state.managerEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"Manager", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 676, 14, 188, 24, state.window, IDC_MANAGER_EDIT);
    state.filterCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 180, 260, state.window, IDC_FILTER_COMBO);

    state.newCareerButton = createControl(state, 0, L"BUTTON", L"Nueva partida", buttonStyle, 0, 0, 100, 28, state.window, IDC_NEW_CAREER_BUTTON);
    state.loadButton = createControl(state, 0, L"BUTTON", L"Cargar", buttonStyle, 0, 0, 86, 28, state.window, IDC_LOAD_BUTTON);
    state.saveButton = createControl(state, 0, L"BUTTON", L"Guardar", buttonStyle, 0, 0, 86, 28, state.window, IDC_SAVE_BUTTON);
    state.simulateButton = createControl(state, 0, L"BUTTON", L"Simular", buttonStyle, 0, 0, 126, 28, state.window, IDC_SIMULATE_BUTTON);
    state.validateButton = createControl(state, 0, L"BUTTON", L"Validar", buttonStyle, 0, 0, 92, 28, state.window, IDC_VALIDATE_BUTTON);
    state.emptyNewButton = createControl(state, 0, L"BUTTON", L"Nueva partida", buttonStyle, 0, 0, 140, 30, state.window, IDC_EMPTY_NEW_BUTTON);
    state.emptyLoadButton = createControl(state, 0, L"BUTTON", L"Cargar guardado", buttonStyle, 0, 0, 140, 30, state.window, IDC_EMPTY_LOAD_BUTTON);
    state.emptyValidateButton = createControl(state, 0, L"BUTTON", L"Validar datos", buttonStyle, 0, 0, 140, 30, state.window, IDC_EMPTY_VALIDATE_BUTTON);
    ShowWindow(state.emptyNewButton, SW_HIDE);
    ShowWindow(state.emptyLoadButton, SW_HIDE);
    ShowWindow(state.emptyValidateButton, SW_HIDE);

    state.dashboardButton = createControl(state, 0, L"BUTTON", L"Inicio", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_DASHBOARD_BUTTON);
    state.squadButton = createControl(state, 0, L"BUTTON", L"Plantilla", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_SQUAD_BUTTON);
    state.tacticsButton = createControl(state, 0, L"BUTTON", L"Tacticas", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_TACTICS_BUTTON);
    state.calendarButton = createControl(state, 0, L"BUTTON", L"Calendario", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_CALENDAR_BUTTON);
    state.leagueButton = createControl(state, 0, L"BUTTON", L"Liga", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_LEAGUE_BUTTON);
    state.transfersButton = createControl(state, 0, L"BUTTON", L"Fichajes", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_TRANSFERS_BUTTON);
    state.financesButton = createControl(state, 0, L"BUTTON", L"Finanzas", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_FINANCES_BUTTON);
    state.youthButton = createControl(state, 0, L"BUTTON", L"Cantera", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_YOUTH_BUTTON);
    state.boardButton = createControl(state, 0, L"BUTTON", L"Directiva", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_BOARD_BUTTON);
    state.newsButton = createControl(state, 0, L"BUTTON", L"Noticias", buttonStyle, 0, 0, 132, 34, state.window, IDC_PAGE_NEWS_BUTTON);

    state.scoutActionButton = createControl(state, 0, L"BUTTON", L"Otear", buttonStyle, 0, 0, 92, 26, state.window, IDC_SCOUT_BUTTON);
    state.shortlistButton = createControl(state, 0, L"BUTTON", L"Shortlist", buttonStyle, 0, 0, 92, 26, state.window, IDC_SHORTLIST_BUTTON);
    state.followShortlistButton = createControl(state, 0, L"BUTTON", L"Actualizar", buttonStyle, 0, 0, 98, 26, state.window, IDC_FOLLOW_SHORTLIST_BUTTON);
    state.buyButton = createControl(state, 0, L"BUTTON", L"Fichar", buttonStyle, 0, 0, 92, 26, state.window, IDC_BUY_BUTTON);
    state.preContractButton = createControl(state, 0, L"BUTTON", L"Precontrato", buttonStyle, 0, 0, 102, 26, state.window, IDC_PRECONTRACT_BUTTON);
    state.renewButton = createControl(state, 0, L"BUTTON", L"Renovar", buttonStyle, 0, 0, 92, 26, state.window, IDC_RENEW_BUTTON);
    state.sellButton = createControl(state, 0, L"BUTTON", L"Vender", buttonStyle, 0, 0, 92, 26, state.window, IDC_SELL_BUTTON);
    state.planButton = createControl(state, 0, L"BUTTON", L"Plan", buttonStyle, 0, 0, 92, 26, state.window, IDC_PLAN_BUTTON);
    state.instructionButton = createControl(state, 0, L"BUTTON", L"Instruccion", buttonStyle, 0, 0, 112, 26, state.window, IDC_INSTRUCTION_BUTTON);
    state.youthUpgradeButton = createControl(state, 0, L"BUTTON", L"Cantera+", buttonStyle, 0, 0, 94, 26, state.window, IDC_YOUTH_UPGRADE_BUTTON);
    state.trainingUpgradeButton = createControl(state, 0, L"BUTTON", L"Entreno+", buttonStyle, 0, 0, 96, 26, state.window, IDC_TRAINING_UPGRADE_BUTTON);
    state.scoutingUpgradeButton = createControl(state, 0, L"BUTTON", L"Scout+", buttonStyle, 0, 0, 94, 26, state.window, IDC_SCOUTING_UPGRADE_BUTTON);
    state.stadiumUpgradeButton = createControl(state, 0, L"BUTTON", L"Estadio+", buttonStyle, 0, 0, 96, 26, state.window, IDC_STADIUM_UPGRADE_BUTTON);

    state.breadcrumbLabel = createControl(state, 0, L"STATIC", L"Club > Resumen del club", WS_CHILD | WS_VISIBLE, 0, 0, 320, 18, state.window, 0);
    state.pageTitleLabel = createControl(state, 0, L"STATIC", L"Resumen del club", WS_CHILD | WS_VISIBLE, 0, 0, 360, 24, state.window, 0);
    state.infoLabel = createControl(state, 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 400, 22, state.window, 0);
    state.summaryLabel = createControl(state, 0, L"STATIC", L"Proximo partido", WS_CHILD | WS_VISIBLE, 0, 0, 240, 18, state.window, 0);
    state.summaryEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 0, 0, 300, 180, state.window, IDC_SUMMARY_EDIT);
    state.tableLabel = createControl(state, 0, L"STATIC", L"Tabla de liga", WS_CHILD | WS_VISIBLE, 0, 0, 240, 18, state.window, 0);
    state.tableList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 300, 180, state.window, IDC_TABLE_LIST);
    state.squadLabel = createControl(state, 0, L"STATIC", L"Estado del equipo", WS_CHILD | WS_VISIBLE, 0, 0, 240, 18, state.window, 0);
    state.squadList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 300, 180, state.window, IDC_SQUAD_LIST);
    state.transferLabel = createControl(state, 0, L"STATIC", L"Lesiones", WS_CHILD | WS_VISIBLE, 0, 0, 260, 18, state.window, 0);
    state.transferList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 0, 0, 300, 180, state.window, IDC_TRANSFER_LIST);
    state.detailLabel = createControl(state, 0, L"STATIC", L"Ultimo resultado", WS_CHILD | WS_VISIBLE, 0, 0, 220, 18, state.window, 0);
    state.detailEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 0, 0, 280, 240, state.window, IDC_DETAIL_EDIT);
    state.newsLabel = createControl(state, 0, L"STATIC", L"Noticias", WS_CHILD | WS_VISIBLE, 0, 0, 240, 18, state.window, 0);
    state.newsList = createControl(state, WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, 0, 280, 220, state.window, IDC_NEWS_LIST);
    state.statusLabel = createControl(state, 0, L"STATIC", L"Interfaz lista.", WS_CHILD | WS_VISIBLE, 0, 0, 420, 18, state.window, 0);

    setLabelFont(state.pageTitleLabel, state.titleFont);
    setLabelFont(state.summaryLabel, state.sectionFont);
    setLabelFont(state.tableLabel, state.sectionFont);
    setLabelFont(state.squadLabel, state.sectionFont);
    setLabelFont(state.transferLabel, state.sectionFont);
    setLabelFont(state.detailLabel, state.sectionFont);
    setLabelFont(state.newsLabel, state.sectionFont);
    setLabelFont(state.breadcrumbLabel, state.font);
    setLabelFont(state.infoLabel, state.font);
    setLabelFont(state.statusLabel, state.font);
    setLabelFont(state.filterLabel, state.font);
    setLabelFont(state.divisionLabel, state.font);
    setLabelFont(state.teamLabel, state.font);
    setLabelFont(state.managerLabel, state.font);

    if (state.font) {
        SendMessageW(state.summaryEdit, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
        SendMessageW(state.detailEdit, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
        SendMessageW(state.newsList, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    }

    DWORD styles = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER;
    ListView_SetExtendedListViewStyle(state.tableList, styles);
    ListView_SetExtendedListViewStyle(state.squadList, styles);
    ListView_SetExtendedListViewStyle(state.transferList, styles);
    ListView_SetBkColor(state.tableList, kThemeInput);
    ListView_SetBkColor(state.squadList, kThemeInput);
    ListView_SetBkColor(state.transferList, kThemeInput);
    ListView_SetTextBkColor(state.tableList, kThemeInput);
    ListView_SetTextBkColor(state.squadList, kThemeInput);
    ListView_SetTextBkColor(state.transferList, kThemeInput);
    ListView_SetTextColor(state.tableList, kThemeText);
    ListView_SetTextColor(state.squadList, kThemeText);
    ListView_SetTextColor(state.transferList, kThemeText);
    SendMessageW(state.newsList, LB_SETITEMHEIGHT, 0, 22);

    state.career.initializeLeague();
    fillDivisionCombo(state);
    fillTeamCombo(state, selectedDivisionId(state));
    setCurrentPage(state, GuiPage::Dashboard);
    layoutWindow(state);
    refreshAll(state);
    setStatus(state, "GUI lista con layout modular.");
}

void paintWindowChrome(AppState& state, HDC hdc) {
    RECT client{};
    GetClientRect(state.window, &client);
    FillRect(hdc, &client, state.backgroundBrush ? state.backgroundBrush : static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    RECT topBar{0, 0, client.right, 108};
    FillRect(hdc, &topBar, state.headerBrush ? state.headerBrush : state.backgroundBrush);
    drawRoundedPanel(hdc, RECT{8, 8, client.right - 8, 104}, RGB(10, 23, 30), RGB(28, 53, 65), 18);
    drawTopMetrics(state, hdc, client);

    RECT sideMenu{8, 116, 170, client.bottom - 34};
    drawRoundedPanel(hdc, sideMenu, RGB(12, 23, 31), RGB(34, 57, 70), 18);

    RECT contentShell{182, 116, client.right - 12, client.bottom - 34};
    drawRoundedPanel(hdc, contentShell, RGB(10, 21, 29), RGB(32, 53, 66), 22);

    RECT summaryCard = expandedRect(childRectOnParent(state.summaryEdit, state.window), 8, 24);
    RECT tableCard = expandedRect(childRectOnParent(state.tableList, state.window), 8, 24);
    RECT squadCard = expandedRect(childRectOnParent(state.squadList, state.window), 8, 24);
    RECT transferCard = expandedRect(childRectOnParent(state.transferList, state.window), 8, 24);
    RECT detailCard = expandedRect(childRectOnParent(state.detailEdit, state.window), 8, 24);
    RECT newsCard = expandedRect(childRectOnParent(state.newsList, state.window), 8, 24);
    RECT statusCard = expandedRect(childRectOnParent(state.statusLabel, state.window), 6, 8);
    if (IsWindowVisible(state.summaryEdit)) drawRoundedPanel(hdc, summaryCard, kThemePanel, RGB(40, 64, 79), 16);
    if (IsWindowVisible(state.tableList)) drawRoundedPanel(hdc, tableCard, kThemePanel, RGB(40, 64, 79), 16);
    if (IsWindowVisible(state.squadList)) drawRoundedPanel(hdc, squadCard, kThemePanel, RGB(40, 64, 79), 16);
    if (IsWindowVisible(state.transferList)) drawRoundedPanel(hdc, transferCard, kThemePanel, RGB(40, 64, 79), 16);
    if (IsWindowVisible(state.detailEdit)) drawRoundedPanel(hdc, detailCard, RGB(15, 27, 37), RGB(44, 72, 90), 16);
    if (IsWindowVisible(state.newsList)) drawRoundedPanel(hdc, newsCard, RGB(15, 27, 37), RGB(44, 72, 90), 16);
    drawRoundedPanel(hdc, statusCard, RGB(11, 23, 31), RGB(39, 65, 79), 12);

    RECT menuTitle{20, 124, 158, 148};
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kThemeMuted);
    HGDIOBJ oldFont = SelectObject(hdc, state.sectionFont ? state.sectionFont : state.font);
    DrawTextW(hdc, L"Secciones", -1, &menuTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    if (state.currentPage == GuiPage::Tactics) {
        RECT boardRect = expandedRect(childRectOnParent(state.tableList, state.window), 8, 24);
        drawTacticsBoard(state, hdc, boardRect);
    }
}

}  // namespace gui_win32

#endif
