#include "gui/gui_internal.h"

#ifdef _WIN32

#include "utils/utils.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <map>
#include <string>
#include <vector>

namespace gui_win32 {

namespace {

enum class TeamLogoPattern {
    Band,
    Stripe,
    Split,
    Sash,
    Ring
};

struct TeamLogoStyle {
    COLORREF base = RGB(42, 74, 108);
    COLORREF band = RGB(20, 34, 48);
    COLORREF accent = RGB(210, 180, 86);
    COLORREF text = RGB(245, 247, 249);
    COLORREF border = RGB(222, 228, 232);
    TeamLogoPattern pattern = TeamLogoPattern::Band;
};

std::string stripLogoNoise(std::string id) {
    static const std::array<std::string, 7> prefixes = {
        "clubdeportes", "clubsocialydeportivo", "clubsocial", "club", "csd", "cd", "fc"
    };
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& prefix : prefixes) {
            if (id.size() > prefix.size() + 3 && id.rfind(prefix, 0) == 0) {
                id = id.substr(prefix.size());
                changed = true;
                break;
            }
        }
    }
    return id;
}

std::string canonicalTeamLogoId(const std::string& teamName) {
    std::string id = normalizeTeamId(teamName);
    if (id.size() > 2 && id.substr(id.size() - 2) == "st") {
        id = id.substr(0, id.size() - 2);
    }
    id = stripLogoNoise(id);
    if (id == "universidadcatolica" || id == "catolica") return "universidadcatolica";
    if (id == "universidaddechile" || id == "udechile") return "universidaddechile";
    if (id == "colocolo" || id == "colocolo") return "colocolo";
    if (id == "ohiggins") return "ohiggins";
    if (id == "unionespanola") return "unionespanola";
    if (id == "unionlacalera") return "unionlacalera";
    if (id == "unionsanfelipe") return "unionsanfelipe";
    if (id == "universidaddeconcepcion") return "universidaddeconcepcion";
    return id;
}

std::wstring cleanDisplayTeamName(const std::string& rawName) {
    std::string name = trim(rawName);
    if (name.size() > 2 && name.substr(name.size() - 2) == " *") {
        name = trim(name.substr(0, name.size() - 2));
    }
    return utf8ToWide(name);
}

std::wstring teamMonogram(const std::string& teamName) {
    const std::string id = canonicalTeamLogoId(teamName);
    static const std::map<std::string, std::wstring> overrides = {
        {"audaxitaliano", L"AI"},
        {"cobresal", L"CS"},
        {"everton", L"EV"},
        {"nublense", L"N"},
        {"ohiggins", L"OH"},
        {"palestino", L"P"},
        {"universidadcatolica", L"UC"},
        {"universidaddechile", L"U"},
        {"coquimbounido", L"CQ"},
        {"colocolo", L"CC"},
        {"deportesconcepcion", L"DC"},
        {"deporteslaserena", L"LS"},
        {"deporteslimache", L"DL"},
        {"huachipato", L"H"},
        {"unionlacalera", L"UL"},
        {"universidaddeconcepcion", L"UD"},
        {"cobreloa", L"CL"},
        {"puertomontt", L"PM"},
        {"deportesiquique", L"DI"},
        {"curicounido", L"CU"},
        {"deportesantofagasta", L"DA"},
        {"deportescopiapo", L"CP"},
        {"deportesrecoleta", L"DR"},
        {"deportessantacruz", L"SC"},
        {"deportestemuco", L"DT"},
        {"magallanes", L"M"},
        {"rangersdetalca", L"RT"},
        {"sanluisdequillota", L"SL"},
        {"sanmarcosdearica", L"SA"},
        {"santiagowanderers", L"SW"},
        {"unionespanola", L"UE"},
        {"unionsanfelipe", L"US"}
    };
    auto it = overrides.find(id);
    if (it != overrides.end()) return it->second;

    std::string cleaned = trim(teamName);
    for (char& c : cleaned) {
        if (c == '-' || c == '/' || c == '.') c = ' ';
    }
    std::vector<std::string> parts = splitByDelimiter(cleaned, ' ');
    std::wstring result;
    for (const auto& part : parts) {
        const std::string token = toLower(trim(part));
        if (token.empty() || token == "club" || token == "cd" || token == "csd" ||
            token == "fc" || token == "de" || token == "del" || token == "la") {
            continue;
        }
        result.push_back(static_cast<wchar_t>(std::towupper(static_cast<wchar_t>(token.front()))));
        if (result.size() == 2) break;
    }
    if (result.empty()) {
        const std::wstring wide = cleanDisplayTeamName(teamName);
        for (wchar_t ch : wide) {
            if (std::iswalnum(ch)) {
                result.push_back(static_cast<wchar_t>(std::towupper(ch)));
                if (result.size() == 2) break;
            }
        }
    }
    if (result.empty()) result = L"FC";
    return result;
}

