#include "gui.h"

#ifdef _WIN32

#include "app_services.h"
#include "ui.h"
#include "utils.h"
#include "validators.h"

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>
#include <commctrl.h>

namespace {

enum ControlId {
    IDC_DIVISION_COMBO = 1001,
    IDC_TEAM_COMBO,
    IDC_MANAGER_EDIT,
    IDC_NEW_CAREER_BUTTON,
    IDC_LOAD_BUTTON,
    IDC_SAVE_BUTTON,
    IDC_SIMULATE_BUTTON,
    IDC_VALIDATE_BUTTON,
    IDC_SUMMARY_EDIT,
    IDC_NEWS_LIST,
    IDC_TABLE_LIST,
    IDC_SQUAD_LIST,
    IDC_TRANSFER_LIST,
    IDC_VIEW_OVERVIEW_BUTTON,
    IDC_VIEW_COMPETITION_BUTTON,
    IDC_VIEW_BOARD_BUTTON,
    IDC_VIEW_CLUB_BUTTON,
    IDC_VIEW_SCOUTING_BUTTON,
    IDC_NEGOTIATION_COMBO,
    IDC_PROMISE_COMBO,
    IDC_SCOUT_BUTTON,
    IDC_SHORTLIST_BUTTON,
    IDC_FOLLOW_SHORTLIST_BUTTON,
    IDC_BUY_BUTTON,
    IDC_PRECONTRACT_BUTTON,
    IDC_RENEW_BUTTON,
    IDC_SELL_BUTTON,
    IDC_PLAN_BUTTON,
    IDC_INSTRUCTION_BUTTON,
    IDC_YOUTH_UPGRADE_BUTTON,
    IDC_TRAINING_UPGRADE_BUTTON,
    IDC_SCOUTING_UPGRADE_BUTTON,
    IDC_STADIUM_UPGRADE_BUTTON
};

enum class SummaryMode {
    Overview,
    Competition,
    Board,
    Club,
    Scouting
};

struct AppState {
    HINSTANCE instance = nullptr;
    HWND window = nullptr;
    HFONT font = nullptr;
    HFONT titleFont = nullptr;
    HFONT sectionFont = nullptr;
    HFONT monoFont = nullptr;
    HBRUSH backgroundBrush = nullptr;
    HBRUSH panelBrush = nullptr;
    HBRUSH headerBrush = nullptr;
    HBRUSH inputBrush = nullptr;

    Career career;
    bool suppressComboEvents = false;
    SummaryMode summaryMode = SummaryMode::Overview;

    HWND divisionCombo = nullptr;
    HWND teamCombo = nullptr;
    HWND managerEdit = nullptr;
    HWND newCareerButton = nullptr;
    HWND loadButton = nullptr;
    HWND saveButton = nullptr;
    HWND simulateButton = nullptr;
    HWND validateButton = nullptr;
    HWND infoLabel = nullptr;
    HWND statusLabel = nullptr;
    HWND summaryLabel = nullptr;
    HWND summaryEdit = nullptr;
    HWND newsLabel = nullptr;
    HWND newsList = nullptr;
    HWND tableLabel = nullptr;
    HWND tableList = nullptr;
    HWND squadLabel = nullptr;
    HWND squadList = nullptr;
    HWND transferLabel = nullptr;
    HWND transferList = nullptr;
    HWND overviewButton = nullptr;
    HWND competitionButton = nullptr;
    HWND boardButton = nullptr;
    HWND clubButton = nullptr;
    HWND scoutingButton = nullptr;
    HWND negotiationCombo = nullptr;
    HWND promiseCombo = nullptr;
    HWND scoutActionButton = nullptr;
    HWND shortlistButton = nullptr;
    HWND followShortlistButton = nullptr;
    HWND buyButton = nullptr;
    HWND preContractButton = nullptr;
    HWND renewButton = nullptr;
    HWND sellButton = nullptr;
    HWND planButton = nullptr;
    HWND instructionButton = nullptr;
    HWND youthUpgradeButton = nullptr;
    HWND trainingUpgradeButton = nullptr;
    HWND scoutingUpgradeButton = nullptr;
    HWND stadiumUpgradeButton = nullptr;
};

constexpr COLORREF kThemeBg = RGB(8, 17, 24);
constexpr COLORREF kThemeHeader = RGB(12, 33, 28);
constexpr COLORREF kThemePanel = RGB(18, 29, 40);
constexpr COLORREF kThemePanelAlt = RGB(13, 23, 32);
constexpr COLORREF kThemeInput = RGB(12, 20, 29);
constexpr COLORREF kThemeAccent = RGB(210, 176, 84);
constexpr COLORREF kThemeAccentGreen = RGB(42, 137, 96);
constexpr COLORREF kThemeText = RGB(236, 239, 232);
constexpr COLORREF kThemeMuted = RGB(145, 164, 167);
constexpr COLORREF kThemeDanger = RGB(191, 87, 87);
constexpr COLORREF kThemeSelection = RGB(68, 92, 108);

RECT childRectOnParent(HWND child, HWND parent) {
    RECT rect{};
    if (!child || !parent) return rect;
    GetWindowRect(child, &rect);
    MapWindowPoints(nullptr, parent, reinterpret_cast<LPPOINT>(&rect), 2);
    return rect;
}

RECT expandedRect(RECT rect, int dx, int dy) {
    rect.left -= dx;
    rect.top -= dy;
    rect.right += dx;
    rect.bottom += dy;
    return rect;
}

void drawRoundedPanel(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius = 16) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void drawPitchOverlay(HDC hdc, const RECT& rect) {
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(42, 88, 68));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    int midX = (rect.left + rect.right) / 2;
    MoveToEx(hdc, midX, rect.top, nullptr);
    LineTo(hdc, midX, rect.bottom);
    int radius = std::min((rect.right - rect.left) / 8, (rect.bottom - rect.top) / 4);
    Ellipse(hdc, midX - radius, (rect.top + rect.bottom) / 2 - radius, midX + radius, (rect.top + rect.bottom) / 2 + radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

bool isSummaryButtonId(int id) {
    return id == IDC_VIEW_OVERVIEW_BUTTON || id == IDC_VIEW_COMPETITION_BUTTON || id == IDC_VIEW_BOARD_BUTTON ||
           id == IDC_VIEW_CLUB_BUTTON || id == IDC_VIEW_SCOUTING_BUTTON;
}

bool isUpgradeButtonId(int id) {
    return id == IDC_YOUTH_UPGRADE_BUTTON || id == IDC_TRAINING_UPGRADE_BUTTON ||
           id == IDC_SCOUTING_UPGRADE_BUTTON || id == IDC_STADIUM_UPGRADE_BUTTON;
}

bool isPrimaryButtonId(int id) {
    return id == IDC_NEW_CAREER_BUTTON || id == IDC_SIMULATE_BUTTON || id == IDC_BUY_BUTTON ||
           id == IDC_RENEW_BUTTON || id == IDC_SCOUT_BUTTON;
}

bool isActiveSummaryButton(const AppState& state, int id) {
    if (id == IDC_VIEW_OVERVIEW_BUTTON) return state.summaryMode == SummaryMode::Overview;
    if (id == IDC_VIEW_COMPETITION_BUTTON) return state.summaryMode == SummaryMode::Competition;
    if (id == IDC_VIEW_BOARD_BUTTON) return state.summaryMode == SummaryMode::Board;
    if (id == IDC_VIEW_CLUB_BUTTON) return state.summaryMode == SummaryMode::Club;
    if (id == IDC_VIEW_SCOUTING_BUTTON) return state.summaryMode == SummaryMode::Scouting;
    return false;
}

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) return std::wstring();
    int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0) return std::wstring(text.begin(), text.end());
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wide[0], length);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}

std::string wideToUtf8(const std::wstring& text) {
    if (text.empty()) return std::string();
    int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0) return std::string(text.begin(), text.end());
    std::string utf8(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &utf8[0], length, nullptr, nullptr);
    if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
    return utf8;
}

std::string toEditText(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 16);
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) out += '\r';
        out += text[i];
    }
    return out;
}

void setWindowTextUtf8(HWND hwnd, const std::string& text) {
    std::wstring wide = utf8ToWide(toEditText(text));
    SetWindowTextW(hwnd, wide.c_str());
}

std::string getWindowTextUtf8(HWND hwnd) {
    int length = GetWindowTextLengthW(hwnd);
    if (length <= 0) return std::string();
    std::wstring buffer(static_cast<size_t>(length + 1), L'\0');
    GetWindowTextW(hwnd, &buffer[0], length + 1);
    buffer.resize(static_cast<size_t>(length));
    return trim(wideToUtf8(buffer));
}

HWND createControl(AppState& state,
                   DWORD exStyle,
                   const wchar_t* className,
                   const wchar_t* text,
                   DWORD style,
                   int x,
                   int y,
                   int width,
                   int height,
                   HWND parent,
                   int id) {
    HWND hwnd = CreateWindowExW(exStyle,
                                className,
                                text,
                                style,
                                x,
                                y,
                                width,
                                height,
                                parent,
                                reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),
                                state.instance,
                                nullptr);
    if (hwnd && state.font) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    }
    return hwnd;
}

void addComboItem(HWND combo, const std::string& text) {
    std::wstring wide = utf8ToWide(text);
    SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
}

int comboIndex(HWND combo) {
    return static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
}

