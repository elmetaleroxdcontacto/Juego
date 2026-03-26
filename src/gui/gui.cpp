#include "gui/gui.h"
#include "gui/gui_internal.h"

#ifdef _WIN32

namespace gui_win32 {

namespace {

using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
using SetProcessDpiAwareFn = BOOL(WINAPI*)();

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

void enableHighDpiSupport() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto setDpiContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiContext && setDpiContext(reinterpret_cast<HANDLE>(-4))) {
            return;
        }
        auto setDpiAware = reinterpret_cast<SetProcessDpiAwareFn>(GetProcAddress(user32, "SetProcessDPIAware"));
        if (setDpiAware) {
            setDpiAware();
        }
    }
}

UINT queryWindowDpi(HWND hwnd) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"));
        if (getDpiForWindow) {
            UINT dpi = getDpiForWindow(hwnd);
            if (dpi != 0) return dpi;
        }
    }

    HDC hdc = GetDC(hwnd ? hwnd : nullptr);
    UINT dpi = hdc ? static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX)) : 96;
    if (hdc) ReleaseDC(hwnd ? hwnd : nullptr, hdc);
    return dpi == 0 ? 96 : dpi;
}

RECT primaryMonitorWorkArea() {
    RECT rect{0, 0, 1600, 900};
    POINT origin{0, 0};
    HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (monitor && GetMonitorInfoW(monitor, &info)) {
        rect = info.rcWork;
    }
    return rect;
}

DisplayMode currentDisplayMode(const AppState& state) {
    if (state.isBorderlessFullscreen) return DisplayMode::BorderlessFullscreen;
    if (state.window && IsZoomed(state.window)) return DisplayMode::MaximizedWindow;
    return DisplayMode::RestoredWindow;
}

std::string displayModeButtonText(const AppState& state) {
    switch (currentDisplayMode(state)) {
        case DisplayMode::RestoredWindow:
            return "Maximizar F11";
        case DisplayMode::MaximizedWindow:
            return "Pantalla F11";
        case DisplayMode::BorderlessFullscreen:
            return "Ventana F11";
    }
    return "Pantalla F11";
}

void syncDisplayModeButton(AppState& state) {
    if (!state.displayModeButton) return;
    setWindowTextUtf8(state.displayModeButton, displayModeButtonText(state));
    InvalidateRect(state.displayModeButton, nullptr, TRUE);
}

void openPageWithFilter(AppState& state, GuiPage page, const std::string& filter) {
    state.currentPage = page;
    state.currentFilter = filter;
    refreshCurrentPage(state);
}