TeamLogoStyle hashedStyle(const std::string& canonicalId, const std::string& divisionId) {
    static const std::array<TeamLogoStyle, 6> fallbackStyles = {{
        {RGB(31, 69, 114), RGB(18, 35, 58), RGB(211, 178, 89), RGB(245, 247, 249), RGB(231, 236, 239), TeamLogoPattern::Band},
        {RGB(38, 98, 72), RGB(20, 50, 39), RGB(221, 199, 126), RGB(244, 247, 248), RGB(228, 235, 238), TeamLogoPattern::Stripe},
        {RGB(118, 42, 43), RGB(69, 18, 22), RGB(232, 211, 154), RGB(248, 245, 240), RGB(238, 228, 220), TeamLogoPattern::Sash},
        {RGB(83, 63, 126), RGB(40, 27, 67), RGB(224, 201, 138), RGB(246, 245, 250), RGB(231, 225, 238), TeamLogoPattern::Ring},
        {RGB(92, 58, 26), RGB(44, 26, 12), RGB(233, 214, 153), RGB(250, 247, 240), RGB(239, 231, 220), TeamLogoPattern::Split},
        {RGB(24, 100, 109), RGB(14, 51, 56), RGB(232, 207, 122), RGB(243, 248, 249), RGB(227, 235, 236), TeamLogoPattern::Band}
    }};

    unsigned int hash = 0;
    const std::string seed = canonicalId + "|" + divisionId;
    for (unsigned char ch : seed) {
        hash = hash * 131u + ch;
    }
    TeamLogoStyle style = fallbackStyles[hash % fallbackStyles.size()];
    if (divisionId == "primera division") {
        style.accent = kThemeAccent;
    } else if (divisionId == "primera b") {
        style.accent = RGB(194, 205, 214);
    } else if (divisionId == "segunda division") {
        style.accent = RGB(202, 177, 107);
    } else if (divisionId == "tercera division a") {
        style.accent = RGB(135, 198, 176);
    } else if (divisionId == "tercera division b") {
        style.accent = RGB(192, 154, 120);
    }
    return style;
}