std::string comboText(HWND combo) {
    int index = comboIndex(combo);
    if (index < 0) return std::string();
    int length = static_cast<int>(SendMessageW(combo, CB_GETLBTEXTLEN, index, 0));
    std::wstring buffer(static_cast<size_t>(length + 1), L'\0');
    SendMessageW(combo, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer.data()));
    buffer.resize(static_cast<size_t>(length));
    return wideToUtf8(buffer);
}

int selectedListViewRow(HWND list) {
    return ListView_GetNextItem(list, -1, LVNI_SELECTED);
}

std::string listViewText(HWND list, int row, int subItem) {
    if (row < 0) return std::string();
    wchar_t buffer[512]{};
    LVITEMW item{};
    item.iSubItem = subItem;
    item.pszText = buffer;
    item.cchTextMax = static_cast<int>(sizeof(buffer) / sizeof(buffer[0]));
    SendMessageW(list, LVM_GETITEMTEXTW, static_cast<WPARAM>(row), reinterpret_cast<LPARAM>(&item));
    return wideToUtf8(buffer);
}

std::string summaryTitleFor(SummaryMode mode) {
    switch (mode) {
        case SummaryMode::Overview: return "Resumen";
        case SummaryMode::Competition: return "Competicion";
        case SummaryMode::Board: return "Directiva";
        case SummaryMode::Club: return "Club y Finanzas";
        case SummaryMode::Scouting: return "Scouting";
    }
    return "Resumen";
}

NegotiationProfile selectedNegotiationProfile(const AppState& state) {
    int index = comboIndex(state.negotiationCombo);
    if (index <= 0) return NegotiationProfile::Safe;
    if (index == 2) return NegotiationProfile::Aggressive;
    return NegotiationProfile::Balanced;
}

NegotiationPromise selectedNegotiationPromise(const AppState& state) {
    int index = comboIndex(state.promiseCombo);
    if (index == 1) return NegotiationPromise::Starter;
    if (index == 2) return NegotiationPromise::Rotation;
    if (index == 3) return NegotiationPromise::Prospect;
    return NegotiationPromise::None;
}

std::string formatMoney(long long value) {
    bool negative = value < 0;
    unsigned long long absValue = static_cast<unsigned long long>(negative ? -value : value);
    std::string digits = std::to_string(absValue);
    std::string out;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return std::string(negative ? "-$" : "$") + out;
}

std::string shortInstructionLabel(const std::string& value) {
    if (value == "Laterales altos") return "Instr LA";
    if (value == "Bloque bajo") return "Instr BB";
    if (value == "Balon parado") return "Instr BP";
    if (value == "Presion final") return "Instr PF";
    if (value == "Por bandas") return "Instr PB";
    if (value == "Juego directo") return "Instr JD";
    if (value == "Contra-presion") return "Instr CP";
    if (value == "Pausar juego") return "Instr PJ";
    return "Instr EQ";
}

std::string boardStatusLabel(int confidence) {
    if (confidence >= 75) return "Muy alta";
    if (confidence >= 55) return "Estable";
    if (confidence >= 35) return "En observacion";
    if (confidence >= 20) return "Bajo presion";
    return "Critica";
}

std::string selectedDivisionId(const AppState& state) {
    int index = comboIndex(state.divisionCombo);
    if (index < 0 || index >= static_cast<int>(state.career.divisions.size())) return std::string();
    return state.career.divisions[static_cast<size_t>(index)].id;
}

void fillDivisionCombo(AppState& state, const std::string& selectedId = std::string()) {
    state.suppressComboEvents = true;
    SendMessageW(state.divisionCombo, CB_RESETCONTENT, 0, 0);
    int selectedIndex = 0;
    for (size_t i = 0; i < state.career.divisions.size(); ++i) {
        addComboItem(state.divisionCombo, state.career.divisions[i].display);
        if (!selectedId.empty() && state.career.divisions[i].id == selectedId) {
            selectedIndex = static_cast<int>(i);
        }
    }
    if (!state.career.divisions.empty()) {
        SendMessageW(state.divisionCombo, CB_SETCURSEL, selectedIndex, 0);
    }
    state.suppressComboEvents = false;
}

void fillTeamCombo(AppState& state, const std::string& divisionId, const std::string& selectedTeam = std::string()) {
    state.suppressComboEvents = true;
    SendMessageW(state.teamCombo, CB_RESETCONTENT, 0, 0);
    auto teams = state.career.getDivisionTeams(divisionId);
    int selectedIndex = 0;
    for (size_t i = 0; i < teams.size(); ++i) {
        addComboItem(state.teamCombo, teams[i]->name);
        if (!selectedTeam.empty() && teams[i]->name == selectedTeam) {
            selectedIndex = static_cast<int>(i);
        }
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

LeagueTable buildTableFromTeams(const std::vector<Team*>& teams, const std::string& title, const std::string& ruleId) {
    LeagueTable table;
    table.title = title;
    table.ruleId = ruleId;
    for (Team* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

LeagueTable relevantTable(const Career& career) {
    if (!career.myTeam) return career.leagueTable;
    if (!career.usesGroupFormat() || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }

    std::vector<Team*> teams;
    const std::vector<int>* indices = nullptr;
    for (int index : career.groupNorthIdx) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size()) && career.activeTeams[index] == career.myTeam) {
            indices = &career.groupNorthIdx;
            break;
        }
    }
    if (!indices) {
        for (int index : career.groupSouthIdx) {
            if (index >= 0 && index < static_cast<int>(career.activeTeams.size()) && career.activeTeams[index] == career.myTeam) {
                indices = &career.groupSouthIdx;
                break;
            }
        }
    }
    if (!indices) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }
    for (int index : *indices) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size())) {
            teams.push_back(career.activeTeams[index]);
        }
    }
    std::string title = career.leagueTable.title;
    if (indices == &career.groupNorthIdx) title = "Grupo / Zona Norte";
    if (indices == &career.groupSouthIdx) title = "Grupo / Zona Sur";
    return buildTableFromTeams(teams, title, career.activeDivision);
}

void setStatus(AppState& state, const std::string& text) {
    setWindowTextUtf8(state.statusLabel, text);
    if (state.window) InvalidateRect(state.statusLabel, nullptr, TRUE);
}

void clearListView(HWND list) {
    ListView_DeleteAllItems(list);
}

void addListViewRow(HWND list, const std::vector<std::string>& values) {
    if (values.empty()) return;
    std::vector<std::wstring> wideValues;
    wideValues.reserve(values.size());
    for (const auto& value : values) {
        wideValues.push_back(utf8ToWide(value));
    }

    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(list);
    item.pszText = const_cast<LPWSTR>(wideValues[0].c_str());
    SendMessageW(list, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));

    for (size_t i = 1; i < wideValues.size(); ++i) {
        LVITEMW subItem{};
        subItem.iSubItem = static_cast<int>(i);
        subItem.pszText = const_cast<LPWSTR>(wideValues[i].c_str());
        SendMessageW(list, LVM_SETITEMTEXTW, item.iItem, reinterpret_cast<LPARAM>(&subItem));
    }
}

void configureListViewColumns(HWND list, const std::vector<std::pair<std::wstring, int>>& columns) {
    for (size_t i = 0; i < columns.size(); ++i) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<LPWSTR>(columns[i].first.c_str());
        column.cx = columns[i].second;
        column.iSubItem = static_cast<int>(i);
        SendMessageW(list, LVM_INSERTCOLUMNW, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(&column));
    }
}

void refreshHeader(AppState& state) {
    bool hasCareer = state.career.myTeam != nullptr;
    EnableWindow(state.saveButton, hasCareer);
    EnableWindow(state.simulateButton, hasCareer);
    EnableWindow(state.overviewButton, hasCareer);
    EnableWindow(state.competitionButton, hasCareer);
    EnableWindow(state.boardButton, hasCareer);
    EnableWindow(state.clubButton, hasCareer);
    EnableWindow(state.scoutingButton, hasCareer);
    EnableWindow(state.negotiationCombo, hasCareer);
    EnableWindow(state.promiseCombo, hasCareer);
    EnableWindow(state.scoutActionButton, hasCareer);
    EnableWindow(state.shortlistButton, hasCareer);
    EnableWindow(state.followShortlistButton, hasCareer);
    EnableWindow(state.buyButton, hasCareer);
    EnableWindow(state.preContractButton, hasCareer);
    EnableWindow(state.renewButton, hasCareer);
    EnableWindow(state.sellButton, hasCareer);
    EnableWindow(state.planButton, hasCareer);
    EnableWindow(state.instructionButton, hasCareer);
    EnableWindow(state.youthUpgradeButton, hasCareer);
    EnableWindow(state.trainingUpgradeButton, hasCareer);
    EnableWindow(state.scoutingUpgradeButton, hasCareer);
    EnableWindow(state.stadiumUpgradeButton, hasCareer);
    setWindowTextUtf8(state.instructionButton,
                      hasCareer ? shortInstructionLabel(state.career.myTeam->matchInstruction) : "Instruccion");

    if (!hasCareer) {
        setWindowTextUtf8(state.infoLabel, "Selecciona division, equipo y manager para crear una carrera o cargar un guardado.");
        setWindowTextUtf8(state.summaryLabel, "Resumen");
        return;
    }

    Team& team = *state.career.myTeam;
    std::ostringstream out;
    out << "Club: " << team.name
        << " | Division: " << divisionDisplay(state.career.activeDivision)
        << " | Temporada " << state.career.currentSeason
        << " | Semana " << state.career.currentWeek << "/" << std::max(1, static_cast<int>(state.career.schedule.size()))
        << " | Pos " << state.career.currentCompetitiveRank() << "/" << std::max(1, state.career.currentCompetitiveFieldSize())
        << " | Presupuesto " << formatMoney(team.budget)
        << " | Directiva " << boardStatusLabel(state.career.boardConfidence);
    setWindowTextUtf8(state.infoLabel, out.str());
    setWindowTextUtf8(state.summaryLabel, summaryTitleFor(state.summaryMode));
    if (state.window) InvalidateRect(state.window, nullptr, FALSE);
}

