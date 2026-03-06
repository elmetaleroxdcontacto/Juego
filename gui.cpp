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
    IDC_SQUAD_LIST
};

struct AppState {
    HINSTANCE instance = nullptr;
    HWND window = nullptr;
    HFONT font = nullptr;

    Career career;
    bool suppressComboEvents = false;

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
    HWND summaryEdit = nullptr;
    HWND newsLabel = nullptr;
    HWND newsList = nullptr;
    HWND tableLabel = nullptr;
    HWND tableList = nullptr;
    HWND squadLabel = nullptr;
    HWND squadList = nullptr;
};

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

    if (!hasCareer) {
        setWindowTextUtf8(state.infoLabel, "Selecciona division, equipo y manager para crear una carrera o cargar un guardado.");
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
}

void refreshSummary(AppState& state) {
    if (!state.career.myTeam) {
        setWindowTextUtf8(state.summaryEdit, "Crea una nueva carrera o carga una partida guardada para empezar.");
        return;
    }

    Team& team = *state.career.myTeam;
    LeagueTable table = relevantTable(state.career);

    int injured = 0;
    int suspended = 0;
    int totalFitness = 0;
    for (const auto& player : team.players) {
        if (player.injured) injured++;
        if (player.matchesSuspended > 0) suspended++;
        totalFitness += player.fitness;
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
        << " | Rotacion: " << team.rotationPolicy << "\r\n";
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

std::string playerRole(const Team& team, int index, const std::vector<int>& xi, const std::vector<int>& bench) {
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
                           std::to_string(player.goals),
                           std::to_string(player.assists),
                           playerStatus(player)
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
                                 {L"G", 45},
                                 {L"A", 45},
                                 {L"Estado", 200}
                             });

    DWORD styles = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
    ListView_SetExtendedListViewStyle(state.tableList, styles);
    ListView_SetExtendedListViewStyle(state.squadList, styles);
}

void layoutWindow(AppState& state) {
    RECT client{};
    GetClientRect(state.window, &client);

    const int padding = 12;
    const int rowHeight = 26;

    MoveWindow(state.divisionCombo, 78, 12, 190, rowHeight, TRUE);
    MoveWindow(state.teamCombo, 332, 12, 240, rowHeight, TRUE);
    MoveWindow(state.managerEdit, 656, 12, 210, 24, TRUE);

    MoveWindow(state.newCareerButton, 12, 46, 120, rowHeight, TRUE);
    MoveWindow(state.loadButton, 140, 46, 100, rowHeight, TRUE);
    MoveWindow(state.saveButton, 248, 46, 100, rowHeight, TRUE);
    MoveWindow(state.simulateButton, 356, 46, 150, rowHeight, TRUE);
    MoveWindow(state.validateButton, 514, 46, 120, rowHeight, TRUE);

    MoveWindow(state.infoLabel, padding, 80, std::max(240, static_cast<int>(client.right) - padding * 2), 22, TRUE);

    int contentTop = 110;
    int contentBottom = client.bottom - 36;
    int contentHeight = std::max(220, contentBottom - contentTop);
    int halfWidth = std::max(280, (static_cast<int>(client.right) - padding * 3) / 2);
    int topHeight = std::max(180, contentHeight * 48 / 100);
    int bottomTop = contentTop + topHeight + 30;
    int bottomHeight = std::max(150, contentBottom - bottomTop);

    MoveWindow(state.summaryEdit, padding, contentTop + 18, halfWidth, topHeight, TRUE);
    MoveWindow(state.newsLabel, padding * 2 + halfWidth, contentTop, halfWidth, 18, TRUE);
    MoveWindow(state.newsList, padding * 2 + halfWidth, contentTop + 18, halfWidth, topHeight, TRUE);

    MoveWindow(state.tableLabel, padding, bottomTop, halfWidth, 18, TRUE);
    MoveWindow(state.tableList, padding, bottomTop + 18, halfWidth, bottomHeight, TRUE);

    MoveWindow(state.squadLabel, padding * 2 + halfWidth, bottomTop, halfWidth, 18, TRUE);
    MoveWindow(state.squadList, padding * 2 + halfWidth, bottomTop + 18, halfWidth, bottomHeight, TRUE);

    MoveWindow(state.statusLabel, padding, client.bottom - 24, std::max(240, static_cast<int>(client.right) - padding * 2), 18, TRUE);
}