TeamLogoStyle styleForTeam(const std::string& teamName, const std::string& divisionId) {
    const std::string id = canonicalTeamLogoId(teamName);
    static const std::map<std::string, TeamLogoStyle> overrides = {
        {"audaxitaliano", {RGB(33, 122, 76), RGB(18, 70, 44), RGB(205, 61, 69), RGB(245, 248, 249), RGB(236, 240, 242), TeamLogoPattern::Stripe}},
        {"cobresal", {RGB(214, 108, 27), RGB(129, 57, 16), RGB(244, 210, 118), RGB(249, 244, 236), RGB(242, 230, 214), TeamLogoPattern::Band}},
        {"everton", {RGB(29, 55, 129), RGB(18, 31, 78), RGB(230, 177, 54), RGB(246, 248, 250), RGB(230, 235, 240), TeamLogoPattern::Band}},
        {"nublense", {RGB(190, 38, 43), RGB(98, 15, 18), RGB(248, 247, 246), RGB(248, 247, 246), RGB(241, 236, 233), TeamLogoPattern::Split}},
        {"ohiggins", {RGB(73, 156, 214), RGB(26, 90, 143), RGB(252, 247, 237), RGB(252, 247, 237), RGB(235, 241, 245), TeamLogoPattern::Band}},
        {"palestino", {RGB(28, 104, 63), RGB(13, 48, 30), RGB(204, 57, 51), RGB(248, 249, 244), RGB(232, 236, 228), TeamLogoPattern::Split}},
        {"universidadcatolica", {RGB(247, 248, 250), RGB(26, 77, 165), RGB(199, 38, 52), RGB(23, 66, 143), RGB(230, 236, 244), TeamLogoPattern::Band}},
        {"universidaddechile", {RGB(19, 83, 176), RGB(13, 48, 105), RGB(210, 45, 48), RGB(247, 249, 250), RGB(227, 234, 242), TeamLogoPattern::Ring}},
        {"coquimbounido", {RGB(214, 173, 58), RGB(92, 60, 16), RGB(33, 29, 22), RGB(32, 28, 20), RGB(232, 217, 166), TeamLogoPattern::Stripe}},
        {"colocolo", {RGB(247, 248, 249), RGB(27, 28, 31), RGB(193, 51, 58), RGB(28, 29, 32), RGB(233, 237, 239), TeamLogoPattern::Sash}},
        {"deportesconcepcion", {RGB(81, 45, 147), RGB(39, 22, 78), RGB(233, 205, 121), RGB(247, 245, 250), RGB(227, 220, 236), TeamLogoPattern::Band}},
        {"deporteslaserena", {RGB(145, 22, 33), RGB(82, 12, 20), RGB(241, 208, 119), RGB(247, 243, 239), RGB(236, 228, 220), TeamLogoPattern::Band}},
        {"deporteslimache", {RGB(46, 128, 74), RGB(21, 73, 43), RGB(242, 209, 104), RGB(245, 249, 246), RGB(226, 236, 228), TeamLogoPattern::Stripe}},
        {"huachipato", {RGB(39, 87, 165), RGB(20, 46, 88), RGB(241, 246, 249), RGB(241, 246, 249), RGB(226, 234, 241), TeamLogoPattern::Split}},
        {"unionlacalera", {RGB(204, 42, 41), RGB(96, 15, 17), RGB(243, 242, 241), RGB(247, 245, 243), RGB(236, 231, 228), TeamLogoPattern::Stripe}},
        {"universidaddeconcepcion", {RGB(246, 201, 37), RGB(31, 69, 129), RGB(31, 69, 129), RGB(24, 49, 93), RGB(232, 224, 170), TeamLogoPattern::Split}},
        {"cobreloa", {RGB(233, 108, 22), RGB(122, 52, 11), RGB(35, 44, 83), RGB(247, 243, 237), RGB(236, 224, 212), TeamLogoPattern::Band}},
        {"puertomontt", {RGB(44, 119, 168), RGB(18, 58, 94), RGB(241, 229, 96), RGB(244, 248, 249), RGB(225, 233, 236), TeamLogoPattern::Band}},
        {"deportesiquique", {RGB(60, 165, 224), RGB(18, 81, 134), RGB(243, 71, 69), RGB(249, 249, 250), RGB(229, 236, 240), TeamLogoPattern::Ring}},
        {"curicounido", {RGB(186, 28, 41), RGB(92, 12, 19), RGB(244, 246, 248), RGB(244, 246, 248), RGB(233, 236, 238), TeamLogoPattern::Band}},
        {"deportesantofagasta", {RGB(45, 96, 171), RGB(19, 47, 94), RGB(220, 67, 70), RGB(246, 247, 249), RGB(227, 233, 238), TeamLogoPattern::Split}},
        {"deportescopiapo", {RGB(208, 41, 53), RGB(103, 14, 22), RGB(242, 242, 243), RGB(246, 246, 247), RGB(233, 229, 231), TeamLogoPattern::Band}},
        {"deportesrecoleta", {RGB(242, 173, 48), RGB(100, 61, 15), RGB(42, 38, 28), RGB(28, 24, 18), RGB(231, 215, 168), TeamLogoPattern::Stripe}},
        {"deportessantacruz", {RGB(198, 34, 42), RGB(24, 24, 27), RGB(246, 247, 248), RGB(246, 247, 248), RGB(233, 235, 237), TeamLogoPattern::Split}},
        {"deportestemuco", {RGB(32, 117, 61), RGB(16, 61, 33), RGB(242, 245, 247), RGB(245, 247, 248), RGB(230, 236, 232), TeamLogoPattern::Band}},
        {"magallanes", {RGB(112, 42, 132), RGB(53, 20, 68), RGB(229, 198, 121), RGB(247, 243, 249), RGB(231, 224, 236), TeamLogoPattern::Band}},
        {"rangersdetalca", {RGB(194, 40, 43), RGB(82, 13, 16), RGB(247, 247, 247), RGB(247, 247, 247), RGB(234, 232, 231), TeamLogoPattern::Stripe}},
        {"sanluisdequillota", {RGB(241, 204, 75), RGB(116, 70, 12), RGB(18, 22, 31), RGB(20, 24, 32), RGB(232, 220, 170), TeamLogoPattern::Band}},
        {"sanmarcosdearica", {RGB(66, 168, 214), RGB(22, 93, 139), RGB(237, 74, 63), RGB(248, 248, 249), RGB(229, 236, 238), TeamLogoPattern::Sash}},
        {"santiagowanderers", {RGB(32, 118, 77), RGB(15, 64, 42), RGB(244, 245, 246), RGB(246, 247, 247), RGB(226, 234, 229), TeamLogoPattern::Split}},
        {"unionespanola", {RGB(208, 42, 46), RGB(112, 16, 24), RGB(235, 200, 112), RGB(247, 244, 239), RGB(235, 227, 219), TeamLogoPattern::Band}},
        {"unionsanfelipe", {RGB(202, 38, 47), RGB(25, 43, 98), RGB(244, 247, 248), RGB(244, 247, 248), RGB(230, 235, 238), TeamLogoPattern::Split}}
    };

    auto it = overrides.find(id);
    if (it != overrides.end()) return it->second;
    return hashedStyle(id, divisionId);
}