void refreshSummary(AppState& state) {
    if (!state.career.myTeam) {
        setWindowTextUtf8(state.summaryLabel, "Resumen");
        setWindowTextUtf8(state.summaryEdit, "Crea una nueva carrera o carga una partida guardada para empezar.");
        return;
    }

    setWindowTextUtf8(state.summaryLabel, summaryTitleFor(state.summaryMode));
    if (state.summaryMode == SummaryMode::Competition) {
        setWindowTextUtf8(state.summaryEdit, buildCompetitionSummaryService(state.career));
        return;
    }
    if (state.summaryMode == SummaryMode::Board) {
        setWindowTextUtf8(state.summaryEdit, buildBoardSummaryService(state.career));
        return;
    }
    if (state.summaryMode == SummaryMode::Club) {
        setWindowTextUtf8(state.summaryEdit, buildClubSummaryService(state.career));
        return;
    }
    if (state.summaryMode == SummaryMode::Scouting) {
        setWindowTextUtf8(state.summaryEdit, buildScoutingSummaryService(state.career));
        return;
    }

    Team& team = *state.career.myTeam;
    LeagueTable table = relevantTable(state.career);

    int injured = 0;
    int suspended = 0;
    int totalFitness = 0;
    int expiringContracts = 0;
    int wantsToLeave = 0;
    int incomingLoans = 0;
    int outgoingLoans = 0;
    int promisesAtRisk = 0;
    int leaders = 0;
    for (const auto& player : team.players) {
        if (player.injured) injured++;
        if (player.matchesSuspended > 0) suspended++;
        totalFitness += player.fitness;
        if (player.contractWeeks <= 12) expiringContracts++;
        if (player.wantsToLeave) wantsToLeave++;
        if (player.onLoan && !player.parentClub.empty()) incomingLoans++;
        if (player.leadership >= 72 || playerHasTrait(player, "Lider")) leaders++;
        if ((player.promisedRole == "Titular" && player.startsThisSeason + 2 < std::max(2, state.career.currentWeek * 2 / 3)) ||
            (player.promisedRole == "Rotacion" && player.startsThisSeason + 1 < std::max(1, state.career.currentWeek / 3)) ||
            (player.promisedRole == "Proyecto" && player.age <= 22 && player.startsThisSeason < std::max(1, state.career.currentWeek / 4))) {
            promisesAtRisk++;
        }
    }
    for (const auto& club : state.career.allTeams) {
        if (&club == state.career.myTeam) continue;
        for (const auto& player : club.players) {
            if (player.onLoan && player.parentClub == team.name) outgoingLoans++;
        }
    }
    int avgFitness = team.players.empty() ? 0 : totalFitness / static_cast<int>(team.players.size());

    std::ostringstream out;
    out << "Equipo: " << team.name << "\r\n";
    out << "Manager: " << state.career.managerName
        << " | Reputacion " << state.career.managerReputation
        << " | Confianza " << boardStatusLabel(state.career.boardConfidence)
        << " (" << state.career.boardConfidence << "/100)\r\n";
    out << "Puntos: " << team.points
        << " | DG " << (team.goalsFor - team.goalsAgainst)
        << " | Record " << team.wins << "-" << team.draws << "-" << team.losses
        << " | Fechas restantes "
        << std::max(0, static_cast<int>(state.career.schedule.size()) - state.career.currentWeek + 1) << "\r\n";
    out << "Tactica: " << team.tactics
        << " | Formacion: " << team.formation
        << " | Entrenamiento: " << team.trainingFocus
        << " | Rotacion: " << team.rotationPolicy
        << " | Instruccion: " << team.matchInstruction << "\r\n";
    out << "Presion " << team.pressingIntensity
        << " | Linea " << team.defensiveLine
        << " | Tempo " << team.tempo
        << " | Amplitud " << team.width
        << " | Marcaje " << team.markingStyle << "\r\n";
    out << "Plantel: " << team.players.size()
        << " | Hab media " << team.getAverageSkill()
        << " | Valor " << formatMoney(team.getSquadValue())
        << " | Condicion media " << avgFitness << "\r\n";
    out << "Bajas: " << injured << " lesionados | " << suspended << " suspendidos\r\n";
    out << "Mercado: " << state.career.pendingTransfers.size() << " pendientes"
        << " | " << expiringContracts << " contratos cortos"
        << " | " << wantsToLeave << " quieren salir"
        << " | Prestamos " << incomingLoans << "/" << outgoingLoans << "\r\n";
    out << "Vestuario: lideres " << leaders
        << " | promesas en riesgo " << promisesAtRisk << "\r\n";
    out << "Sponsor semanal: " << formatMoney(team.sponsorWeekly)
        << " | Deuda: " << formatMoney(team.debt) << "\r\n";
    if (!state.career.boardMonthlyObjective.empty()) {
        int weeksLeft = std::max(0, state.career.boardMonthlyDeadlineWeek - state.career.currentWeek);
        out << "Objetivo mensual: " << state.career.boardMonthlyObjective
            << " | Progreso " << state.career.boardMonthlyProgress << "/" << state.career.boardMonthlyTarget
            << " | Cierre en " << weeksLeft << " semana(s)\r\n";
    }
    if (!table.teams.empty()) {
        out << "Lider actual: " << table.teams.front()->name << " (" << table.teams.front()->points << " pts)\r\n";
    }
    if (!state.career.cupChampion.empty()) {
        out << "Copa: campeon " << state.career.cupChampion << "\r\n";
    } else if (state.career.cupActive) {
        out << "Copa: ronda " << state.career.cupRound + 1
            << " | Equipos vivos " << state.career.cupRemainingTeams.size() << "\r\n";
    }
    if (state.career.currentWeek <= static_cast<int>(state.career.schedule.size())) {
        out << "\r\nProxima fecha:\r\n";
        for (const auto& match : state.career.schedule[static_cast<size_t>(state.career.currentWeek - 1)]) {
            if (match.first < 0 || match.second < 0 ||
                match.first >= static_cast<int>(state.career.activeTeams.size()) ||
                match.second >= static_cast<int>(state.career.activeTeams.size())) {
                continue;
            }
            Team* home = state.career.activeTeams[static_cast<size_t>(match.first)];
            Team* away = state.career.activeTeams[static_cast<size_t>(match.second)];
            out << "- " << home->name << " vs " << away->name;
            if (home == state.career.myTeam || away == state.career.myTeam) out << " <- tu partido";
            out << "\r\n";
        }
    }
    if (!state.career.lastMatchAnalysis.empty()) {
        out << "\r\nUltimo partido:\r\n" << toEditText(state.career.lastMatchAnalysis);
    }
    if (!state.career.history.empty()) {
        const auto& latest = state.career.history.back();
        out << "\r\nUltimo cierre de temporada:\r\n";
        out << "- Temporada " << latest.season
            << " | Club " << latest.club
            << " | Puesto " << latest.finish
            << " | Campeon " << latest.champion << "\r\n";
    }

    setWindowTextUtf8(state.summaryEdit, out.str());
}