bool restoreWindowedPlacement(AppState& state) {
    if (!state.window) return false;

    if (state.restoreStyle != 0) {
        SetWindowLongPtrW(state.window, GWL_STYLE, static_cast<LONG_PTR>(state.restoreStyle));
        SetWindowLongPtrW(state.window, GWL_EXSTYLE, static_cast<LONG_PTR>(state.restoreExStyle));
    }

    WINDOWPLACEMENT placement = state.restorePlacement;
    placement.length = sizeof(WINDOWPLACEMENT);
    if (placement.rcNormalPosition.left == 0 && placement.rcNormalPosition.right == 0) {
        GetWindowPlacement(state.window, &placement);
    }
    placement.flags = 0;
    placement.showCmd = SW_SHOWNORMAL;
    SetWindowPlacement(state.window, &placement);
    SetWindowPos(state.window,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    ShowWindow(state.window, SW_RESTORE);
    state.isBorderlessFullscreen = false;
    syncDisplayModeButton(state);
    setStatus(state, "Ventana restaurada. El siguiente paso del ciclo es maximizar y luego fullscreen.");
    return true;
}

bool enterBorderlessFullscreen(AppState& state) {
    if (!state.window) return false;

    state.restoreStyle = static_cast<DWORD>(GetWindowLongPtrW(state.window, GWL_STYLE));
    state.restoreExStyle = static_cast<DWORD>(GetWindowLongPtrW(state.window, GWL_EXSTYLE));
    state.restorePlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(state.window, &state.restorePlacement);

    HMONITOR monitor = MonitorFromWindow(state.window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    RECT target = primaryMonitorWorkArea();
    if (monitor && GetMonitorInfoW(monitor, &info)) {
        target = info.rcMonitor;
    }

    SetWindowLongPtrW(state.window,
                      GWL_STYLE,
                      static_cast<LONG_PTR>((state.restoreStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP | WS_VISIBLE));
    SetWindowLongPtrW(state.window, GWL_EXSTYLE, static_cast<LONG_PTR>(state.restoreExStyle));
    SetWindowPos(state.window,
                 HWND_TOP,
                 target.left,
                 target.top,
                 target.right - target.left,
                 target.bottom - target.top,
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    state.isBorderlessFullscreen = true;
    syncDisplayModeButton(state);
    setStatus(state, "Pantalla completa sin borde activa. El siguiente paso del ciclo devuelve la ventana restaurada.");
    return true;
}

void maximizeWindow(AppState& state) {
    if (!state.window) return;
    ShowWindow(state.window, SW_MAXIMIZE);
    state.isBorderlessFullscreen = false;
    syncDisplayModeButton(state);
    setStatus(state, "Ventana maximizada. Usa Pantalla o F11 para pasar al fullscreen sin borde.");
}

const InsightHotspot* hitInsightHotspot(const AppState& state, POINT point) {
    for (const auto& hotspot : state.insightHotspots) {
        if (hotspot.action != InsightAction::None && PtInRect(&hotspot.rect, point)) {
            return &hotspot;
        }
    }
    return nullptr;
}

void executeInsightAction(AppState& state, InsightAction action) {
    switch (action) {
        case InsightAction::FocusDivision:
            SetFocus(state.divisionCombo);
            setStatus(state, "Insight: elige la division para definir el universo de la partida.");
            return;
        case InsightAction::FocusClub:
            SetFocus(state.teamCombo);
            setStatus(state, "Insight: elige el club para fijar proyecto, presupuesto y plantilla inicial.");
            return;
        case InsightAction::StartCareer:
            startNewCareer(state);
            return;
        case InsightAction::OpenLeague:
            openPageWithFilter(state, GuiPage::League, "Grupo actual");
            setStatus(state, "Insight: se abrio la vista de liga para revisar la carrera competitiva.");
            return;
        case InsightAction::OpenSquad:
            openPageWithFilter(state, GuiPage::Squad, "Todos");
            setStatus(state, "Insight: se abrio la plantilla para revisar forma, moral y salud.");
            return;
        case InsightAction::OpenBoard:
            openPageWithFilter(state, GuiPage::Board, "Resumen");
            setStatus(state, "Insight: se abrio la directiva para revisar confianza y objetivos.");
            return;
        case InsightAction::RefreshMarketPulse:
            runFollowShortlistAction(state);
            return;
        case InsightAction::RunScouting:
            runScoutingAction(state);
            return;
        case InsightAction::OpenFinanceSummary:
            openPageWithFilter(state, GuiPage::Finances, "Resumen");
            setStatus(state, "Insight: se abrio el resumen financiero del club.");
            return;
        case InsightAction::OpenFinanceSalaries:
            openPageWithFilter(state, GuiPage::Finances, "Salarios");
            setStatus(state, "Insight: se abrio la vista salarial para revisar contratos y masa salarial.");
            return;
        case InsightAction::OpenFinanceInfrastructure:
            openPageWithFilter(state, GuiPage::Finances, "Infraestructura");
            setStatus(state, "Insight: se abrio infraestructura para revisar capacidad de crecimiento.");
            return;
        case InsightAction::OpenBoardSummary:
            openPageWithFilter(state, GuiPage::Board, "Resumen");
            setStatus(state, "Insight: se abrio el resumen de la directiva.");
            return;
        case InsightAction::OpenBoardObjectives:
            openPageWithFilter(state, GuiPage::Board, "Objetivos");
            setStatus(state, "Insight: se abrio el seguimiento de objetivos mensuales.");
            return;
        case InsightAction::OpenBoardHistory:
            openPageWithFilter(state, GuiPage::Board, "Historial");
            setStatus(state, "Insight: se abrio el historial para evaluar la presion del cargo.");
            return;
        case InsightAction::None:
            return;
    }
}

}  // namespace

void cycleDisplayMode(AppState& state) {
    switch (currentDisplayMode(state)) {
        case DisplayMode::RestoredWindow:
            maximizeWindow(state);
            return;
        case DisplayMode::MaximizedWindow:
            enterBorderlessFullscreen(state);
            return;
        case DisplayMode::BorderlessFullscreen:
            restoreWindowedPlacement(state);
            return;
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
            if (state) {
                state->dpi = queryWindowDpi(hwnd);
                initializeInterface(*state);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            if (state) {
                layoutWindow(*state);
                autosizeCurrentLists(*state);
                syncDisplayModeButton(*state);
            }
            return 0;
        case WM_DPICHANGED:
            if (state) {
                state->dpi = HIWORD(wParam) ? HIWORD(wParam) : queryWindowDpi(hwnd);
                rebuildFonts(*state);
                applyInterfaceFonts(*state);
                auto* suggested = reinterpret_cast<const RECT*>(lParam);
                if (suggested) {
                    SetWindowPos(hwnd,
                                 nullptr,
                                 suggested->left,
                                 suggested->top,
                                 suggested->right - suggested->left,
                                 suggested->bottom - suggested->top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                }
                layoutWindow(*state);
                autosizeCurrentLists(*state);
                syncDisplayModeButton(*state);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        case WM_SETCURSOR:
            if (state && LOWORD(lParam) == HTCLIENT) {
                POINT cursor{};
                GetCursorPos(&cursor);
                ScreenToClient(hwnd, &cursor);
                if (hitInsightHotspot(*state, cursor)) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
            }
            break;
        case WM_LBUTTONUP:
            if (state) {
                POINT point{static_cast<LONG>(static_cast<short>(LOWORD(lParam))),
                            static_cast<LONG>(static_cast<short>(HIWORD(lParam)))};
                if (const InsightHotspot* hotspot = hitInsightHotspot(*state, point)) {
                    executeInsightAction(*state, hotspot->action);
                    return 0;
                }
            }
            break;
        case WM_NOTIFY:
            if (!state) break;
            if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW) {
                return handleListCustomDraw(*state, reinterpret_cast<LPNMHDR>(lParam));
            }
            if (reinterpret_cast<LPNMHDR>(lParam)->code == LVN_COLUMNCLICK) {
                handleListColumnClick(*state, *reinterpret_cast<NMLISTVIEW*>(lParam));
                return 0;
            }
            if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_SQUAD_LIST ||
                reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_TABLE_LIST ||
                reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_TRANSFER_LIST) {
                if (reinterpret_cast<LPNMHDR>(lParam)->code == LVN_ITEMCHANGED ||
                    reinterpret_cast<LPNMHDR>(lParam)->code == NM_CLICK) {
                    handleListSelectionChange(*state, static_cast<int>(reinterpret_cast<LPNMHDR>(lParam)->idFrom));
                    return 0;
                }
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
                if (message == WM_CTLCOLOREDIT || control == state->managerEdit || control == state->summaryEdit || control == state->detailEdit) {
                    SetBkMode(hdc, OPAQUE);
                    SetBkColor(hdc, kThemeInput);
                    SetTextColor(hdc, kThemeText);
                    return reinterpret_cast<LRESULT>(state->inputBrush ? state->inputBrush : state->panelBrush);
                }
                if (control == state->divisionCombo || control == state->teamCombo || control == state->filterCombo) {
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
                if (control == state->summaryLabel || control == state->tableLabel || control == state->squadLabel ||
                    control == state->transferLabel || control == state->detailLabel || control == state->newsLabel) {
                    SetTextColor(hdc, kThemeAccent);
                } else if (control == state->pageTitleLabel) {
                    SetTextColor(hdc, RGB(242, 247, 249));
                } else if (control == state->breadcrumbLabel) {
                    SetTextColor(hdc, kThemeMuted);
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
                case IDC_FILTER_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE && !state->suppressFilterEvents) {
                        handleFilterChange(*state);
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
                case IDC_DISPLAY_MODE_BUTTON:
                    cycleDisplayMode(*state);
                    return 0;
                case IDC_EMPTY_NEW_BUTTON:
                    startNewCareer(*state);
                    return 0;
                case IDC_EMPTY_LOAD_BUTTON:
                    loadCareer(*state);
                    return 0;
                case IDC_EMPTY_VALIDATE_BUTTON:
                    validateSystem(*state);
                    return 0;
                case IDC_PAGE_DASHBOARD_BUTTON:
                case IDC_PAGE_SQUAD_BUTTON:
                case IDC_PAGE_TACTICS_BUTTON:
                case IDC_PAGE_CALENDAR_BUTTON:
                case IDC_PAGE_LEAGUE_BUTTON:
                case IDC_PAGE_TRANSFERS_BUTTON:
                case IDC_PAGE_FINANCES_BUTTON:
                case IDC_PAGE_YOUTH_BUTTON:
                case IDC_PAGE_BOARD_BUTTON:
                case IDC_PAGE_NEWS_BUTTON:
                    setCurrentPage(*state, pageForControlId(static_cast<int>(LOWORD(wParam))));
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
                    runUpgradeAction(*state, ClubUpgrade::Youth, "Cantera");
                    return 0;
                case IDC_TRAINING_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Training, "Entrenamiento");
                    return 0;
                case IDC_SCOUTING_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Scouting, "Scouting");
                    return 0;
                case IDC_STADIUM_UPGRADE_BUTTON:
                    runUpgradeAction(*state, ClubUpgrade::Stadium, "Estadio");
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

}  // namespace gui_win32

int runGuiApp() {
    gui_win32::enableHighDpiSupport();

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controls);

    gui_win32::AppState state;
    state.instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = gui_win32::windowProc;
    windowClass.hInstance = state.instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = L"FootballManagerGuiWindow";
    RegisterClassExW(&windowClass);

    RECT workArea = gui_win32::primaryMonitorWorkArea();
    HWND window = CreateWindowExW(0,
                                  windowClass.lpszClassName,
                                  L"Football Manager",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE,
                                  workArea.left,
                                  workArea.top,
                                  workArea.right - workArea.left,
                                  workArea.bottom - workArea.top,
                                  nullptr,
                                  nullptr,
                                  state.instance,
                                  &state);
    if (!window) {
        return 1;
    }

    ShowWindow(window, SW_MAXIMIZE);
    UpdateWindow(window);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F11 && (msg.lParam & (1u << 30)) == 0) {
            gui_win32::cycleDisplayMode(state);
            continue;
        }
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