const Team* findKnownTeam(const AppState& state, const std::string& rawName) {
    const std::string name = trim(rawName);
    if (name.empty() || name == "-" || name.find("No hay") != std::string::npos) return nullptr;

    for (const auto& team : state.career.allTeams) {
        if (team.name == name) return &team;
    }

    const std::string targetId = canonicalTeamLogoId(name);
    for (const auto& team : state.career.allTeams) {
        if (canonicalTeamLogoId(team.name) == targetId) return &team;
    }
    return nullptr;
}

std::string teamDivisionForName(const AppState& state, const std::string& rawName) {
    if (const Team* team = findKnownTeam(state, rawName)) {
        return team->division;
    }
    if (!state.gameSetup.division.empty()) return state.gameSetup.division;
    if (!state.career.activeDivision.empty()) return state.career.activeDivision;
    return std::string();
}

void fillShieldRegion(HDC hdc,
                      const RECT& rect,
                      const TeamLogoStyle& style,
                      const std::array<POINT, 6>& points) {
    HRGN clip = CreatePolygonRgn(points.data(), static_cast<int>(points.size()), WINDING);
    if (!clip) return;

    SelectClipRgn(hdc, clip);

    HBRUSH baseBrush = CreateSolidBrush(style.base);
    FillRect(hdc, &rect, baseBrush);
    DeleteObject(baseBrush);

    HBRUSH layerBrush = CreateSolidBrush(style.band);
    HBRUSH accentBrush = CreateSolidBrush(style.accent);
    switch (style.pattern) {
        case TeamLogoPattern::Band: {
            RECT band{rect.left, rect.top, rect.right, rect.top + (rect.bottom - rect.top) / 3};
            FillRect(hdc, &band, layerBrush);
            RECT footer{rect.left, rect.bottom - (rect.bottom - rect.top) / 7, rect.right, rect.bottom};
            FillRect(hdc, &footer, accentBrush);
            break;
        }
        case TeamLogoPattern::Stripe: {
            RECT stripe{rect.left + (rect.right - rect.left) / 3,
                        rect.top,
                        rect.left + (rect.right - rect.left) * 2 / 3,
                        rect.bottom};
            FillRect(hdc, &stripe, accentBrush);
            RECT band{rect.left, rect.top, rect.right, rect.top + (rect.bottom - rect.top) / 4};
            FillRect(hdc, &band, layerBrush);
            break;
        }
        case TeamLogoPattern::Split: {
            RECT leftHalf{rect.left, rect.top, rect.left + (rect.right - rect.left) / 2, rect.bottom};
            RECT rightHalf{leftHalf.right, rect.top, rect.right, rect.bottom};
            FillRect(hdc, &leftHalf, layerBrush);
            FillRect(hdc, &rightHalf, accentBrush);
            break;
        }
        case TeamLogoPattern::Sash: {
            RECT band{rect.left, rect.top, rect.right, rect.top + (rect.bottom - rect.top) / 4};
            FillRect(hdc, &band, layerBrush);
            POINT sash[4] = {
                {rect.left - (rect.right - rect.left) / 8, rect.top + (rect.bottom - rect.top) / 3},
                {rect.left + (rect.right - rect.left) / 6, rect.top + (rect.bottom - rect.top) / 5},
                {rect.right + (rect.right - rect.left) / 10, rect.bottom - (rect.bottom - rect.top) / 3},
                {rect.right - (rect.right - rect.left) / 5, rect.bottom - (rect.bottom - rect.top) / 6}
            };
            HGDIOBJ oldBrush = SelectObject(hdc, accentBrush);
            HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
            Polygon(hdc, sash, 4);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            break;
        }
        case TeamLogoPattern::Ring: {
            RECT band{rect.left, rect.top, rect.right, rect.top + (rect.bottom - rect.top) / 4};
            FillRect(hdc, &band, layerBrush);
            const int ringWidth = std::max(2, static_cast<int>((rect.right - rect.left) / 10));
            HPEN pen = CreatePen(PS_SOLID, ringWidth, style.accent);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            const int inset = (rect.right - rect.left) / 5;
            Ellipse(hdc, rect.left + inset, rect.top + inset, rect.right - inset, rect.bottom - inset);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            break;
        }
    }

    DeleteObject(layerBrush);
    DeleteObject(accentBrush);
    SelectClipRgn(hdc, nullptr);
    DeleteObject(clip);
}