void refreshNews(AppState& state) {
    SendMessageW(state.newsList, LB_RESETCONTENT, 0, 0);
    if (!state.career.myTeam) return;

    if (!state.career.lastMatchAnalysis.empty()) {
        std::wstring line = utf8ToWide("[Ultimo partido] " + state.career.lastMatchAnalysis);
        SendMessageW(state.newsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }

    if (state.career.newsFeed.empty()) {
        std::wstring empty = utf8ToWide("No hay noticias todavia.");
        SendMessageW(state.newsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(empty.c_str()));
        return;
    }

    for (auto it = state.career.newsFeed.rbegin(); it != state.career.newsFeed.rend(); ++it) {
        std::wstring line = utf8ToWide(*it);
        SendMessageW(state.newsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }
}

std::string playerRole(const Team&, int index, const std::vector<int>& xi, const std::vector<int>& bench) {
    if (std::find(xi.begin(), xi.end(), index) != xi.end()) return "XI";
    if (std::find(bench.begin(), bench.end(), index) != bench.end()) return "Banco";
    return "Plantel";
}

std::string playerStatus(const Player& player) {
    std::vector<std::string> flags;
    if (player.injured) flags.push_back("Les");
    if (player.matchesSuspended > 0) flags.push_back("Susp " + std::to_string(player.matchesSuspended));
    if (player.wantsToLeave) flags.push_back("Salida");
    if (player.onLoan) flags.push_back("Prestamo");
    if (flags.empty()) return "OK";

    std::ostringstream out;
    for (size_t i = 0; i < flags.size(); ++i) {
        if (i) out << " | ";
        out << flags[i];
    }
    return out.str();
}

std::string transferTypeFor(const PendingTransfer& move) {
    if (move.preContract) return "Precontrato";
    if (move.loan) return "Prestamo";
    return "Pendiente";
}

struct TransferPreviewItem {
    std::string type;
    std::string player;
    std::string club;
    std::string detail;
    std::string amount;
};

std::vector<TransferPreviewItem> buildTransferPreview(const Career& career) {
    std::vector<TransferPreviewItem> rows;
    if (!career.myTeam) return rows;

    const Team& team = *career.myTeam;
    rows.reserve(32);

    for (const auto& move : career.pendingTransfers) {
        std::ostringstream detail;
        detail << transferTypeFor(move) << " para T" << move.effectiveSeason;
        if (move.contractWeeks > 0) detail << " | contrato " << move.contractWeeks << " sem";
        if (!move.promisedRole.empty()) detail << " | promesa " << move.promisedRole;
        rows.push_back({
            transferTypeFor(move),
            move.playerName,
            move.fromTeam + " -> " + move.toTeam,
            detail.str(),
            move.fee > 0 ? formatMoney(move.fee) : formatMoney(move.wage)
        });
    }

    for (const auto& player : team.players) {
        if (player.contractWeeks <= 12) {
            rows.push_back({
                "Contrato",
                player.name,
                team.name,
                "Vence en " + std::to_string(player.contractWeeks) + " sem",
                formatMoney(player.wage)
            });
        }
        if (player.wantsToLeave) {
            rows.push_back({
                "Salida",
                player.name,
                team.name,
                "Pidio salir del club",
                formatMoney(player.value)
            });
        }
        if (player.onLoan && !player.parentClub.empty()) {
            rows.push_back({
                "Prestamo",
                player.name,
                player.parentClub + " -> " + team.name,
                std::to_string(std::max(0, player.loanWeeksRemaining)) + " sem restantes",
                formatMoney(player.wage)
            });
        }
    }

    for (const auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        for (const auto& player : club.players) {
            if (player.onLoan && player.parentClub == team.name) {
                rows.push_back({
                    "Cedido",
                    player.name,
                    team.name + " -> " + club.name,
                    std::to_string(std::max(0, player.loanWeeksRemaining)) + " sem restantes",
                    formatMoney(player.value)
                });
            }
        }
    }

    std::vector<std::pair<const Team*, int>> marketPool;
    for (const auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (club.players.size() <= 18) continue;
        for (size_t i = 0; i < club.players.size(); ++i) {
            const Player& player = club.players[i];
            if (player.onLoan) continue;
            if (player.age > 35) continue;
            marketPool.push_back({&club, static_cast<int>(i)});
        }
    }
    std::sort(marketPool.begin(), marketPool.end(), [](const auto& left, const auto& right) {
        const Player& a = left.first->players[static_cast<size_t>(left.second)];
        const Player& b = right.first->players[static_cast<size_t>(right.second)];
        if (a.skill != b.skill) return a.skill > b.skill;
        if (a.value != b.value) return a.value < b.value;
        return a.name < b.name;
    });
    if (marketPool.size() > 10) marketPool.resize(10);

    for (const auto& entry : marketPool) {
        const Team* seller = entry.first;
        const Player& player = seller->players[static_cast<size_t>(entry.second)];
        long long ask = std::max(player.value, player.releaseClause * 65 / 100);
        std::ostringstream detail;
        detail << normalizePosition(player.position)
               << " | Hab " << player.skill
               << " | Pie " << player.preferredFoot
               << " | Forma " << playerFormLabel(player)
               << " | Contrato " << player.contractWeeks << " sem"
               << " | " << joinStringValues(player.traits, ", ");
        rows.push_back({
            "Mercado",
            player.name,
            seller->name,
            detail.str(),
            formatMoney(ask)
        });
    }

    return rows;
}

int positionRank(const Player& player) {
    std::string position = normalizePosition(player.position);
    if (position == "ARQ") return 0;
    if (position == "DEF") return 1;
    if (position == "MED") return 2;
    if (position == "DEL") return 3;
    return 4;
}

void refreshSquad(AppState& state) {
    clearListView(state.squadList);
    if (!state.career.myTeam) return;

    Team& team = *state.career.myTeam;
    auto xi = team.getStartingXIIndices();
    auto bench = team.getBenchIndices();

    std::vector<int> indices;
    indices.reserve(team.players.size());
    for (size_t i = 0; i < team.players.size(); ++i) {
        indices.push_back(static_cast<int>(i));
    }
    std::sort(indices.begin(), indices.end(), [&](int left, int right) {
        const Player& a = team.players[static_cast<size_t>(left)];
        const Player& b = team.players[static_cast<size_t>(right)];
        if (positionRank(a) != positionRank(b)) return positionRank(a) < positionRank(b);
        if (a.skill != b.skill) return a.skill > b.skill;
        if (a.fitness != b.fitness) return a.fitness > b.fitness;
        return a.name < b.name;
    });

    for (int index : indices) {
        const Player& player = team.players[static_cast<size_t>(index)];
        addListViewRow(state.squadList,
                       {
                           std::to_string(index + 1),
                           playerRole(team, index, xi, bench),
                           normalizePosition(player.position),
                           player.name,
                           std::to_string(player.skill),
                           std::to_string(player.potential),
                           std::to_string(player.age),
                           std::to_string(player.fitness),
                           player.developmentPlan,
                           player.promisedRole,
                           playerStatus(player)
                       });
    }
}

void refreshTransfers(AppState& state) {
    clearListView(state.transferList);
    if (!state.career.myTeam) {
        setWindowTextUtf8(state.transferLabel, "Centro de transferencias");
        return;
    }

    const Team& team = *state.career.myTeam;
    int expiringContracts = 0;
    int wantsToLeave = 0;
    int incomingLoans = 0;
    int outgoingLoans = 0;

    for (const auto& player : team.players) {
        if (player.contractWeeks <= 12) expiringContracts++;
        if (player.wantsToLeave) wantsToLeave++;
        if (player.onLoan && !player.parentClub.empty()) incomingLoans++;
    }
    for (const auto& club : state.career.allTeams) {
        if (&club == state.career.myTeam) continue;
        for (const auto& player : club.players) {
            if (player.onLoan && player.parentClub == team.name) outgoingLoans++;
        }
    }

    std::ostringstream label;
    label << "Centro de transferencias"
          << " | Pendientes " << state.career.pendingTransfers.size()
          << " | Contratos cortos " << expiringContracts
          << " | Salidas " << wantsToLeave
          << " | Prestamos " << incomingLoans << "/" << outgoingLoans;
    setWindowTextUtf8(state.transferLabel, label.str());

    std::vector<TransferPreviewItem> rows = buildTransferPreview(state.career);
    if (rows.empty()) {
        addListViewRow(state.transferList,
                       {
                           "Info",
                           "-",
                           team.name,
                           "No hay movimientos ni objetivos destacados en este momento.",
                           "-"
                       });
        return;
    }

    for (const auto& row : rows) {
        addListViewRow(state.transferList,
                       {
                           row.type,
                           row.player,
                           row.club,
                           row.detail,
                           row.amount
                       });
    }
}

void refreshTable(AppState& state) {
    clearListView(state.tableList);
    if (!state.career.myTeam) {
        setWindowTextUtf8(state.tableLabel, "Tabla");
        return;
    }

    LeagueTable table = relevantTable(state.career);
    std::string title = table.title.empty() ? "Tabla actual" : table.title;
    if (state.career.usesGroupFormat()) title += " (grupo del club)";
    setWindowTextUtf8(state.tableLabel, title);

    for (size_t i = 0; i < table.teams.size(); ++i) {
        Team* team = table.teams[i];
        if (!team) continue;
        int played = team->wins + team->draws + team->losses;
        int goalDiff = team->goalsFor - team->goalsAgainst;
        std::string name = team->name;
        if (team == state.career.myTeam) name += " *";
        addListViewRow(state.tableList,
                       {
                           std::to_string(i + 1),
                           name,
                           std::to_string(team->points),
                           std::to_string(played),
                           std::to_string(team->wins),
                           std::to_string(team->draws),
                           std::to_string(team->losses),
                           std::to_string(team->goalsFor),
                           std::to_string(team->goalsAgainst),
                           std::to_string(goalDiff)
                       });
    }
}

void refreshAll(AppState& state) {
    refreshHeader(state);
    refreshSummary(state);
    refreshNews(state);
    refreshTable(state);
    refreshSquad(state);
    refreshTransfers(state);
}

void configureViews(AppState& state) {
    configureListViewColumns(state.tableList,
                             {
                                 {L"Pos", 45},
                                 {L"Equipo", 210},
                                 {L"Pts", 55},
                                 {L"PJ", 55},
                                 {L"G", 45},
                                 {L"E", 45},
                                 {L"P", 45},
                                 {L"GF", 55},
                                 {L"GA", 55},
                                 {L"DG", 55}
                             });
    configureListViewColumns(state.squadList,
                             {
                                 {L"#", 45},
                                 {L"Rol", 70},
                                 {L"Pos", 55},
                                 {L"Jugador", 220},
                                 {L"Hab", 55},
                                 {L"Pot", 55},
                                 {L"Edad", 55},
                                 {L"Cond", 55},
                                 {L"Plan", 110},
                                 {L"Prom", 90},
                                 {L"Estado", 200}
                             });
    configureListViewColumns(state.transferList,
                             {
                                 {L"Tipo", 110},
                                 {L"Jugador", 220},
                                 {L"Club", 260},
                                 {L"Detalle", 430},
                                 {L"Monto", 120}
                             });

    DWORD styles = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
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
}

void layoutWindow(AppState& state) {
    RECT client{};
    GetClientRect(state.window, &client);

    const int padding = 12;
    const int rowHeight = 26;
    const int smallButtonWidth = 74;
    const int buttonGap = 6;

    MoveWindow(state.divisionCombo, 78, 12, 190, rowHeight, TRUE);
    MoveWindow(state.teamCombo, 332, 12, 240, rowHeight, TRUE);
    MoveWindow(state.managerEdit, 656, 12, 210, 24, TRUE);

    MoveWindow(state.newCareerButton, 12, 46, 120, rowHeight, TRUE);
    MoveWindow(state.loadButton, 140, 46, 100, rowHeight, TRUE);
    MoveWindow(state.saveButton, 248, 46, 100, rowHeight, TRUE);
    MoveWindow(state.simulateButton, 356, 46, 150, rowHeight, TRUE);
    MoveWindow(state.validateButton, 514, 46, 120, rowHeight, TRUE);

    int actionY = 80;
    int x = padding;
    MoveWindow(state.overviewButton, x, actionY, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.competitionButton, x, actionY, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.boardButton, x, actionY, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.clubButton, x, actionY, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.scoutingButton, x, actionY, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.negotiationCombo, x, actionY, 108, 180, TRUE); x += 108 + buttonGap;
    MoveWindow(state.promiseCombo, x, actionY, 108, 180, TRUE);

    int actionY2 = actionY + rowHeight + 8;
    x = padding;
    MoveWindow(state.scoutActionButton, x, actionY2, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.shortlistButton, x, actionY2, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.followShortlistButton, x, actionY2, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.buyButton, x, actionY2, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.preContractButton, x, actionY2, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.renewButton, x, actionY2, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.sellButton, x, actionY2, smallButtonWidth, rowHeight, TRUE);

    int actionY3 = actionY2 + rowHeight + 8;
    x = padding;
    MoveWindow(state.planButton, x, actionY3, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.instructionButton, x, actionY3, 120, rowHeight, TRUE); x += 120 + buttonGap;
    MoveWindow(state.youthUpgradeButton, x, actionY3, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.trainingUpgradeButton, x, actionY3, smallButtonWidth + 8, rowHeight, TRUE); x += smallButtonWidth + 8 + buttonGap;
    MoveWindow(state.scoutingUpgradeButton, x, actionY3, smallButtonWidth, rowHeight, TRUE); x += smallButtonWidth + buttonGap;
    MoveWindow(state.stadiumUpgradeButton, x, actionY3, smallButtonWidth + 6, rowHeight, TRUE);

    MoveWindow(state.infoLabel, padding, 178, std::max(240, static_cast<int>(client.right) - padding * 2), 22, TRUE);

    int contentTop = 208;
    int contentBottom = client.bottom - 36;
    int contentHeight = std::max(360, contentBottom - contentTop);
    int halfWidth = std::max(280, (static_cast<int>(client.right) - padding * 3) / 2);
    int topHeight = std::max(170, contentHeight * 33 / 100);
    int middleHeight = std::max(150, contentHeight * 24 / 100);
    int middleTop = contentTop + topHeight + 30;
    int transferTop = middleTop + middleHeight + 30;
    int transferHeight = std::max(140, contentBottom - transferTop - 18);

    MoveWindow(state.summaryLabel, padding, contentTop, halfWidth, 18, TRUE);
    MoveWindow(state.summaryEdit, padding, contentTop + 18, halfWidth, topHeight, TRUE);
    MoveWindow(state.newsLabel, padding * 2 + halfWidth, contentTop, halfWidth, 18, TRUE);
    MoveWindow(state.newsList, padding * 2 + halfWidth, contentTop + 18, halfWidth, topHeight, TRUE);

    MoveWindow(state.tableLabel, padding, middleTop, halfWidth, 18, TRUE);
    MoveWindow(state.tableList, padding, middleTop + 18, halfWidth, middleHeight, TRUE);

    MoveWindow(state.squadLabel, padding * 2 + halfWidth, middleTop, halfWidth, 18, TRUE);
    MoveWindow(state.squadList, padding * 2 + halfWidth, middleTop + 18, halfWidth, middleHeight, TRUE);

    MoveWindow(state.transferLabel, padding, transferTop, std::max(240, static_cast<int>(client.right) - padding * 2), 18, TRUE);
    MoveWindow(state.transferList, padding, transferTop + 18, std::max(240, static_cast<int>(client.right) - padding * 2), transferHeight, TRUE);

    MoveWindow(state.statusLabel, padding, client.bottom - 24, std::max(240, static_cast<int>(client.right) - padding * 2), 18, TRUE);
}

void initializeInterface(AppState& state) {
    state.font = CreateFontW(-17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Bahnschrift");
    state.titleFont = CreateFontW(-34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
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

    createControl(state, 0, L"STATIC", L"Division", WS_CHILD | WS_VISIBLE, 12, 16, 60, 20, state.window, 0);
    createControl(state, 0, L"STATIC", L"Equipo", WS_CHILD | WS_VISIBLE, 280, 16, 50, 20, state.window, 0);
    createControl(state, 0, L"STATIC", L"Manager", WS_CHILD | WS_VISIBLE, 586, 16, 60, 20, state.window, 0);

    state.divisionCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 78, 12, 190, 300, state.window, IDC_DIVISION_COMBO);
    state.teamCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 332, 12, 240, 300, state.window, IDC_TEAM_COMBO);
    state.managerEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"Manager", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 656, 12, 210, 24, state.window, IDC_MANAGER_EDIT);

    state.newCareerButton = createControl(state, 0, L"BUTTON", L"Nueva carrera", buttonStyle, 12, 46, 120, 26, state.window, IDC_NEW_CAREER_BUTTON);
    state.loadButton = createControl(state, 0, L"BUTTON", L"Cargar", buttonStyle, 140, 46, 100, 26, state.window, IDC_LOAD_BUTTON);
    state.saveButton = createControl(state, 0, L"BUTTON", L"Guardar", buttonStyle, 248, 46, 100, 26, state.window, IDC_SAVE_BUTTON);
    state.simulateButton = createControl(state, 0, L"BUTTON", L"Simular semana", buttonStyle, 356, 46, 150, 26, state.window, IDC_SIMULATE_BUTTON);
    state.validateButton = createControl(state, 0, L"BUTTON", L"Validar", buttonStyle, 514, 46, 120, 26, state.window, IDC_VALIDATE_BUTTON);
    state.overviewButton = createControl(state, 0, L"BUTTON", L"Resumen", buttonStyle, 12, 80, 88, 26, state.window, IDC_VIEW_OVERVIEW_BUTTON);
    state.competitionButton = createControl(state, 0, L"BUTTON", L"Compet.", buttonStyle, 104, 80, 88, 26, state.window, IDC_VIEW_COMPETITION_BUTTON);
    state.boardButton = createControl(state, 0, L"BUTTON", L"Directiva", buttonStyle, 196, 80, 88, 26, state.window, IDC_VIEW_BOARD_BUTTON);
    state.clubButton = createControl(state, 0, L"BUTTON", L"Club", buttonStyle, 288, 80, 88, 26, state.window, IDC_VIEW_CLUB_BUTTON);
    state.scoutingButton = createControl(state, 0, L"BUTTON", L"Scouting", buttonStyle, 380, 80, 88, 26, state.window, IDC_VIEW_SCOUTING_BUTTON);
    state.negotiationCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 472, 80, 110, 180, state.window, IDC_NEGOTIATION_COMBO);
    state.promiseCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 588, 80, 110, 180, state.window, IDC_PROMISE_COMBO);
    state.scoutActionButton = createControl(state, 0, L"BUTTON", L"Otear", buttonStyle, 12, 114, 88, 26, state.window, IDC_SCOUT_BUTTON);
    state.shortlistButton = createControl(state, 0, L"BUTTON", L"Seguir", buttonStyle, 104, 114, 88, 26, state.window, IDC_SHORTLIST_BUTTON);
    state.followShortlistButton = createControl(state, 0, L"BUTTON", L"Actualizar", buttonStyle, 196, 114, 88, 26, state.window, IDC_FOLLOW_SHORTLIST_BUTTON);
    state.buyButton = createControl(state, 0, L"BUTTON", L"Fichar", buttonStyle, 288, 114, 88, 26, state.window, IDC_BUY_BUTTON);
    state.preContractButton = createControl(state, 0, L"BUTTON", L"Precontr.", buttonStyle, 380, 114, 88, 26, state.window, IDC_PRECONTRACT_BUTTON);
    state.renewButton = createControl(state, 0, L"BUTTON", L"Renovar", buttonStyle, 472, 114, 88, 26, state.window, IDC_RENEW_BUTTON);
    state.sellButton = createControl(state, 0, L"BUTTON", L"Vender", buttonStyle, 564, 114, 88, 26, state.window, IDC_SELL_BUTTON);
    state.planButton = createControl(state, 0, L"BUTTON", L"Plan+", buttonStyle, 12, 148, 88, 26, state.window, IDC_PLAN_BUTTON);
    state.instructionButton = createControl(state, 0, L"BUTTON", L"Instruccion", buttonStyle, 104, 148, 110, 26, state.window, IDC_INSTRUCTION_BUTTON);
    state.youthUpgradeButton = createControl(state, 0, L"BUTTON", L"Cantera+", buttonStyle, 220, 148, 88, 26, state.window, IDC_YOUTH_UPGRADE_BUTTON);
    state.trainingUpgradeButton = createControl(state, 0, L"BUTTON", L"Entreno+", buttonStyle, 312, 148, 94, 26, state.window, IDC_TRAINING_UPGRADE_BUTTON);
    state.scoutingUpgradeButton = createControl(state, 0, L"BUTTON", L"Scout+", buttonStyle, 410, 148, 88, 26, state.window, IDC_SCOUTING_UPGRADE_BUTTON);
    state.stadiumUpgradeButton = createControl(state, 0, L"BUTTON", L"Estadio+", buttonStyle, 502, 148, 94, 26, state.window, IDC_STADIUM_UPGRADE_BUTTON);

    addComboItem(state.negotiationCombo, "Segura");
    addComboItem(state.negotiationCombo, "Balanceada");
    addComboItem(state.negotiationCombo, "Agresiva");
    SendMessageW(state.negotiationCombo, CB_SETCURSEL, 1, 0);
    addComboItem(state.promiseCombo, "Sin promesa");
    addComboItem(state.promiseCombo, "Titular");
    addComboItem(state.promiseCombo, "Rotacion");
    addComboItem(state.promiseCombo, "Proyecto");
    SendMessageW(state.promiseCombo, CB_SETCURSEL, 0, 0);

    state.infoLabel = createControl(state, 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 12, 178, 900, 22, state.window, 0);
    state.summaryLabel = createControl(state, 0, L"STATIC", L"Resumen", WS_CHILD | WS_VISIBLE, 12, 142, 300, 18, state.window, 0);
    state.summaryEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 12, 128, 300, 200, state.window, IDC_SUMMARY_EDIT);
    state.newsLabel = createControl(state, 0, L"STATIC", L"Noticias", WS_CHILD | WS_VISIBLE, 330, 110, 200, 18, state.window, 0);
    state.newsList = createControl(state, WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 330, 128, 300, 200, state.window, IDC_NEWS_LIST);
    state.tableLabel = createControl(state, 0, L"STATIC", L"Tabla actual", WS_CHILD | WS_VISIBLE, 12, 350, 200, 18, state.window, 0);
    state.tableList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 12, 368, 300, 200, state.window, IDC_TABLE_LIST);
    state.squadLabel = createControl(state, 0, L"STATIC", L"Plantel", WS_CHILD | WS_VISIBLE, 330, 350, 200, 18, state.window, 0);
    state.squadList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 330, 368, 300, 200, state.window, IDC_SQUAD_LIST);
    state.transferLabel = createControl(state, 0, L"STATIC", L"Centro de transferencias", WS_CHILD | WS_VISIBLE, 12, 590, 300, 18, state.window, 0);
    state.transferList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 12, 608, 620, 120, state.window, IDC_TRANSFER_LIST);
    state.statusLabel = createControl(state, 0, L"STATIC", L"Listo.", WS_CHILD | WS_VISIBLE, 12, 740, 900, 18, state.window, 0);

    if (state.sectionFont) {
        SendMessageW(state.summaryLabel, WM_SETFONT, reinterpret_cast<WPARAM>(state.sectionFont), TRUE);
        SendMessageW(state.newsLabel, WM_SETFONT, reinterpret_cast<WPARAM>(state.sectionFont), TRUE);
        SendMessageW(state.tableLabel, WM_SETFONT, reinterpret_cast<WPARAM>(state.sectionFont), TRUE);
        SendMessageW(state.squadLabel, WM_SETFONT, reinterpret_cast<WPARAM>(state.sectionFont), TRUE);
        SendMessageW(state.transferLabel, WM_SETFONT, reinterpret_cast<WPARAM>(state.sectionFont), TRUE);
    }
    if (state.monoFont) {
        SendMessageW(state.summaryEdit, WM_SETFONT, reinterpret_cast<WPARAM>(state.monoFont), TRUE);
        SendMessageW(state.newsList, WM_SETFONT, reinterpret_cast<WPARAM>(state.monoFont), TRUE);
    }

    configureViews(state);
    layoutWindow(state);

    state.career.initializeLeague();
    fillDivisionCombo(state);
    fillTeamCombo(state, selectedDivisionId(state));
    refreshAll(state);
    setStatus(state, "Interfaz grafica lista.");
}

