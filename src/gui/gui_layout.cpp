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

constexpr int kWindowPadding = 12;
constexpr int kSideRailWideWidth = 182;
constexpr int kSideRailCompactWidth = 168;
constexpr int kContentGap = 16;
constexpr int kMetricGap = 14;
constexpr int kMetricHeight = 58;
constexpr int kPanelLabelHeight = 22;
constexpr int kPanelBodyOffset = 24;
constexpr int kPanelSectionGap = 46;
constexpr int kStatusHeight = 28;
constexpr int kHeaderFieldHeight = 32;
constexpr int kHeaderButtonHeight = 34;
constexpr int kHeaderRowGap = 8;
constexpr int kMetricCount = 5;

struct HeaderLayoutProfile {
    int sideWidth = kSideRailWideWidth;
    int topBarHeight = 138;
    int controlsBottom = 48;
    int buttonsTop = 16;
    int buttonsBottom = 50;
    int metricTop = 66;
    int metricColumns = kMetricCount;
    int metricRows = 1;
    bool shareTopRow = true;
    bool splitControls = false;
    bool stackedControls = false;
};

struct PageLayoutProfile {
    int preferredInfoWidth = 340;
    int minInfoWidth = 300;
    int summaryPercent = 40;
    int summaryMinWidth = 320;
    int topPanelHeight = 196;
    int midPanelHeight = 226;
    int footerMinHeight = 150;
    int dashboardDetailHeight = 148;
    int nonDashboardDetailExtra = 132;
};

HeaderLayoutProfile buildHeaderLayout(const RECT& client) {
    HeaderLayoutProfile layout;
    const int width = client.right - client.left;
    layout.sideWidth = width < 1240 ? kSideRailCompactWidth : kSideRailWideWidth;
    layout.shareTopRow = width >= 1780;
    layout.splitControls = width < 1360;
    layout.stackedControls = width < 1100;

    if (layout.stackedControls) {
        layout.controlsBottom = 16 + kHeaderFieldHeight * 3 + kHeaderRowGap * 2;
    } else if (layout.splitControls) {
        layout.controlsBottom = 16 + kHeaderFieldHeight * 2 + kHeaderRowGap;
    } else {
        layout.controlsBottom = 16 + kHeaderFieldHeight;
    }

    layout.buttonsTop = layout.shareTopRow ? 16 : layout.controlsBottom + 10;
    const int availableButtonRow = std::max(320, width - kWindowPadding * 2);
    const int fullButtonRowWidth = 156 + 112 + 112 + 126 + 94 + 10 * 4;
    const int buttonRows = availableButtonRow >= fullButtonRowWidth ? 1 : 2;
    layout.buttonsBottom = layout.buttonsTop + kHeaderButtonHeight + (buttonRows - 1) * (kHeaderButtonHeight + kHeaderRowGap);

    if (width < 980) layout.metricColumns = 2;
    else if (width < 1280) layout.metricColumns = 3;
    else layout.metricColumns = kMetricCount;
    layout.metricRows = (kMetricCount + layout.metricColumns - 1) / layout.metricColumns;
    layout.metricTop = std::max(layout.controlsBottom, layout.buttonsBottom) + 16;
    layout.topBarHeight = layout.metricTop + layout.metricRows * kMetricHeight + (layout.metricRows - 1) * kMetricGap + 14;
    return layout;
}

