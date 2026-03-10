#pragma once

#ifdef _WIN32

#include "career/app_services.h"
#include "career/career_reports.h"
#include "engine/models.h"

#include <string>
#include <utility>
#include <vector>

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

#include <windows.h>
#include <commctrl.h>

namespace gui_win32 {

enum ControlId {
    IDC_DIVISION_COMBO = 1001,
    IDC_TEAM_COMBO,
    IDC_MANAGER_EDIT,
    IDC_NEW_CAREER_BUTTON,
    IDC_LOAD_BUTTON,
    IDC_SAVE_BUTTON,
    IDC_SIMULATE_BUTTON,
    IDC_VALIDATE_BUTTON,
    IDC_FILTER_COMBO,
    IDC_SUMMARY_EDIT,
    IDC_NEWS_LIST,
    IDC_TABLE_LIST,
    IDC_SQUAD_LIST,
    IDC_TRANSFER_LIST,
    IDC_DETAIL_EDIT,
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
    IDC_STADIUM_UPGRADE_BUTTON,
    IDC_PAGE_DASHBOARD_BUTTON,
    IDC_PAGE_SQUAD_BUTTON,
    IDC_PAGE_TACTICS_BUTTON,
    IDC_PAGE_CALENDAR_BUTTON,
    IDC_PAGE_LEAGUE_BUTTON,
    IDC_PAGE_TRANSFERS_BUTTON,
    IDC_PAGE_FINANCES_BUTTON,
    IDC_PAGE_YOUTH_BUTTON,
    IDC_PAGE_BOARD_BUTTON,
    IDC_PAGE_NEWS_BUTTON
};

enum class GuiPage {
    Dashboard,
    Squad,
    Tactics,
    Calendar,
    League,
    Transfers,
    Finances,
    Youth,
    Board,
    News
};

struct SortState {
    int column = 0;
    bool ascending = true;
};

struct DashboardMetric {
    std::string label;
    std::string value;
    COLORREF accent = RGB(90, 120, 140);
};

struct TextPanelModel {
    std::string title;
    std::string content;
};

struct ListPanelModel {
    std::string title;
    std::vector<std::pair<std::wstring, int> > columns;
    std::vector<std::vector<std::string> > rows;
};

struct FeedPanelModel {
    std::string title;
    std::vector<std::string> lines;
};

struct GuiPageModel {
    std::string title;
    std::string breadcrumb;
    std::string infoLine;
    TextPanelModel summary;
    ListPanelModel primary;
    ListPanelModel secondary;
    ListPanelModel footer;
    TextPanelModel detail;
    FeedPanelModel feed;
    std::vector<DashboardMetric> metrics;
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
    bool suppressFilterEvents = false;
    GuiPage currentPage = GuiPage::Dashboard;
    std::string currentFilter = "Todo";
    SortState squadSort;
    std::string selectedPlayerName;
    std::string selectedTransferPlayer;
    std::string selectedTransferClub;
    GuiPageModel currentModel;

    HWND divisionCombo = nullptr;
    HWND teamCombo = nullptr;
    HWND managerEdit = nullptr;
    HWND filterLabel = nullptr;
    HWND filterCombo = nullptr;
    HWND newCareerButton = nullptr;
    HWND loadButton = nullptr;
    HWND saveButton = nullptr;
    HWND simulateButton = nullptr;
    HWND validateButton = nullptr;

    HWND dashboardButton = nullptr;
    HWND squadButton = nullptr;
    HWND tacticsButton = nullptr;
    HWND calendarButton = nullptr;
    HWND leagueButton = nullptr;
    HWND transfersButton = nullptr;
    HWND financesButton = nullptr;
    HWND youthButton = nullptr;
    HWND boardButton = nullptr;
    HWND newsButton = nullptr;

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