HFONT makeLogoFont(int pixelHeight) {
    return CreateFontW(-pixelHeight,
                       0,
                       0,
                       0,
                       FW_HEAVY,
                       FALSE,
                       FALSE,
                       FALSE,
                       DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY,
                       DEFAULT_PITCH | FF_SWISS,
                       L"Segoe UI");
}

void drawGeneratedTeamLogo(HDC hdc,
                           const RECT& rect,
                           const std::string& teamName,
                           const std::string& divisionId) {
    const TeamLogoStyle style = styleForTeam(teamName, divisionId);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const int topDip = std::max(4, height / 22);
    const int shoulder = std::max(8, width / 6);
    const int bottomInset = std::max(10, width / 5);

    std::array<POINT, 6> shield = {{
        {rect.left + width / 2, rect.top + topDip},
        {rect.right - shoulder, rect.top + topDip + height / 7},
        {rect.right - shoulder / 2, rect.top + height / 2},
        {rect.left + width / 2, rect.bottom - topDip},
        {rect.left + shoulder / 2, rect.top + height / 2},
        {rect.left + shoulder, rect.top + topDip + height / 7}
    }};

    fillShieldRegion(hdc, rect, style, shield);

    HPEN borderPen = CreatePen(PS_SOLID, std::max(2, width / 20), style.border);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Polygon(hdc, shield.data(), static_cast<int>(shield.size()));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    RECT textRect{
        rect.left + width / 6,
        rect.top + height / 3 - height / 14,
        rect.right - width / 6,
        rect.bottom - height / 5
    };
    const std::wstring monogram = teamMonogram(teamName);
    HFONT font = makeLogoFont(std::max(12, height / (monogram.size() > 1 ? 3 : 2)));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, style.text);
    DrawTextW(hdc, monogram.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);
    DeleteObject(font);

    HPEN innerPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    oldPen = SelectObject(hdc, innerPen);
    oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    const int inset = std::max(3, width / 14);
    std::array<POINT, 6> inner = {{
        {shield[0].x, shield[0].y + inset},
        {shield[1].x - inset, shield[1].y + inset / 2},
        {shield[2].x - inset, shield[2].y - inset},
        {shield[3].x, shield[3].y - inset},
        {shield[4].x + inset, shield[4].y - inset},
        {shield[5].x + inset, shield[5].y + inset / 2}
    }};
    Polygon(hdc, inner.data(), static_cast<int>(inner.size()));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(innerPen);

    (void)bottomInset;
}