PageLayoutProfile buildPageLayoutProfile(GuiPage page) {
    switch (page) {
        case GuiPage::Dashboard:
            return {340, 300, 56, 348, 226, 166, 160, 148, 132};
        case GuiPage::Squad:
        case GuiPage::Youth:
            return {364, 320, 38, 312, 192, 258, 152, 148, 144};
        case GuiPage::Tactics:
            return {360, 320, 40, 336, 208, 240, 154, 152, 144};
        case GuiPage::Calendar:
            return {350, 310, 40, 320, 194, 220, 150, 148, 136};
        case GuiPage::League:
            return {350, 310, 42, 320, 194, 216, 150, 148, 136};
        case GuiPage::Transfers:
            return {392, 340, 35, 300, 202, 252, 160, 156, 150};
        case GuiPage::Finances:
            return {376, 330, 36, 308, 198, 238, 160, 152, 148};
        case GuiPage::Board:
            return {360, 320, 38, 312, 206, 236, 160, 152, 146};
        case GuiPage::News:
            return {404, 350, 32, 292, 194, 228, 166, 154, 150};
    }
    return PageLayoutProfile{};
}

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
    const HeaderLayoutProfile header = buildHeaderLayout(client);
    const auto s = [&](int value) { return scaleByDpi(state, value); };
    std::vector<DashboardMetric> metrics = state.currentModel.metrics.empty() ? defaultMetrics() : state.currentModel.metrics;
    const int left = s(kWindowPadding + 6);
    const int gap = s(kMetricGap);
    const int width = std::max(s(header.metricColumns <= 2 ? 220 : 158),
                               static_cast<int>((client.right - left * 2 - gap * (header.metricColumns - 1)) / header.metricColumns));
    const int height = s(kMetricHeight);

    for (size_t i = 0; i < metrics.size() && i < 5; ++i) {
        const int row = static_cast<int>(i) / header.metricColumns;
        const int column = static_cast<int>(i) % header.metricColumns;
        const int top = s(header.metricTop) + row * (height + gap);
        RECT card{left + column * (width + gap), top, left + column * (width + gap) + width, top + height};
        drawRoundedPanel(hdc, card, RGB(11, 24, 33), RGB(35, 58, 74), s(14));

        RECT accent = card;
        accent.bottom = accent.top + s(6);
        HBRUSH accentBrush = CreateSolidBrush(metrics[i].accent);
        FillRect(hdc, &accent, accentBrush);
        DeleteObject(accentBrush);

        RECT labelRect{card.left + s(14), card.top + s(10), card.right - s(14), card.top + s(28)};
        RECT valueRect{card.left + s(14), card.top + s(28), card.right - s(14), card.bottom - s(10)};
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

void setControlFont(HWND hwnd, HFONT font) {
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
        {state.scoutActionButton, {GuiPage::Transfers, GuiPage::Dashboard, GuiPage::Youth, GuiPage::News}},
        {state.shortlistButton, {GuiPage::Transfers}},
        {state.followShortlistButton, {GuiPage::Transfers, GuiPage::News}},
        {state.buyButton, {GuiPage::Transfers}},
        {state.preContractButton, {GuiPage::Transfers}},
        {state.renewButton, {GuiPage::Squad, GuiPage::Finances}},
        {state.sellButton, {GuiPage::Squad, GuiPage::Transfers}},
        {state.planButton, {GuiPage::Squad, GuiPage::Youth}},
        {state.instructionButton, {GuiPage::Tactics, GuiPage::Dashboard, GuiPage::Squad, GuiPage::Youth, GuiPage::Board, GuiPage::News}},
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

void rebuildFonts(AppState& state) {
    if (state.font) DeleteObject(state.font);
    if (state.titleFont) DeleteObject(state.titleFont);
    if (state.sectionFont) DeleteObject(state.sectionFont);
    if (state.monoFont) DeleteObject(state.monoFont);

    state.font = CreateFontW(-scaleByDpi(state, 15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift");
    state.titleFont = CreateFontW(-scaleByDpi(state, 24), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift SemiBold");
    state.sectionFont = CreateFontW(-scaleByDpi(state, 17), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift SemiBold");
    state.monoFont = CreateFontW(-scaleByDpi(state, 15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
}

void applyInterfaceFonts(AppState& state) {
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

    const std::array<HWND, 5> inputs = {
        state.divisionCombo, state.teamCombo, state.managerEdit, state.filterCombo, state.newsList
    };
    for (HWND hwnd : inputs) setControlFont(hwnd, state.font);

    const std::array<HWND, 2> textPanels = {state.summaryEdit, state.detailEdit};
    for (HWND hwnd : textPanels) setControlFont(hwnd, state.font);

    const std::array<HWND, 3> tablePanels = {state.tableList, state.squadList, state.transferList};
    for (HWND hwnd : tablePanels) setControlFont(hwnd, state.font);

    const std::array<HWND, 18> buttons = {
        state.newCareerButton, state.loadButton, state.saveButton, state.simulateButton, state.validateButton,
        state.dashboardButton, state.squadButton, state.tacticsButton, state.calendarButton, state.leagueButton,
        state.transfersButton, state.financesButton, state.youthButton, state.boardButton, state.newsButton,
        state.emptyNewButton, state.emptyLoadButton, state.emptyValidateButton
    };
    for (HWND hwnd : buttons) setControlFont(hwnd, state.font);

    const std::array<HWND, 13> actionButtons = {
        state.scoutActionButton, state.shortlistButton, state.followShortlistButton, state.buyButton,
        state.preContractButton, state.renewButton, state.sellButton, state.planButton, state.instructionButton,
        state.youthUpgradeButton, state.trainingUpgradeButton, state.scoutingUpgradeButton, state.stadiumUpgradeButton
    };
    for (HWND hwnd : actionButtons) setControlFont(hwnd, state.font);

    const int comboFieldHeight = scaleByDpi(state, 24);
    const int comboItemHeight = scaleByDpi(state, 28);
    const std::array<HWND, 3> combos = {state.divisionCombo, state.teamCombo, state.filterCombo};
    for (HWND combo : combos) {
        if (!combo) continue;
        SendMessageW(combo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), comboFieldHeight);
        SendMessageW(combo, CB_SETITEMHEIGHT, 0, comboItemHeight);
    }

    if (state.newsList) {
        SendMessageW(state.newsList, LB_SETITEMHEIGHT, 0, scaleByDpi(state, 24));
    }
}

void layoutWindow(AppState& state) {
    RECT client{};
    GetClientRect(state.window, &client);

    const auto s = [&](int value) { return scaleByDpi(state, value); };
    const HeaderLayoutProfile header = buildHeaderLayout(client);
    const PageLayoutProfile pageLayout = buildPageLayoutProfile(state.currentPage);
    const int padding = s(kWindowPadding);
    const int sideWidth = s(header.sideWidth);
    const int topBarHeight = s(header.topBarHeight);
    const int contentLeft = padding + sideWidth + s(kContentGap);
    const int contentTop = topBarHeight + s(68);
    const int availableMainWidth = std::max(760, static_cast<int>(client.right - contentLeft - padding * 2));
    int infoWidth = clampValue(s(pageLayout.preferredInfoWidth), s(260), std::max(s(260), availableMainWidth / 2 - s(24)));
    int contentWidth = availableMainWidth - infoWidth - padding;
    if (contentWidth < s(460)) {
        infoWidth = std::max(s(260), infoWidth - (s(460) - contentWidth));
        contentWidth = availableMainWidth - infoWidth - padding;
    }
    const int infoLeft = contentLeft + contentWidth + padding;
    const bool dashboardLayout = state.currentPage == GuiPage::Dashboard;
    const bool dashboardEmptyState = dashboardLayout && !state.career.myTeam;
    const int summaryWidth = clampValue(contentWidth * pageLayout.summaryPercent / 100,
                                        std::min(s(280), contentWidth - padding - s(220)),
                                        std::max(s(280), contentWidth - padding - s(220)));
    const int tableWidth = contentWidth - summaryWidth - padding;
    const int topPanelHeight = s(pageLayout.topPanelHeight);
    const int midPanelHeight = s(pageLayout.midPanelHeight);

    const int primaryButtonHeight = s(kHeaderButtonHeight);
    const int buttonGap = s(10);
    int buttonRight = client.right - padding;
    int buttonTop = s(header.buttonsTop);
    int topRowLeftmostButton = client.right - padding;
    auto placeHeaderButton = [&](HWND hwnd, int width) {
        if (!header.shareTopRow && buttonRight - width < padding) {
            buttonRight = client.right - padding;
            buttonTop += primaryButtonHeight + s(kHeaderRowGap);
        }
        buttonRight -= width;
        MoveWindow(hwnd, buttonRight, buttonTop, width, primaryButtonHeight, TRUE);
        if (buttonTop == s(header.buttonsTop)) topRowLeftmostButton = std::min(topRowLeftmostButton, buttonRight);
        buttonRight -= buttonGap;
    };
    placeHeaderButton(state.validateButton, s(94));
    placeHeaderButton(state.simulateButton, s(126));
    placeHeaderButton(state.saveButton, s(112));
    placeHeaderButton(state.loadButton, s(112));
    placeHeaderButton(state.newCareerButton, s(156));
    const int headerRightLimit = header.shareTopRow ? topRowLeftmostButton - s(12) : client.right - padding;

    const int fieldTop = s(16);
    const int fieldHeight = s(kHeaderFieldHeight);
    const int blockGap = s(20);
    const int rowTwoTop = fieldTop + fieldHeight + s(kHeaderRowGap);
    const int rowThreeTop = rowTwoTop + fieldHeight + s(kHeaderRowGap);
    const int headerAvailableWidth = std::max(s(520), headerRightLimit - s(20));

    auto placeFieldBlock = [&](HWND label, int labelWidth, HWND field, int inputOffset, int x, int y, int width, int controlHeight) {
        int fieldWidth = std::max(s(150), width - inputOffset);
        MoveWindow(label, x, y + s(4), labelWidth, s(22), TRUE);
        MoveWindow(field, x + inputOffset, y, fieldWidth, controlHeight, TRUE);
    };

    if (header.stackedControls) {
        placeFieldBlock(state.divisionLabel, s(78), state.divisionCombo, s(90), s(20), fieldTop, headerAvailableWidth, s(420));
        placeFieldBlock(state.teamLabel, s(46), state.teamCombo, s(56), s(20), rowTwoTop, headerAvailableWidth, s(420));
        placeFieldBlock(state.managerLabel, s(78), state.managerEdit, s(86), s(20), rowThreeTop, std::min(s(430), headerAvailableWidth), fieldHeight);
    } else if (header.splitControls) {
        int rowOneWidth = headerAvailableWidth - blockGap;
        int divisionBlockWidth = std::max(s(250), rowOneWidth * 40 / 100);
        int teamBlockWidth = rowOneWidth - divisionBlockWidth;
        placeFieldBlock(state.divisionLabel, s(78), state.divisionCombo, s(90), s(20), fieldTop, divisionBlockWidth, s(420));
        placeFieldBlock(state.teamLabel, s(46), state.teamCombo, s(56), s(20) + divisionBlockWidth + blockGap, fieldTop, teamBlockWidth, s(420));
        placeFieldBlock(state.managerLabel, s(78), state.managerEdit, s(86), s(20), rowTwoTop, std::min(s(440), headerAvailableWidth), fieldHeight);
    } else {
        int rowOneWidth = headerAvailableWidth - blockGap * 2;
        int divisionBlockWidth = clampValue(rowOneWidth * 28 / 100, s(300), s(360));
        int managerBlockWidth = clampValue(rowOneWidth * 24 / 100, s(240), s(300));
        int teamBlockWidth = rowOneWidth - divisionBlockWidth - managerBlockWidth;
        int divisionX = s(20);
        int teamX = divisionX + divisionBlockWidth + blockGap;
        int managerX = teamX + teamBlockWidth + blockGap;
        placeFieldBlock(state.divisionLabel, s(78), state.divisionCombo, s(90), divisionX, fieldTop, divisionBlockWidth, s(420));
        placeFieldBlock(state.teamLabel, s(46), state.teamCombo, s(56), teamX, fieldTop, teamBlockWidth, s(420));
        placeFieldBlock(state.managerLabel, s(78), state.managerEdit, s(86), managerX, fieldTop, managerBlockWidth, fieldHeight);
    }

    const int sideX = padding;
    int navY = topBarHeight + s(12);
    std::array<HWND, 10> pages = {
        state.dashboardButton, state.squadButton, state.tacticsButton, state.calendarButton, state.leagueButton,
        state.transfersButton, state.financesButton, state.youthButton, state.boardButton, state.newsButton
    };
    for (HWND button : pages) {
        MoveWindow(button, sideX, navY, sideWidth, s(40), TRUE);
        navY += s(48);
    }

    MoveWindow(state.breadcrumbLabel, contentLeft, topBarHeight + s(12), contentWidth, s(20), TRUE);
    MoveWindow(state.pageTitleLabel, contentLeft, topBarHeight + s(38), contentWidth, s(32), TRUE);
    MoveWindow(state.infoLabel, contentLeft, topBarHeight + s(74), contentWidth, s(24), TRUE);
    MoveWindow(state.filterLabel, infoLeft + s(6), topBarHeight + s(40), s(56), s(20), TRUE);
    MoveWindow(state.filterCombo, infoLeft + s(68), topBarHeight + s(36), infoWidth - s(68), s(320), TRUE);

    showActionButtonsForPage(state);
    std::vector<ActionButtonRef> visibleButtons = {
        {state.scoutActionButton, 92}, {state.shortlistButton, 92}, {state.followShortlistButton, 98},
        {state.buyButton, 92}, {state.preContractButton, 102}, {state.renewButton, 92},
        {state.sellButton, 92}, {state.planButton, 92}, {state.instructionButton, 112},
        {state.youthUpgradeButton, 94}, {state.trainingUpgradeButton, 96},
        {state.scoutingUpgradeButton, 94}, {state.stadiumUpgradeButton, 96}
    };

    int actionX = contentLeft;
    int actionY = topBarHeight + s(106);
    bool hasVisibleActionButtons = false;
    int lastActionBottom = 0;
    for (const auto& action : visibleButtons) {
        if (!action.hwnd || !IsWindowVisible(action.hwnd)) continue;
        hasVisibleActionButtons = true;
        if (actionX + action.width > contentLeft + contentWidth) {
            actionX = contentLeft;
            actionY += s(36);
        }
        MoveWindow(action.hwnd, actionX, actionY, action.width, s(28), TRUE);
        actionX += action.width + s(10);
        lastActionBottom = actionY + s(28);
    }

    int panelsTop = hasVisibleActionButtons ? lastActionBottom + s(18) : contentTop + s(28);
    const int footerHeight = std::max(s(pageLayout.footerMinHeight),
                                      static_cast<int>(client.bottom - (panelsTop + topPanelHeight + midPanelHeight + s(150))));

    if (dashboardEmptyState) {
        int summaryHeight = std::max(s(380), static_cast<int>(client.bottom) - panelsTop - s(82));
        int sideHeight = std::max(s(170), (summaryHeight - s(34)) / 2);
        const int buttonWidth = clampValue((contentWidth - s(72)) / 3, s(140), s(184));
        const int emptyButtonTop = panelsTop + summaryHeight - s(62);

        MoveWindow(state.summaryLabel, contentLeft, panelsTop, contentWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.summaryEdit, contentLeft, panelsTop + s(kPanelBodyOffset), contentWidth, summaryHeight, TRUE);

        MoveWindow(state.detailLabel, infoLeft, panelsTop, infoWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.detailEdit, infoLeft, panelsTop + s(kPanelBodyOffset), infoWidth, sideHeight, TRUE);

        int newsTop = panelsTop + sideHeight + s(kPanelSectionGap);
        MoveWindow(state.newsLabel, infoLeft, newsTop, infoWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.newsList, infoLeft, newsTop + s(kPanelBodyOffset), infoWidth, summaryHeight - sideHeight - s(28), TRUE);
        MoveWindow(state.emptyNewButton, contentLeft + s(20), emptyButtonTop, buttonWidth, s(34), TRUE);
        MoveWindow(state.emptyLoadButton, contentLeft + s(28) + buttonWidth, emptyButtonTop, buttonWidth, s(34), TRUE);
        MoveWindow(state.emptyValidateButton, contentLeft + s(36) + buttonWidth * 2, emptyButtonTop, buttonWidth, s(34), TRUE);
        ShowWindow(state.emptyNewButton, SW_SHOW);
        ShowWindow(state.emptyLoadButton, SW_SHOW);
        ShowWindow(state.emptyValidateButton, SW_SHOW);

        MoveWindow(state.statusLabel, padding, client.bottom - s(kStatusHeight), client.right - padding * 2, s(20), TRUE);
        return;
    }

    ShowWindow(state.emptyNewButton, SW_HIDE);
    ShowWindow(state.emptyLoadButton, SW_HIDE);
    ShowWindow(state.emptyValidateButton, SW_HIDE);

    MoveWindow(state.summaryLabel, contentLeft, panelsTop, summaryWidth, s(kPanelLabelHeight), TRUE);
    MoveWindow(state.summaryEdit, contentLeft, panelsTop + s(kPanelBodyOffset), summaryWidth, topPanelHeight, TRUE);
    MoveWindow(state.tableLabel, contentLeft + summaryWidth + padding, panelsTop, tableWidth, s(kPanelLabelHeight), TRUE);
    MoveWindow(state.tableList, contentLeft + summaryWidth + padding, panelsTop + s(kPanelBodyOffset), tableWidth, topPanelHeight, TRUE);

    int secondTop = panelsTop + topPanelHeight + s(kPanelSectionGap);
    MoveWindow(state.squadLabel, contentLeft, secondTop, contentWidth, s(kPanelLabelHeight), TRUE);
    MoveWindow(state.squadList, contentLeft, secondTop + s(kPanelBodyOffset), contentWidth, midPanelHeight, TRUE);

    int footerTop = secondTop + midPanelHeight + s(kPanelSectionGap);
    MoveWindow(state.transferLabel, contentLeft, footerTop, contentWidth, s(kPanelLabelHeight), TRUE);
    MoveWindow(state.transferList, contentLeft, footerTop + s(kPanelBodyOffset), contentWidth, footerHeight, TRUE);

    if (dashboardLayout) {
        MoveWindow(state.detailLabel, infoLeft, secondTop, infoWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.detailEdit, infoLeft, secondTop + s(kPanelBodyOffset), infoWidth, s(pageLayout.dashboardDetailHeight), TRUE);
        int newsTop = secondTop + s(kPanelBodyOffset) + s(pageLayout.dashboardDetailHeight) + s(26);
        MoveWindow(state.newsLabel, infoLeft, newsTop, infoWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.newsList, infoLeft, newsTop + s(kPanelBodyOffset), infoWidth, client.bottom - (newsTop + s(kPanelBodyOffset)) - s(58), TRUE);
    } else {
        MoveWindow(state.detailLabel, infoLeft, panelsTop, infoWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.detailEdit, infoLeft, panelsTop + s(kPanelBodyOffset), infoWidth, topPanelHeight + s(pageLayout.nonDashboardDetailExtra), TRUE);
        int feedTop = panelsTop + topPanelHeight + s(pageLayout.nonDashboardDetailExtra) + s(42);
        MoveWindow(state.newsLabel, infoLeft, feedTop, infoWidth, s(kPanelLabelHeight), TRUE);
        MoveWindow(state.newsList, infoLeft, feedTop + s(kPanelBodyOffset), infoWidth, client.bottom - feedTop - s(60), TRUE);
    }

    MoveWindow(state.statusLabel, padding, client.bottom - s(kStatusHeight), client.right - padding * 2, s(20), TRUE);
}

void initializeInterface(AppState& state) {
    rebuildFonts(state);
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
    state.emptyNewButton = createControl(state, 0, L"BUTTON", L"Crear carrera", buttonStyle, 0, 0, 140, 30, state.window, IDC_EMPTY_NEW_BUTTON);
    state.emptyLoadButton = createControl(state, 0, L"BUTTON", L"Abrir guardado", buttonStyle, 0, 0, 140, 30, state.window, IDC_EMPTY_LOAD_BUTTON);
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

    applyInterfaceFonts(state);

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
    const auto s = [&](int value) { return scaleByDpi(state, value); };
    const HeaderLayoutProfile header = buildHeaderLayout(client);
    FillRect(hdc, &client, state.backgroundBrush ? state.backgroundBrush : static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    RECT topBar{0, 0, client.right, s(header.topBarHeight) - s(10)};
    FillRect(hdc, &topBar, state.headerBrush ? state.headerBrush : state.backgroundBrush);
    drawRoundedPanel(hdc, RECT{s(8), s(8), client.right - s(8), s(header.topBarHeight) - s(12)}, RGB(10, 23, 30), RGB(28, 53, 65), s(18));
    drawTopMetrics(state, hdc, client);

    RECT sideMenu{s(8), s(header.topBarHeight), s(8) + s(header.sideWidth), client.bottom - s(34)};
    drawRoundedPanel(hdc, sideMenu, RGB(12, 23, 31), RGB(34, 57, 70), s(18));

    RECT contentShell{s(kWindowPadding + header.sideWidth + 6), s(header.topBarHeight), client.right - s(kWindowPadding), client.bottom - s(34)};
    drawRoundedPanel(hdc, contentShell, RGB(10, 21, 29), RGB(32, 53, 66), s(22));

    RECT summaryCard = expandedRect(childRectOnParent(state.summaryEdit, state.window), s(8), s(24));
    RECT tableCard = expandedRect(childRectOnParent(state.tableList, state.window), s(8), s(24));
    RECT squadCard = expandedRect(childRectOnParent(state.squadList, state.window), s(8), s(24));
    RECT transferCard = expandedRect(childRectOnParent(state.transferList, state.window), s(8), s(24));
    RECT detailCard = expandedRect(childRectOnParent(state.detailEdit, state.window), s(8), s(24));
    RECT newsCard = expandedRect(childRectOnParent(state.newsList, state.window), s(8), s(24));
    RECT statusCard = expandedRect(childRectOnParent(state.statusLabel, state.window), s(6), s(8));
    if (IsWindowVisible(state.summaryEdit)) drawRoundedPanel(hdc, summaryCard, kThemePanel, RGB(40, 64, 79), s(16));
    if (IsWindowVisible(state.tableList)) drawRoundedPanel(hdc, tableCard, kThemePanel, RGB(40, 64, 79), s(16));
    if (IsWindowVisible(state.squadList)) drawRoundedPanel(hdc, squadCard, kThemePanel, RGB(40, 64, 79), s(16));
    if (IsWindowVisible(state.transferList)) drawRoundedPanel(hdc, transferCard, kThemePanel, RGB(40, 64, 79), s(16));
    if (IsWindowVisible(state.detailEdit)) drawRoundedPanel(hdc, detailCard, RGB(15, 27, 37), RGB(44, 72, 90), s(16));
    if (IsWindowVisible(state.newsList)) drawRoundedPanel(hdc, newsCard, RGB(15, 27, 37), RGB(44, 72, 90), s(16));
    drawRoundedPanel(hdc, statusCard, RGB(11, 23, 31), RGB(39, 65, 79), s(12));

    RECT menuTitle{s(20), s(header.topBarHeight + 10), s(header.sideWidth - 10), s(header.topBarHeight + 34)};
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