void startNewCareer(AppState& state) {
    if (state.career.divisions.empty()) {
        MessageBoxW(state.window, L"No se encontraron divisiones disponibles.", L"Football Manager", MB_OK | MB_ICONWARNING);
        return;
    }

    std::string divisionId = selectedDivisionId(state);
    if (divisionId.empty()) divisionId = state.career.divisions.front().id;
    std::string teamName = comboText(state.teamCombo);
    syncManagerNameFromUi(state);
    ServiceResult result = startCareerService(state.career, divisionId, teamName, state.career.managerName);
    if (!result.ok) {
        std::string message = result.messages.empty() ? "No se pudo iniciar la carrera." : result.messages.front();
        MessageBoxW(state.window, utf8ToWide(message).c_str(), L"Football Manager", MB_OK | MB_ICONWARNING);
    }

    state.summaryMode = SummaryMode::Overview;
    syncCombosFromCareer(state);
    refreshAll(state);
    setStatus(state, result.messages.empty() ? "Nueva carrera iniciada." : result.messages.back());
}

void loadCareer(AppState& state) {
    ServiceResult result = loadCareerService(state.career);
    if (!result.ok) {
        std::string message = result.messages.empty() ? "No se encontro una carrera guardada." : result.messages.front();
        MessageBoxW(state.window, utf8ToWide(message).c_str(), L"Football Manager", MB_OK | MB_ICONINFORMATION);
        setStatus(state, message);
        fillDivisionCombo(state);
        fillTeamCombo(state, selectedDivisionId(state));
        refreshAll(state);
        return;
    }

    state.summaryMode = SummaryMode::Overview;
    syncCombosFromCareer(state);
    refreshAll(state);
    setStatus(state, result.messages.empty() ? "Carrera cargada." : result.messages.back());
}