HBITMAP createTeamLogoBitmap(const AppState& state,
                             const std::string& teamName,
                             const std::string& divisionId,
                             int size) {
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap) return nullptr;

    HDC mem = CreateCompatibleDC(nullptr);
    HGDIOBJ oldBitmap = SelectObject(mem, bitmap);
    RECT full{0, 0, size, size};
    HBRUSH maskBrush = CreateSolidBrush(RGB(255, 0, 255));
    FillRect(mem, &full, maskBrush);
    DeleteObject(maskBrush);

    RECT inner{size / 8, size / 10, size - size / 8, size - size / 10};
    drawGeneratedTeamLogo(mem, inner, teamName, divisionId);

    SelectObject(mem, oldBitmap);
    DeleteDC(mem);
    (void)state;
    return bitmap;
}

int ensureTeamLogoImageIndex(AppState& state,
                             const std::string& teamName,
                             const std::string& divisionId) {
    if (!state.teamLogoImageList) {
        rebuildTeamLogoImageList(state);
    }
    if (!state.teamLogoImageList) return -1;

    const int iconSize = scaleByDpi(state, 20);
    const std::string key = canonicalTeamLogoId(teamName) + "|" + divisionId + "|" + std::to_string(iconSize);
    auto existing = state.teamLogoImageIndices.find(key);
    if (existing != state.teamLogoImageIndices.end()) return existing->second;

    HBITMAP bitmap = createTeamLogoBitmap(state, teamName, divisionId, iconSize);
    if (!bitmap) return -1;

    const int index = ImageList_AddMasked(state.teamLogoImageList, bitmap, RGB(255, 0, 255));
    DeleteObject(bitmap);
    if (index >= 0) {
        state.teamLogoImageIndices[key] = index;
    }
    return index;
}

void setListSubItemImage(HWND list, int row, int column, int imageIndex) {
    if (!list || row < 0 || column < 0 || imageIndex < 0) return;
    LVITEMW item{};
    item.mask = LVIF_IMAGE;
    item.iItem = row;
    item.iSubItem = column;
    item.iImage = imageIndex;
    ListView_SetItem(list, &item);
}

std::vector<int> teamColumnsForTitle(const std::string& title) {
    if (title == "LeagueTableView") return {1};
    if (title == "FixtureListView") return {2};
    if (title == "RaceContext") return {1};
    if (title == "SeasonHistory" || title == "SeasonRecords") return {1};
    if (title == "TransferMarketView") return {10};
    return {};
}

std::string contextTeamNameFromRow(const ListPanelModel& model,
                                   const std::vector<std::string>& row,
                                   int column) {
    if (column < 0 || column >= static_cast<int>(row.size())) return std::string();
    std::string name = trim(row[static_cast<size_t>(column)]);
    if (name.empty() || name == "-") return std::string();
    if (column == 1 && model.title == "LeagueTableView" && name.size() > 2 && name.substr(name.size() - 2) == " *") {
        name = trim(name.substr(0, name.size() - 2));
    }
    return name;
}

const Team* selectedSetupTeam(const AppState& state) {
    if (state.gameSetup.division.empty() || state.gameSetup.club.empty()) return nullptr;
    for (const auto& team : state.career.allTeams) {
        if (team.division == state.gameSetup.division && team.name == state.gameSetup.club) return &team;
    }
    return nullptr;
}

const Team* selectedListContextTeam(const AppState& state) {
    if (state.currentPage == GuiPage::Transfers && !state.selectedTransferClub.empty()) {
        if (const Team* team = findKnownTeam(state, state.selectedTransferClub)) return team;
    }

    const int tableRow = state.tableList ? selectedListViewRow(state.tableList) : -1;
    if (tableRow >= 0 && tableRow < static_cast<int>(state.currentModel.primary.rows.size())) {
        for (int column : teamColumnsForTitle(state.currentModel.primary.title)) {
            const std::string name = contextTeamNameFromRow(state.currentModel.primary, state.currentModel.primary.rows[static_cast<size_t>(tableRow)], column);
            if (const Team* team = findKnownTeam(state, name)) return team;
        }
    }

    const int squadRow = state.squadList ? selectedListViewRow(state.squadList) : -1;
    if (squadRow >= 0 && squadRow < static_cast<int>(state.currentModel.secondary.rows.size())) {
        for (int column : teamColumnsForTitle(state.currentModel.secondary.title)) {
            const std::string name = contextTeamNameFromRow(state.currentModel.secondary, state.currentModel.secondary.rows[static_cast<size_t>(squadRow)], column);
            if (const Team* team = findKnownTeam(state, name)) return team;
        }
    }

    return nullptr;
}

