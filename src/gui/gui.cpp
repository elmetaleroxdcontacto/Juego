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

}  // namespace

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
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
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