void saveCareer(AppState& state) {
    if (!state.career.myTeam) return;
    syncManagerNameFromUi(state);
    ServiceResult result = saveCareerService(state.career);
    refreshHeader(state);
    setStatus(state, result.messages.empty() ? "Carrera guardada." : result.messages.back());
}

void simulateWeek(AppState& state) {
    if (!state.career.myTeam) return;
    syncManagerNameFromUi(state);
    setStatus(state, "Simulando semana...");
    UpdateWindow(state.window);
    ServiceResult result = simulateCareerWeekService(state.career);
    syncCombosFromCareer(state);
    refreshAll(state);
    if (!result.messages.empty()) {
        setStatus(state, result.messages.back());
    } else {
        setStatus(state, "Semana simulada.");
    }
}

void validateSystem(AppState& state) {
    setStatus(state, "Ejecutando validacion...");
    UpdateWindow(state.window);
    ValidationSuiteSummary summary = runValidationService();
    if (summary.ok) {
        MessageBoxW(state.window, L"La suite de validacion termino sin fallas.", L"Football Manager", MB_OK | MB_ICONINFORMATION);
        setStatus(state, "Validacion completada sin fallas.");
    } else {
        MessageBoxW(state.window, L"La suite de validacion detecto fallas.", L"Football Manager", MB_OK | MB_ICONWARNING);
        setStatus(state, "Validacion completada con fallas.");
    }
}

void setSummaryMode(AppState& state, SummaryMode mode) {
    state.summaryMode = mode;
    refreshSummary(state);
    refreshHeader(state);
}

void showServiceMessages(AppState& state, const ServiceResult& result, const std::string& title) {
    if (result.messages.empty()) return;
    std::ostringstream out;
    for (size_t i = 0; i < result.messages.size(); ++i) {
        if (i) out << "\n";
        out << result.messages[i];
    }
    MessageBoxW(state.window,
                utf8ToWide(out.str()).c_str(),
                utf8ToWide(title).c_str(),
                MB_OK | (result.ok ? MB_ICONINFORMATION : MB_ICONWARNING));
}

void finalizeAction(AppState& state,
                    const ServiceResult& result,
                    SummaryMode successMode,
                    const std::string& title,
                    bool forceDialog = false) {
    if (result.ok) {
        state.summaryMode = successMode;
        syncCombosFromCareer(state);
        refreshAll(state);
    }
    if (!result.messages.empty()) {
        setStatus(state, result.messages.back());
    }
    if (!result.ok || forceDialog || result.messages.size() > 2) {
        showServiceMessages(state, result, title);
    }
}

void runScoutingAction(AppState& state) {
    ServiceResult result = scoutPlayersService(state.career, "Todas", "");
    finalizeAction(state, result, SummaryMode::Scouting, "Scouting", true);
}