const Team* contextTeamForPage(const AppState& state) {
    if (const Team* selected = selectedListContextTeam(state)) return selected;
    if (state.career.myTeam) return state.career.myTeam;
    return selectedSetupTeam(state);
}

void drawLogoCard(AppState& state, HDC hdc, const RECT& rect, const Team& team) {
    const auto s = [&](int value) { return scaleByDpi(state, value); };
    drawRoundedPanel(hdc, rect, RGB(15, 29, 38), RGB(51, 78, 94), s(18));

    RECT logoRect{
        rect.left + s(14),
        rect.top + s(12),
        rect.right - s(14),
        rect.top + s(12) + std::min(rect.bottom - rect.top - s(58), rect.right - rect.left - s(28))
    };
    if (logoRect.bottom > rect.bottom - s(44)) {
        const int size = std::max(s(40), static_cast<int>(rect.bottom - rect.top - s(60)));
        logoRect.top = rect.top + s(12);
        logoRect.bottom = logoRect.top + size;
    }
    drawGeneratedTeamLogo(hdc, logoRect, team.name, team.division);

    RECT nameRect{rect.left + s(12), logoRect.bottom + s(8), rect.right - s(12), rect.bottom - s(20)};
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kThemeText);
    HGDIOBJ oldFont = SelectObject(hdc, state.sectionFont ? state.sectionFont : state.font);
    std::wstring title = cleanDisplayTeamName(team.name);
    DrawTextW(hdc, title.c_str(), -1, &nameRect, DT_CENTER | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
    SelectObject(hdc, state.font ? state.font : oldFont);
    SetTextColor(hdc, kThemeMuted);
    RECT subtitleRect{nameRect.left, nameRect.top + s(22), nameRect.right, rect.bottom - s(8)};
    const std::wstring subtitle = utf8ToWide(divisionDisplay(team.division));
    DrawTextW(hdc, subtitle.c_str(), -1, &subtitleRect, DT_CENTER | DT_TOP | DT_END_ELLIPSIS | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
}

}  // namespace

void rebuildTeamLogoImageList(AppState& state) {
    if (state.teamLogoImageList) {
        ImageList_Destroy(state.teamLogoImageList);
        state.teamLogoImageList = nullptr;
    }
    state.teamLogoImageIndices.clear();

    const int iconSize = scaleByDpi(state, 20);
    state.teamLogoImageList = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 24, 48);
    if (state.tableList) ListView_SetImageList(state.tableList, state.teamLogoImageList, LVSIL_SMALL);
    if (state.squadList) ListView_SetImageList(state.squadList, state.teamLogoImageList, LVSIL_SMALL);
    if (state.transferList) ListView_SetImageList(state.transferList, state.teamLogoImageList, LVSIL_SMALL);
}

void applyTeamLogosToList(AppState& state, HWND list, const ListPanelModel& model) {
    if (!list || model.rows.empty()) return;
    const std::vector<int> columns = teamColumnsForTitle(model.title);
    if (columns.empty()) return;

    if (!state.teamLogoImageList) {
        rebuildTeamLogoImageList(state);
    }
    if (!state.teamLogoImageList) return;
    ListView_SetImageList(list, state.teamLogoImageList, LVSIL_SMALL);

    for (size_t rowIndex = 0; rowIndex < model.rows.size(); ++rowIndex) {
        const auto& row = model.rows[rowIndex];
        for (int column : columns) {
            const std::string teamName = contextTeamNameFromRow(model, row, column);
            if (teamName.empty()) continue;
            const int imageIndex = ensureTeamLogoImageIndex(state, teamName, teamDivisionForName(state, teamName));
            if (imageIndex >= 0) {
                setListSubItemImage(list, static_cast<int>(rowIndex), column, imageIndex);
            }
        }
    }
}

const Team* currentContextTeam(const AppState& state) {
    return contextTeamForPage(state);
}

void drawContextTeamLogo(AppState& state, HDC hdc) {
    if (isFrontMenuPage(state.currentPage)) return;
    const Team* team = currentContextTeam(state);
    if (!team) return;
    if (state.layout.contextCard.right <= state.layout.contextCard.left ||
        state.layout.contextCard.bottom <= state.layout.contextCard.top) {
        return;
    }
    drawLogoCard(state, hdc, state.layout.contextCard, *team);
}

}  // namespace gui_win32

#endif
