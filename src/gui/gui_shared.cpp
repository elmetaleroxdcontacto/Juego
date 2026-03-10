#include "gui/gui_internal.h"

#ifdef _WIN32

#include "utils/utils.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace gui_win32 {

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

void resetComboItems(HWND combo, const std::vector<std::string>& items) {
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    for (const auto& item : items) {
        addComboItem(combo, item);
    }
    if (!items.empty()) {
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
    }
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

void resetListViewColumns(HWND list, const std::vector<std::pair<std::wstring, int> >& columns) {
    HWND header = ListView_GetHeader(list);
    if (header) {
        int count = Header_GetItemCount(header);
        for (int i = count - 1; i >= 0; --i) {
            ListView_DeleteColumn(list, i);
        }
    }
    for (size_t i = 0; i < columns.size(); ++i) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<LPWSTR>(columns[i].first.c_str());
        column.cx = columns[i].second;
        column.iSubItem = static_cast<int>(i);
        SendMessageW(list, LVM_INSERTCOLUMNW, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(&column));
    }
}

void drawRoundedPanel(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius) {
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
    int midY = (rect.top + rect.bottom) / 2;
    MoveToEx(hdc, midX, rect.top, nullptr);
    LineTo(hdc, midX, rect.bottom);
    int radius = std::min((rect.right - rect.left) / 8, (rect.bottom - rect.top) / 4);
    Ellipse(hdc, midX - radius, midY - radius, midX + radius, midY + radius);

    int boxWidth = (rect.right - rect.left) / 6;
    int boxHeight = (rect.bottom - rect.top) / 3;
    Rectangle(hdc, rect.left, midY - boxHeight / 2, rect.left + boxWidth, midY + boxHeight / 2);
    Rectangle(hdc, rect.right - boxWidth, midY - boxHeight / 2, rect.right, midY + boxHeight / 2);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void drawSectionHeader(HDC hdc, const RECT& rect, const std::wstring& title) {
    RECT lineRect = rect;
    lineRect.top += 14;
    lineRect.left += 110;
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(52, 78, 98));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, lineRect.left, lineRect.top, nullptr);
    LineTo(hdc, lineRect.right, lineRect.top);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kThemeAccent);
    DrawTextW(hdc, title.c_str(), -1, const_cast<RECT*>(&rect), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void drawStatBar(HDC hdc, const RECT& rect, const std::wstring& label, int value, int maxValue, COLORREF accent) {
    int safeMax = std::max(1, maxValue);
    int pct = clampValue(value * 100 / safeMax, 0, 100);

    RECT track = rect;
    track.top += 18;
    drawRoundedPanel(hdc, track, RGB(20, 33, 44), RGB(42, 63, 79), 10);

    RECT fill = track;
    fill.right = fill.left + (track.right - track.left) * pct / 100;
    fill.left += 1;
    fill.top += 1;
    fill.bottom -= 1;
    fill.right = std::max(fill.left + 4, fill.right - 1);
    drawRoundedPanel(hdc, fill, accent, accent, 8);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kThemeText);
    DrawTextW(hdc, label.c_str(), -1, const_cast<RECT*>(&rect), DT_LEFT | DT_TOP | DT_SINGLELINE);
    std::wostringstream valueStream;
    valueStream << value;
    std::wstring valueText = valueStream.str();
    DrawTextW(hdc, valueText.c_str(), -1, const_cast<RECT*>(&rect), DT_RIGHT | DT_TOP | DT_SINGLELINE);
}

GuiPage pageForControlId(int id) {
    switch (id) {
        case IDC_PAGE_DASHBOARD_BUTTON: return GuiPage::Dashboard;
        case IDC_PAGE_SQUAD_BUTTON: return GuiPage::Squad;
        case IDC_PAGE_TACTICS_BUTTON: return GuiPage::Tactics;
        case IDC_PAGE_CALENDAR_BUTTON: return GuiPage::Calendar;
        case IDC_PAGE_LEAGUE_BUTTON: return GuiPage::League;
        case IDC_PAGE_TRANSFERS_BUTTON: return GuiPage::Transfers;
        case IDC_PAGE_FINANCES_BUTTON: return GuiPage::Finances;
        case IDC_PAGE_YOUTH_BUTTON: return GuiPage::Youth;
        case IDC_PAGE_BOARD_BUTTON: return GuiPage::Board;
        case IDC_PAGE_NEWS_BUTTON: return GuiPage::News;
        default: return GuiPage::Dashboard;
    }
}