void initializeInterface(AppState& state) {
    state.font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    createControl(state, 0, L"STATIC", L"Division", WS_CHILD | WS_VISIBLE, 12, 16, 60, 20, state.window, 0);
    createControl(state, 0, L"STATIC", L"Equipo", WS_CHILD | WS_VISIBLE, 280, 16, 50, 20, state.window, 0);
    createControl(state, 0, L"STATIC", L"Manager", WS_CHILD | WS_VISIBLE, 586, 16, 60, 20, state.window, 0);

    state.divisionCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 78, 12, 190, 300, state.window, IDC_DIVISION_COMBO);
    state.teamCombo = createControl(state, 0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 332, 12, 240, 300, state.window, IDC_TEAM_COMBO);
    state.managerEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"Manager", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 656, 12, 210, 24, state.window, IDC_MANAGER_EDIT);

    state.newCareerButton = createControl(state, 0, L"BUTTON", L"Nueva carrera", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 12, 46, 120, 26, state.window, IDC_NEW_CAREER_BUTTON);
    state.loadButton = createControl(state, 0, L"BUTTON", L"Cargar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 140, 46, 100, 26, state.window, IDC_LOAD_BUTTON);
    state.saveButton = createControl(state, 0, L"BUTTON", L"Guardar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 248, 46, 100, 26, state.window, IDC_SAVE_BUTTON);
    state.simulateButton = createControl(state, 0, L"BUTTON", L"Simular semana", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 356, 46, 150, 26, state.window, IDC_SIMULATE_BUTTON);
    state.validateButton = createControl(state, 0, L"BUTTON", L"Validar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 514, 46, 120, 26, state.window, IDC_VALIDATE_BUTTON);

    state.infoLabel = createControl(state, 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 12, 80, 900, 22, state.window, 0);
    state.summaryEdit = createControl(state, WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 12, 128, 300, 200, state.window, IDC_SUMMARY_EDIT);
    state.newsLabel = createControl(state, 0, L"STATIC", L"Noticias", WS_CHILD | WS_VISIBLE, 330, 110, 200, 18, state.window, 0);
    state.newsList = createControl(state, WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 330, 128, 300, 200, state.window, IDC_NEWS_LIST);
    state.tableLabel = createControl(state, 0, L"STATIC", L"Tabla actual", WS_CHILD | WS_VISIBLE, 12, 350, 200, 18, state.window, 0);
    state.tableList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 12, 368, 300, 200, state.window, IDC_TABLE_LIST);
    state.squadLabel = createControl(state, 0, L"STATIC", L"Plantel", WS_CHILD | WS_VISIBLE, 330, 350, 200, 18, state.window, 0);
    state.squadList = createControl(state, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 330, 368, 300, 200, state.window, IDC_SQUAD_LIST);
    state.statusLabel = createControl(state, 0, L"STATIC", L"Listo.", WS_CHILD | WS_VISIBLE, 12, 740, 900, 18, state.window, 0);

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
        case WM_SIZE:
            if (state) layoutWindow(*state);
            return 0;
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
                default:
                    break;
            }
            break;
        case WM_DESTROY:
            if (state && state->font) {
                DeleteObject(state->font);
                state->font = nullptr;
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
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = L"FootballManagerGuiWindow";
    RegisterClassExW(&windowClass);

    HWND window = CreateWindowExW(0,
                                  windowClass.lpszClassName,
                                  L"Football Manager",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  1320,
                                  860,
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