    HWND breadcrumbLabel = nullptr;
    HWND pageTitleLabel = nullptr;
    HWND infoLabel = nullptr;
    HWND summaryLabel = nullptr;
    HWND summaryEdit = nullptr;
    HWND tableLabel = nullptr;
    HWND tableList = nullptr;
    HWND squadLabel = nullptr;
    HWND squadList = nullptr;
    HWND transferLabel = nullptr;
    HWND transferList = nullptr;
    HWND detailLabel = nullptr;
    HWND detailEdit = nullptr;
    HWND newsLabel = nullptr;
    HWND newsList = nullptr;
    HWND statusLabel = nullptr;
};

static const COLORREF kThemeBg = RGB(7, 16, 22);
static const COLORREF kThemeHeader = RGB(10, 28, 35);
static const COLORREF kThemePanel = RGB(16, 28, 39);
static const COLORREF kThemePanelAlt = RGB(12, 24, 33);
static const COLORREF kThemeInput = RGB(11, 19, 27);
static const COLORREF kThemeAccent = RGB(226, 191, 92);
static const COLORREF kThemeAccentBlue = RGB(71, 126, 212);
static const COLORREF kThemeAccentGreen = RGB(46, 142, 96);
static const COLORREF kThemeText = RGB(234, 238, 241);
static const COLORREF kThemeMuted = RGB(145, 163, 176);
static const COLORREF kThemeDanger = RGB(186, 78, 78);
static const COLORREF kThemeWarning = RGB(208, 167, 72);
static const COLORREF kThemeSelection = RGB(64, 91, 109);

RECT childRectOnParent(HWND child, HWND parent);
RECT expandedRect(RECT rect, int dx, int dy);
std::wstring utf8ToWide(const std::string& text);
std::string wideToUtf8(const std::wstring& text);
std::string toEditText(const std::string& text);
void setWindowTextUtf8(HWND hwnd, const std::string& text);
std::string getWindowTextUtf8(HWND hwnd);
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
                   int id);
void addComboItem(HWND combo, const std::string& text);
void resetComboItems(HWND combo, const std::vector<std::string>& items);
int comboIndex(HWND combo);
std::string comboText(HWND combo);
int selectedListViewRow(HWND list);
std::string listViewText(HWND list, int row, int subItem);
void clearListView(HWND list);
void addListViewRow(HWND list, const std::vector<std::string>& values);
void resetListViewColumns(HWND list, const std::vector<std::pair<std::wstring, int> >& columns);
void drawRoundedPanel(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius = 16);
void drawPitchOverlay(HDC hdc, const RECT& rect);
void drawSectionHeader(HDC hdc, const RECT& rect, const std::wstring& title);
void drawStatBar(HDC hdc, const RECT& rect, const std::wstring& label, int value, int maxValue, COLORREF accent);

GuiPage pageForControlId(int id);
bool isPageButtonId(int id);
bool isPrimaryButtonId(int id);
bool isUpgradeButtonId(int id);
bool isActionButtonId(int id);

void initializeInterface(AppState& state);
void layoutWindow(AppState& state);
void paintWindowChrome(AppState& state, HDC hdc);
void drawThemedButton(AppState& state, const DRAWITEMSTRUCT* drawItem);
LRESULT handleListCustomDraw(AppState& state, LPNMHDR header);

std::string selectedDivisionId(const AppState& state);
void fillDivisionCombo(AppState& state, const std::string& selectedId = std::string());
void fillTeamCombo(AppState& state, const std::string& divisionId, const std::string& selectedTeam = std::string());
void syncCombosFromCareer(AppState& state);
void syncManagerNameFromUi(AppState& state);
void setStatus(AppState& state, const std::string& text);
void setCurrentPage(AppState& state, GuiPage page);
void refreshAll(AppState& state);
void refreshCurrentPage(AppState& state);
void handleFilterChange(AppState& state);
void handleListSelectionChange(AppState& state, int controlId);
void handleListColumnClick(AppState& state, const NMLISTVIEW& view);

void startNewCareer(AppState& state);
void loadCareer(AppState& state);
void saveCareer(AppState& state);
void simulateWeek(AppState& state);
void validateSystem(AppState& state);
void runScoutingAction(AppState& state);
void runBuyAction(AppState& state);
void runPreContractAction(AppState& state);
void runRenewAction(AppState& state);
void runSellAction(AppState& state);
void runPlanAction(AppState& state);
void runInstructionAction(AppState& state);
void runShortlistAction(AppState& state);
void runFollowShortlistAction(AppState& state);
void runUpgradeAction(AppState& state, ClubUpgrade upgrade, const std::string& title);

}  // namespace gui_win32

#endif