bool isPageButtonId(int id) {
    return id >= IDC_PAGE_DASHBOARD_BUTTON && id <= IDC_PAGE_NEWS_BUTTON;
}

bool isPrimaryButtonId(int id) {
    return id == IDC_NEW_CAREER_BUTTON || id == IDC_SIMULATE_BUTTON || id == IDC_BUY_BUTTON ||
           id == IDC_RENEW_BUTTON || id == IDC_SCOUT_BUTTON || id == IDC_PAGE_DASHBOARD_BUTTON;
}

bool isUpgradeButtonId(int id) {
    return id == IDC_YOUTH_UPGRADE_BUTTON || id == IDC_TRAINING_UPGRADE_BUTTON ||
           id == IDC_SCOUTING_UPGRADE_BUTTON || id == IDC_STADIUM_UPGRADE_BUTTON;
}

bool isActionButtonId(int id) {
    return id == IDC_SCOUT_BUTTON || id == IDC_SHORTLIST_BUTTON || id == IDC_FOLLOW_SHORTLIST_BUTTON ||
           id == IDC_BUY_BUTTON || id == IDC_PRECONTRACT_BUTTON || id == IDC_RENEW_BUTTON ||
           id == IDC_SELL_BUTTON || id == IDC_PLAN_BUTTON || id == IDC_INSTRUCTION_BUTTON ||
           id == IDC_YOUTH_UPGRADE_BUTTON || id == IDC_TRAINING_UPGRADE_BUTTON ||
           id == IDC_SCOUTING_UPGRADE_BUTTON || id == IDC_STADIUM_UPGRADE_BUTTON;
}

static bool isActivePageButton(const AppState& state, int id) {
    return isPageButtonId(id) && pageForControlId(id) == state.currentPage;
}

static bool titleMatches(const std::string& value, const char* expected) {
    return value == expected;
}