void runBuyAction(AppState& state) {
    int row = selectedListViewRow(state.transferList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un objetivo del centro de transferencias.", L"Mercado", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::string type = listViewText(state.transferList, row, 0);
    if (type != "Mercado") {
        MessageBoxW(state.window, L"Solo puedes fichar filas marcadas como Mercado.", L"Mercado", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = buyTransferTargetService(state.career,
                                                    listViewText(state.transferList, row, 2),
                                                    listViewText(state.transferList, row, 1),
                                                    selectedNegotiationProfile(state),
                                                    selectedNegotiationPromise(state));
    finalizeAction(state, result, SummaryMode::Overview, "Fichaje");
}

void runPreContractAction(AppState& state) {
    int row = selectedListViewRow(state.transferList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un objetivo del centro de transferencias.", L"Precontrato", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::string type = listViewText(state.transferList, row, 0);
    if (type != "Mercado") {
        MessageBoxW(state.window, L"El precontrato se intenta sobre un objetivo de mercado.", L"Precontrato", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = signPreContractService(state.career,
                                                  listViewText(state.transferList, row, 2),
                                                  listViewText(state.transferList, row, 1),
                                                  selectedNegotiationProfile(state),
                                                  selectedNegotiationPromise(state));
    finalizeAction(state, result, SummaryMode::Overview, "Precontrato");
}

void runRenewAction(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un jugador del plantel.", L"Renovar", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = renewPlayerContractService(state.career,
                                                      listViewText(state.squadList, row, 3),
                                                      selectedNegotiationProfile(state),
                                                      selectedNegotiationPromise(state));
    finalizeAction(state, result, SummaryMode::Overview, "Renovacion");
}

void runSellAction(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un jugador del plantel.", L"Venta", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = sellPlayerService(state.career, listViewText(state.squadList, row, 3));
    finalizeAction(state, result, SummaryMode::Overview, "Venta");
}

void runUpgradeAction(AppState& state, ClubUpgrade upgrade, SummaryMode mode, const std::string& title) {
    ServiceResult result = upgradeClubService(state.career, upgrade);
    finalizeAction(state, result, mode, title);
}

void runPlanAction(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un jugador del plantel.", L"Plan individual", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = cyclePlayerDevelopmentPlanService(state.career, listViewText(state.squadList, row, 3));
    finalizeAction(state, result, SummaryMode::Club, "Plan individual");
}

void runInstructionAction(AppState& state) {
    ServiceResult result = cycleMatchInstructionService(state.career);
    finalizeAction(state, result, SummaryMode::Overview, "Instruccion");
}

void runShortlistAction(AppState& state) {
    int row = selectedListViewRow(state.transferList);
    if (row < 0) {
        MessageBoxW(state.window, L"Selecciona un objetivo del centro de transferencias.", L"Shortlist", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::string type = listViewText(state.transferList, row, 0);
    if (type != "Mercado") {
        MessageBoxW(state.window, L"Solo puedes seguir objetivos de mercado.", L"Shortlist", MB_OK | MB_ICONINFORMATION);
        return;
    }
    ServiceResult result = shortlistPlayerService(state.career,
                                                  listViewText(state.transferList, row, 2),
                                                  listViewText(state.transferList, row, 1));
    finalizeAction(state, result, SummaryMode::Scouting, "Shortlist");
}

void runFollowShortlistAction(AppState& state) {
    ServiceResult result = followShortlistService(state.career);
    finalizeAction(state, result, SummaryMode::Scouting, "Seguimiento", true);
}

void showSelectedSquadDetails(AppState& state) {
    int row = selectedListViewRow(state.squadList);
    if (row < 0 || !state.career.myTeam) return;
    std::string playerName = listViewText(state.squadList, row, 3);
    for (const auto& player : state.career.myTeam->players) {
        if (player.name != playerName) continue;
        std::ostringstream out;
        out << player.name
            << " | Hab " << player.skill
            << " | Pot " << player.potential
            << " | Contrato " << player.contractWeeks
            << " | Salario " << formatMoney(player.wage)
            << " | Clausula " << formatMoney(player.releaseClause)
            << " | Pie " << player.preferredFoot
            << " | Sec " << (player.secondaryPositions.empty() ? std::string("-") : joinStringValues(player.secondaryPositions, "/"))
            << " | Forma " << playerFormLabel(player) << " (" << player.currentForm << ")"
            << " | Fiabilidad " << playerReliabilityLabel(player)
            << " | Partidos grandes " << player.bigMatches
            << " | Plan " << player.developmentPlan
            << " | Promesa " << player.promisedRole
            << " | Fel " << player.happiness
            << " | Quim " << player.chemistry
            << " | Rasgos " << joinStringValues(player.traits, ", ");
        setStatus(state, out.str());
        return;
    }
}

void showSelectedTransferDetails(AppState& state) {
    int row = selectedListViewRow(state.transferList);
    if (row < 0) return;
    std::string type = listViewText(state.transferList, row, 0);
    std::string playerName = listViewText(state.transferList, row, 1);
    std::string clubName = listViewText(state.transferList, row, 2);
    std::string detail = listViewText(state.transferList, row, 3);
    std::string amount = listViewText(state.transferList, row, 4);
    if (type == "Mercado") {
        const Team* seller = state.career.findTeamByName(clubName);
        if (seller) {
            for (const auto& player : seller->players) {
                if (player.name != playerName) continue;
                setStatus(state,
                          type + " | " + player.name + " | " + seller->name +
                          " | " + detail + " | " + amount +
                          " | Rasgos " + joinStringValues(player.traits, ", "));
                return;
            }
        }
    }
    setStatus(state, type + " | " + playerName + " | " + clubName + " | " + detail + " | " + amount);
}

void drawThemedButton(AppState& state, const DRAWITEMSTRUCT* drawItem) {
    if (!drawItem) return;
    HDC hdc = drawItem->hDC;
    RECT rect = drawItem->rcItem;
    int id = static_cast<int>(drawItem->CtlID);
    bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
    bool disabled = (drawItem->itemState & ODS_DISABLED) != 0;
    bool active = isActiveSummaryButton(state, id);

    COLORREF fill = kThemePanelAlt;
    COLORREF border = RGB(44, 62, 76);
    COLORREF text = kThemeText;
    if (isPrimaryButtonId(id)) {
        fill = RGB(19, 63, 49);
        border = kThemeAccentGreen;
    } else if (isUpgradeButtonId(id)) {
        fill = RGB(54, 45, 20);
        border = kThemeAccent;
    } else if (id == IDC_VALIDATE_BUTTON) {
        fill = RGB(39, 48, 72);
        border = RGB(94, 122, 184);
    } else if (id == IDC_SAVE_BUTTON || id == IDC_LOAD_BUTTON) {
        fill = RGB(28, 43, 56);
    }
    if (active) {
        fill = RGB(72, 56, 18);
        border = kThemeAccent;
        text = RGB(255, 245, 214);
    }
    if (pressed) {
        fill = RGB(std::max(0, static_cast<int>(GetRValue(fill)) - 18),
                   std::max(0, static_cast<int>(GetGValue(fill)) - 18),
                   std::max(0, static_cast<int>(GetBValue(fill)) - 18));
    }
    if (disabled) {
        fill = RGB(30, 35, 40);
        border = RGB(52, 58, 64);
        text = RGB(102, 112, 118);
    }

    drawRoundedPanel(hdc, rect, fill, border, 12);

    RECT accentRect = rect;
    accentRect.bottom = accentRect.top + 4;
    HBRUSH accentBrush = CreateSolidBrush(active ? kThemeAccent : (isPrimaryButtonId(id) ? kThemeAccentGreen : border));
    FillRect(hdc, &accentRect, accentBrush);
    DeleteObject(accentBrush);

    wchar_t textBuffer[128]{};
    GetWindowTextW(drawItem->hwndItem, textBuffer, static_cast<int>(sizeof(textBuffer) / sizeof(textBuffer[0])));
    RECT textRect = rect;
    if (pressed) OffsetRect(&textRect, 0, 1);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, text);
    HFONT font = state.font ? state.font : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    DrawTextW(hdc, textBuffer, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);

    if ((drawItem->itemState & ODS_FOCUS) != 0) {
        RECT focusRect = rect;
        InflateRect(&focusRect, -6, -6);
        DrawFocusRect(hdc, &focusRect);
    }
}

LRESULT handleListCustomDraw(AppState& state, LPNMHDR header) {
    if (!header || header->code != NM_CUSTOMDRAW) return CDRF_DODEFAULT;
    if (header->idFrom != IDC_TABLE_LIST && header->idFrom != IDC_SQUAD_LIST && header->idFrom != IDC_TRANSFER_LIST) {
        return CDRF_DODEFAULT;
    }
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(header);
    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }
    if (draw->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) {
        return CDRF_DODEFAULT;
    }

    int row = static_cast<int>(draw->nmcd.dwItemSpec);
    COLORREF bg = (row % 2 == 0) ? kThemeInput : RGB(15, 25, 35);
    COLORREF text = kThemeText;

    if (header->idFrom == IDC_TABLE_LIST) {
        std::string teamName = listViewText(state.tableList, row, 1);
        if (teamName.find('*') != std::string::npos) {
            bg = RGB(24, 67, 50);
        } else if (row == 0) {
            bg = RGB(62, 49, 18);
            text = RGB(255, 239, 192);
        }
    } else if (header->idFrom == IDC_SQUAD_LIST) {
        std::string role = listViewText(state.squadList, row, 1);
        std::string status = listViewText(state.squadList, row, 10);
        if (role == "XI") {
            bg = RGB(21, 64, 48);
        } else if (status.find("Les") != std::string::npos || status.find("Salida") != std::string::npos) {
            bg = RGB(72, 31, 33);
        } else if (role == "Banco") {
            bg = RGB(23, 42, 60);
        }
    } else if (header->idFrom == IDC_TRANSFER_LIST) {
        std::string type = listViewText(state.transferList, row, 0);
        if (type == "Mercado") bg = RGB(19, 61, 47);
        else if (type == "Contrato" || type == "Precontrato") bg = RGB(64, 48, 19);
        else if (type == "Salida") bg = RGB(74, 32, 35);
        else if (type == "Prestamo" || type == "Cedido") bg = RGB(25, 43, 63);
    }

    if ((draw->nmcd.uItemState & CDIS_SELECTED) != 0) {
        bg = kThemeSelection;
        text = RGB(255, 255, 255);
    }

    draw->clrText = text;
    draw->clrTextBk = bg;
    return CDRF_NEWFONT;
}

void paintWindowChrome(AppState& state, HDC hdc) {
    RECT client{};
    GetClientRect(state.window, &client);
    FillRect(hdc, &client, state.backgroundBrush ? state.backgroundBrush : static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    RECT headerRect{0, 0, client.right, 196};
    FillRect(hdc, &headerRect, state.headerBrush ? state.headerBrush : state.backgroundBrush);

    RECT actionCard{8, 8, client.right - 8, 192};
    drawRoundedPanel(hdc, actionCard, RGB(12, 24, 32), RGB(32, 58, 54), 18);

    RECT titleRect{std::max(900, static_cast<int>(client.right) - 470), 14, client.right - 24, 66};
    RECT subtitleRect{titleRect.left, 70, titleRect.right, 96};
    RECT pitchRect{titleRect.left - 130, 18, titleRect.left - 12, 96};
    drawPitchOverlay(hdc, pitchRect);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kThemeAccent);
    HGDIOBJ oldFont = SelectObject(hdc, state.titleFont ? state.titleFont : state.font);
    DrawTextW(hdc, L"CAREER COMMAND", -1, &titleRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, state.sectionFont ? state.sectionFont : state.font);
    SetTextColor(hdc, kThemeMuted);
    DrawTextW(hdc, L"Football Manager // native game hub", -1, &subtitleRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    RECT infoCard = expandedRect(childRectOnParent(state.infoLabel, state.window), 8, 8);
    drawRoundedPanel(hdc, infoCard, RGB(13, 26, 35), RGB(41, 74, 88), 14);

    RECT summaryCard = expandedRect(childRectOnParent(state.summaryEdit, state.window), 8, 24);
    RECT newsCard = expandedRect(childRectOnParent(state.newsList, state.window), 8, 24);
    RECT tableCard = expandedRect(childRectOnParent(state.tableList, state.window), 8, 24);
    RECT squadCard = expandedRect(childRectOnParent(state.squadList, state.window), 8, 24);
    RECT transferCard = expandedRect(childRectOnParent(state.transferList, state.window), 8, 24);
    drawRoundedPanel(hdc, summaryCard, kThemePanel, RGB(44, 69, 84), 16);
    drawRoundedPanel(hdc, newsCard, kThemePanel, RGB(44, 69, 84), 16);
    drawRoundedPanel(hdc, tableCard, kThemePanel, RGB(44, 69, 84), 16);
    drawRoundedPanel(hdc, squadCard, kThemePanel, RGB(44, 69, 84), 16);
    drawRoundedPanel(hdc, transferCard, kThemePanel, RGB(44, 69, 84), 16);

    RECT statusCard = expandedRect(childRectOnParent(state.statusLabel, state.window), 8, 8);
    drawRoundedPanel(hdc, statusCard, RGB(12, 25, 34), RGB(46, 82, 98), 12);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
        case WM_NCCREATE: {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* incomingState = reinterpret_cast<AppState*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(incomingState));
            incomingState->window = hwnd;
            return TRUE;
        }
        case WM_CREATE:
            if (state) initializeInterface(*state);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            if (state) layoutWindow(*state);
            return 0;
        case WM_NOTIFY:
            if (!state) break;
            if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW) {
                return handleListCustomDraw(*state, reinterpret_cast<LPNMHDR>(lParam));
            }
            if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_SQUAD_LIST &&
                reinterpret_cast<LPNMHDR>(lParam)->code != LVN_COLUMNCLICK) {
                showSelectedSquadDetails(*state);
                return 0;
            }
            if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_TRANSFER_LIST &&
                reinterpret_cast<LPNMHDR>(lParam)->code != LVN_COLUMNCLICK) {
                showSelectedTransferDetails(*state);
                return 0;
            }
            break;
        case WM_DRAWITEM:
            if (state && wParam != 0) {
                drawThemedButton(*state, reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
                return TRUE;
            }
            break;
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSTATIC:
            if (state) {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                HWND control = reinterpret_cast<HWND>(lParam);
                SetBkMode(hdc, TRANSPARENT);
                if (message == WM_CTLCOLOREDIT || control == state->managerEdit || control == state->summaryEdit) {
                    SetBkMode(hdc, OPAQUE);
                    SetBkColor(hdc, kThemeInput);
                    SetTextColor(hdc, kThemeText);
                    return reinterpret_cast<LRESULT>(state->inputBrush ? state->inputBrush : state->panelBrush);
                }
                if (control == state->divisionCombo || control == state->teamCombo ||
                    control == state->negotiationCombo || control == state->promiseCombo) {
                    SetBkMode(hdc, OPAQUE);
                    SetBkColor(hdc, kThemeInput);
                    SetTextColor(hdc, kThemeText);
                    return reinterpret_cast<LRESULT>(state->inputBrush ? state->inputBrush : state->panelBrush);
                }
                if (message == WM_CTLCOLORLISTBOX || control == state->newsList) {
                    SetBkMode(hdc, OPAQUE);
                    SetBkColor(hdc, kThemeInput);
                    SetTextColor(hdc, kThemeText);
                    return reinterpret_cast<LRESULT>(state->inputBrush ? state->inputBrush : state->panelBrush);
                }
                if (control == state->summaryLabel || control == state->newsLabel || control == state->tableLabel ||
                    control == state->squadLabel || control == state->transferLabel) {
                    SetTextColor(hdc, kThemeAccent);
                } else if (control == state->statusLabel) {
                    SetTextColor(hdc, RGB(188, 228, 216));
                } else if (control == state->infoLabel) {
                    SetTextColor(hdc, kThemeText);
                } else {
                    SetTextColor(hdc, kThemeMuted);
                }
                return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
            }
            break;
        case WM_COMMAND:
            if (!state) break;
            switch (LOWORD(wParam)) {
                case IDC_DIVISION_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE && !state->suppressComboEvents) {
                        fillTeamCombo(*state, selectedDivisionId(*state));
                    }
                    return 0;
                case IDC_NEW_CAREER_BUTTON:
                    startNewCareer(*state);
                    return 0;
                case IDC_LOAD_BUTTON:
                    loadCareer(*state);
                    return 0;
                case IDC_SAVE_BUTTON:
                    saveCareer(*state);
                    return 0;
                case IDC_SIMULATE_BUTTON:
                    simulateWeek(*state);
                    return 0;
                case IDC_VALIDATE_BUTTON:
                    validateSystem(*state);
                    return 0;
                case IDC_VIEW_OVERVIEW_BUTTON:
                    setSummaryMode(*state, SummaryMode::Overview);
                    return 0;
                case IDC_VIEW_COMPETITION_BUTTON:
                    setSummaryMode(*state, SummaryMode::Competition);
                    return 0;
                case IDC_VIEW_BOARD_BUTTON:
                    setSummaryMode(*state, SummaryMode::Board);
                    return 0;
                case IDC_VIEW_CLUB_BUTTON:
                    setSummaryMode(*state, SummaryMode::Club);
                    return 0;
                case IDC_VIEW_SCOUTING_BUTTON:
                    setSummaryMode(*state, SummaryMode::Scouting);
                    return 0;
                case IDC_SCOUT_BUTTON:
                    runScoutingAction(*state);
                    return 0;
                case IDC_SHORTLIST_BUTTON:
                    runShortlistAction(*state);
                    return 0;
                case IDC_FOLLOW_SHORTLIST_BUTTON:
                    runFollowShortlistAction(*state);
                    return 0;
                case IDC_BUY_BUTTON:
                    runBuyAction(*state);
                    return 0;
                case IDC_PRECONTRACT_BUTTON:
                    runPreContractAction(*state);
                    return 0;
                case IDC_RENEW_BUTTON:
                    runRenewAction(*state);
                    return 0;
                case IDC_SELL_BUTTON:
                    runSellAction(*state);
                    return 0;
                case IDC_PLAN_BUTTON:
                    runPlanAction(*state);
                    return 0;
                case IDC_INSTRUCTION_BUTTON:
                    runInstructionAction(*state);
                    return 0;
                case IDC_YOUTH_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Youth, SummaryMode::Club, "Cantera");
                    return 0;
                case IDC_TRAINING_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Training, SummaryMode::Club, "Entrenamiento");
                    return 0;
                case IDC_SCOUTING_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Scouting, SummaryMode::Club, "Scouting");
                    return 0;
                case IDC_STADIUM_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Stadium, SummaryMode::Club, "Estadio");
                    return 0;
                default:
                    break;
            }
            break;
        case WM_PAINT:
            if (state) {
                PAINTSTRUCT paint{};
                HDC hdc = BeginPaint(hwnd, &paint);
                paintWindowChrome(*state, hdc);
                EndPaint(hwnd, &paint);
                return 0;
            }
            break;
        case WM_DESTROY:
            if (state) {
                if (state->font) DeleteObject(state->font);
                if (state->titleFont) DeleteObject(state->titleFont);
                if (state->sectionFont) DeleteObject(state->sectionFont);
                if (state->monoFont) DeleteObject(state->monoFont);
                if (state->backgroundBrush) DeleteObject(state->backgroundBrush);
                if (state->panelBrush) DeleteObject(state->panelBrush);
                if (state->headerBrush) DeleteObject(state->headerBrush);
                if (state->inputBrush) DeleteObject(state->inputBrush);
                state->font = nullptr;
                state->titleFont = nullptr;
                state->sectionFont = nullptr;
                state->monoFont = nullptr;
                state->backgroundBrush = nullptr;
                state->panelBrush = nullptr;
                state->headerBrush = nullptr;
                state->inputBrush = nullptr;
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int runGuiApp() {
    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controls);

    AppState state;
    state.instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = state.instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = L"FootballManagerGuiWindow";
    RegisterClassExW(&windowClass);

    HWND window = CreateWindowExW(0,
                                  windowClass.lpszClassName,
                                  L"Football Manager",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  1460,
                                  940,
                                  nullptr,
                                  nullptr,
                                  state.instance,
                                  &state);
    if (!window) {
        return 1;
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

#else

int runGuiApp() {
    return 1;
}

#endif