void drawThemedButton(AppState& state, const DRAWITEMSTRUCT* drawItem) {
    if (!drawItem) return;
    HDC hdc = drawItem->hDC;
    RECT rect = drawItem->rcItem;
    int id = static_cast<int>(drawItem->CtlID);
    bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
    bool disabled = (drawItem->itemState & ODS_DISABLED) != 0;
    bool activePage = isActivePageButton(state, id);

    COLORREF fill = kThemePanelAlt;
    COLORREF border = RGB(40, 62, 77);
    COLORREF text = kThemeText;
    if (isPageButtonId(id)) {
        fill = RGB(14, 24, 32);
        border = RGB(35, 56, 69);
    } else if (isPrimaryButtonId(id)) {
        fill = RGB(19, 63, 49);
        border = kThemeAccentGreen;
    } else if (isUpgradeButtonId(id)) {
        fill = RGB(50, 39, 19);
        border = kThemeAccent;
    } else if (id == IDC_VALIDATE_BUTTON) {
        fill = RGB(30, 43, 70);
        border = kThemeAccentBlue;
    } else if (id == IDC_SAVE_BUTTON || id == IDC_LOAD_BUTTON) {
        fill = RGB(28, 40, 53);
    }
    if (activePage) {
        fill = RGB(68, 54, 20);
        border = kThemeAccent;
    }
    if (pressed) {
        fill = RGB(std::max(0, static_cast<int>(GetRValue(fill)) - 12),
                   std::max(0, static_cast<int>(GetGValue(fill)) - 12),
                   std::max(0, static_cast<int>(GetBValue(fill)) - 12));
    }
    if (disabled) {
        fill = RGB(28, 32, 38);
        border = RGB(48, 55, 62);
        text = RGB(103, 111, 118);
    }

    drawRoundedPanel(hdc, rect, fill, border, 12);

    RECT accentRect = rect;
    accentRect.right = accentRect.left + 4;
    HBRUSH accentBrush = CreateSolidBrush(activePage ? kThemeAccent : (isPrimaryButtonId(id) ? kThemeAccentGreen : border));
    FillRect(hdc, &accentRect, accentBrush);
    DeleteObject(accentBrush);

    wchar_t textBuffer[128]{};
    GetWindowTextW(drawItem->hwndItem, textBuffer, static_cast<int>(sizeof(textBuffer) / sizeof(textBuffer[0])));
    RECT textRect = rect;
    textRect.left += isPageButtonId(id) ? 18 : 6;
    if (pressed) OffsetRect(&textRect, 0, 1);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, text);
    HGDIOBJ oldFont = SelectObject(hdc, state.font ? state.font : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    DrawTextW(hdc,
              textBuffer,
              -1,
              &textRect,
              (isPageButtonId(id) ? DT_LEFT : DT_CENTER) | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);
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
    COLORREF bg = (row % 2 == 0) ? kThemeInput : RGB(14, 24, 33);
    COLORREF text = kThemeText;

    if (header->idFrom == IDC_TABLE_LIST) {
        std::string first = listViewText(state.tableList, row, 0);
        std::string second = listViewText(state.tableList, row, 1);
        if (titleMatches(state.currentModel.primary.title, "LeagueTableView")) {
            int totalRows = ListView_GetItemCount(state.tableList);
            int position = std::atoi(first.c_str());
            int relegationStart = std::max(1, totalRows - 1);
            int continentalSpots = std::max(1, totalRows / 4);
            if (second.find('*') != std::string::npos) {
                bg = RGB(24, 71, 54);
                text = RGB(242, 252, 247);
            } else if (position > 0 && position <= continentalSpots) {
                bg = RGB(20, 67, 49);
            } else if (position >= relegationStart) {
                bg = RGB(88, 34, 38);
                text = RGB(255, 237, 237);
            } else if (position > totalRows / 2) {
                bg = RGB(79, 64, 20);
            }
        } else if (second.find('*') != std::string::npos) {
            bg = RGB(22, 66, 51);
        } else if (first == "1" || row == 0) {
            bg = RGB(58, 48, 17);
            text = RGB(255, 244, 208);
        }
    } else if (header->idFrom == IDC_SQUAD_LIST) {
        if (titleMatches(state.currentModel.secondary.title, "TeamStatusPanel")) {
            std::string level = listViewText(state.squadList, row, 1);
            if (level == "Alta" && row >= 2) {
                bg = RGB(88, 34, 38);
                text = RGB(255, 237, 237);
            } else if (level == "Alta") {
                bg = RGB(21, 69, 52);
            } else if (level == "Media") {
                bg = RGB(84, 68, 24);
            } else if (level == "Baja" && row <= 1) {
                bg = RGB(88, 34, 38);
                text = RGB(255, 237, 237);
            } else if (level == "Baja") {
                bg = RGB(21, 69, 52);
            } else if (level == "Sin datos") {
                bg = RGB(33, 45, 56);
            }
        } else {
            std::string fitness = listViewText(state.squadList, row, 7);
            std::string morale = listViewText(state.squadList, row, 5);
            std::string status = listViewText(state.squadList, row, std::max(0, Header_GetItemCount(ListView_GetHeader(state.squadList)) - 1));
            int fitnessValue = fitness.empty() ? 80 : std::atoi(fitness.c_str());
            int moraleValue = morale.empty() ? 60 : std::atoi(morale.c_str());
            if (status.find("Les") != std::string::npos) {
                bg = RGB(82, 34, 36);
            } else if (fitnessValue < 62 || moraleValue < 45) {
                bg = RGB(82, 62, 20);
            } else if (fitnessValue >= 85 && moraleValue >= 65) {
                bg = RGB(19, 62, 47);
            }
        }
    } else if (header->idFrom == IDC_TRANSFER_LIST) {
        if (titleMatches(state.currentModel.footer.title, "InjuryListWidget")) {
            std::string issueType = listViewText(state.transferList, row, 1);
            std::string note = listViewText(state.transferList, row, 4);
            if (issueType == "-" && note.find("Plantilla completa") != std::string::npos) {
                bg = RGB(21, 69, 52);
            } else if (issueType.find("Fatiga") != std::string::npos) {
                bg = RGB(84, 68, 24);
            } else {
                bg = RGB(88, 34, 38);
                text = RGB(255, 237, 237);
            }
        } else {
            std::string type = listViewText(state.transferList, row, 0);
            if (type.find("Mercado") != std::string::npos || type.find("Objetivo") != std::string::npos) {
                bg = RGB(19, 61, 47);
            } else if (type.find("Contrato") != std::string::npos || type.find("Precontrato") != std::string::npos) {
                bg = RGB(66, 50, 19);
            } else if (type.find("Salida") != std::string::npos) {
                bg = RGB(78, 33, 35);
            } else if (type.find("Prestamo") != std::string::npos) {
                bg = RGB(24, 42, 64);
            }
        }
    }

    if ((draw->nmcd.uItemState & CDIS_SELECTED) != 0) {
        bg = kThemeSelection;
        text = RGB(255, 255, 255);
    }

    draw->clrText = text;
    draw->clrTextBk = bg;
    return CDRF_NEWFONT;
}

}  // namespace gui_win32

#endif
