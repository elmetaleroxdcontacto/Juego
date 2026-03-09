#include "utils.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace std;

static std::mt19937 rng{std::random_device{}()};

static bool isPathSeparator(char c) {
    return c == '/' || c == '\\';
}

#ifdef _WIN32
static std::wstring utf8ToWide(const string& text) {
    if (text.empty()) return std::wstring();
    int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0) return std::wstring(text.begin(), text.end());
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wide[0], length);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}

static string wideToUtf8(const std::wstring& text) {
    if (text.empty()) return string();
    int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0) return string(text.begin(), text.end());
    string utf8(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &utf8[0], length, nullptr, nullptr);
    if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
    return utf8;
}
#endif

static char preferredSeparator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

static string normalizePathSeparators(const string& path) {
    string normalized = path;
    const char separator = preferredSeparator();
    for (char& c : normalized) {
        if (isPathSeparator(c)) c = separator;
    }
    return normalized;
}

static bool isAbsolutePath(const string& path) {
    if (path.empty()) return false;
#ifdef _WIN32
    return (path.size() >= 2 && path[1] == ':') || isPathSeparator(path[0]);
#else
    return path[0] == '/';
#endif
}

static bool createSingleDirectory(const string& path) {
#ifdef _WIN32
    if (_wmkdir(utf8ToWide(path).c_str()) == 0) return true;
#else
    if (mkdir(path.c_str(), 0755) == 0) return true;
#endif
    return errno == EEXIST && isDirectory(path);
}

int randInt(int minVal, int maxVal) {
    if (maxVal < minVal) swap(maxVal, minVal);
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(rng);
}

double rand01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

string trim(const string& s) {
    size_t start = 0;
    while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

string toLower(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return out;
}

bool pathExists(const string& path) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(utf8ToWide(path).c_str());
    return attributes != INVALID_FILE_ATTRIBUTES;
#else
    struct stat info;
    return stat(path.c_str(), &info) == 0;
#endif
}

bool isDirectory(const string& path) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(utf8ToWide(path).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat info;
    if (stat(path.c_str(), &info) != 0) return false;
    return S_ISDIR(info.st_mode);
#endif
}

bool ensureDirectory(const string& path) {
    string normalized = trim(normalizePathSeparators(path));
    if (normalized.empty()) return false;
    if (isDirectory(normalized)) return true;

    string current;
    size_t index = 0;
    const char separator = preferredSeparator();

#ifdef _WIN32
    if (normalized.size() >= 2 && normalized[1] == ':') {
        current = normalized.substr(0, 2);
        index = 2;
        if (index < normalized.size() && isPathSeparator(normalized[index])) {
            current.push_back(separator);
            ++index;
        }
    } else if (isPathSeparator(normalized[0])) {
        current.push_back(separator);
        index = 1;
    }
#else
    if (isPathSeparator(normalized[0])) {
        current.push_back(separator);
        index = 1;
    }
#endif

    string component;
    for (; index <= normalized.size(); ++index) {
        const bool reachedEnd = index == normalized.size();
        const char c = reachedEnd ? separator : normalized[index];
        if (reachedEnd || isPathSeparator(c)) {
            if (component.empty()) continue;
            if (!current.empty() && !isPathSeparator(current.back())) current.push_back(separator);
            current += component;
            if (!isDirectory(current) && !createSingleDirectory(current)) return false;
            component.clear();
            continue;
        }
        component.push_back(c);
    }
    return isDirectory(normalized);
}

bool readTextFileLines(const string& path, vector<string>& lines) {
    lines.clear();
#ifdef _WIN32
    HANDLE handle = CreateFileW(utf8ToWide(path).c_str(),
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                nullptr,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;

    string content;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (ReadFile(handle, buffer, static_cast<DWORD>(sizeof(buffer)), &bytesRead, nullptr) && bytesRead > 0) {
        content.append(buffer, bytesRead);
    }
    CloseHandle(handle);
#else
    FILE* file = fopen(path.c_str(), "rb");
    if (file == nullptr) return false;
    string content;
    char buffer[4096];
    size_t bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        content.append(buffer, bytesRead);
    }
    fclose(file);
#endif

    istringstream stream(content);
    string line;
    while (getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    if (!content.empty() && content.back() == '\n') return true;
    if (!content.empty() && (lines.empty() || !lines.back().empty())) {
        return true;
    }
    return !lines.empty();
}

vector<string> listDirectories(const string& root) {
    vector<string> out;
    if (!isDirectory(root)) return out;
#ifdef _WIN32
    string pattern = joinPath(root, "*");
    WIN32_FIND_DATAW findData;
    HANDLE handle = FindFirstFileW(utf8ToWide(pattern).c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE) return out;
    do {
        const string name = wideToUtf8(findData.cFileName);
        if (name == "." || name == "..") continue;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            out.push_back(joinPath(root, name));
        }
    } while (FindNextFileW(handle, &findData));
    FindClose(handle);
#else
    DIR* dir = opendir(root.c_str());
    if (dir == nullptr) return out;
    while (dirent* entry = readdir(dir)) {
        const string name = entry->d_name;
        if (name == "." || name == "..") continue;
        const string fullPath = joinPath(root, name);
        if (isDirectory(fullPath)) out.push_back(fullPath);
    }
    closedir(dir);
#endif
    return out;
}

string joinPath(const string& a, const string& b) {
    if (a.empty()) return b;
    if (b.empty()) return normalizePathSeparators(a);
    if (isAbsolutePath(b)) return normalizePathSeparators(b);
    string left = normalizePathSeparators(a);
    string right = normalizePathSeparators(b);
    const char separator = preferredSeparator();
    while (!right.empty() && isPathSeparator(right.front())) right.erase(right.begin());
    if (!left.empty() && !isPathSeparator(left.back())) left.push_back(separator);
    return left + right;
}

string pathFilename(const string& path) {
    const string normalized = normalizePathSeparators(path);
    const size_t pos = normalized.find_last_of("/\\");
    if (pos == string::npos) return normalized;
    return normalized.substr(pos + 1);
}

string pathExtension(const string& path) {
    const string filename = pathFilename(path);
    const size_t pos = filename.find_last_of('.');
    if (pos == string::npos || pos == 0) return "";
    return filename.substr(pos);
}

vector<string> splitCsvLine(const string& line) {
    vector<string> out;
    string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        if (c == ',' && !inQuotes) {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(trim(cur));
    return out;
}

vector<string> splitByDelimiter(const string& line, char delim) {
    vector<string> out;
    string cur;
    for (char c : line) {
        if (c == delim) {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(trim(cur));
    return out;
}

int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

string normalizePosition(string raw) {
    string s = toLower(trim(raw));
    if (s.empty() || s == "n/a") return "N/A";
    if (s.find("arq") != string::npos || s.find("goalkeeper") != string::npos || s == "gk" || s == "por") return "ARQ";
    if (s.find("def") != string::npos || s.find("centre-back") != string::npos || s.find("center-back") != string::npos ||
        s.find("full-back") != string::npos || s.find("fullback") != string::npos || s.find("wing-back") != string::npos ||
        s.find("left-back") != string::npos || s.find("right-back") != string::npos || s.find("lateral") != string::npos ||
        s.find("carrilero") != string::npos || s == "cb" || s == "rb" || s == "lb" || s == "lwb" || s == "rwb" || s == "wb")
        return "DEF";
    if (s.find("med") != string::npos || s.find("mid") != string::npos || s.find("medioc") != string::npos ||
        s.find("volante") != string::npos || s.find("pivote") != string::npos || s.find("enganche") != string::npos ||
        s.find("interior") != string::npos || s.find("holding") != string::npos ||
        s == "cm" || s == "dm" || s == "am" || s == "lm" || s == "rm")
        return "MED";
    if (s.find("del") != string::npos || s.find("forward") != string::npos || s.find("striker") != string::npos ||
        s.find("centre-forward") != string::npos || s.find("center-forward") != string::npos ||
        s.find("second striker") != string::npos || s.find("winger") != string::npos || s.find("extremo") != string::npos ||
        s.find("punta") != string::npos || s == "fw" || s == "st" || s == "cf" || s == "ss" || s == "rw" || s == "lw")
        return "DEL";
    return "N/A";
}

int parseAge(string s) {
    s = trim(s);
    if (s.empty() || s == "N/A") return 24;
    try {
        return stoi(s);
    } catch (...) {
        return 24;
    }
}

long long parseMarketValue(string s) {
    s = trim(s);
    if (s.empty() || s == "N/A" || s == "-") return 0;
    string t;
    for (char c : s) {
        if (isdigit(static_cast<unsigned char>(c)) || c == '.' || c == ',' || c == 'k' || c == 'K' || c == 'm' || c == 'M') {
            t.push_back(c);
        }
    }
    if (t.empty()) return 0;
    double factor = 1.0;
    char last = t.back();
    if (last == 'k' || last == 'K') {
        factor = 1e3;
        t.pop_back();
    } else if (last == 'm' || last == 'M') {
        factor = 1e6;
        t.pop_back();
    }
    for (char& c : t) {
        if (c == ',') c = '.';
    }
    try {
        double val = stod(t);
        return static_cast<long long>(val * factor);
    } catch (...) {
        return 0;
    }
}

int computeSkillFromValue(long long value, int age, const string& division) {
    int skill = 55;
    if (value > 0) {
        double logv = log10(static_cast<double>(value));
        skill = static_cast<int>(round((logv - 4.0) * 20.0 + 50.0));
    }
    if (division == "primera division") skill += 3;
    else if (division == "segunda division") skill -= 4;
    else if (division == "tercera division a") skill -= 6;
    else if (division == "tercera division b") skill -= 8;
    if (age > 30) skill -= (age - 30) / 2;
    return clampInt(skill, 30, 90);
}

void getDivisionSkillRange(const string& division, int& minSkill, int& maxSkill) {
    if (division == "primera division") {
        minSkill = 60; maxSkill = 85; return;
    }
    if (division == "primera b") {
        minSkill = 52; maxSkill = 78; return;
    }
    if (division == "segunda division") {
        minSkill = 48; maxSkill = 72; return;
    }
    if (division == "tercera division a") {
        minSkill = 42; maxSkill = 68; return;
    }
    minSkill = 40; maxSkill = 65;
}

string divisionDisplay(const string& id) {
    if (id == "primera division") return "Primera Division";
    if (id == "primera b") return "Primera B";
    if (id == "segunda division") return "Segunda Division";
    if (id == "tercera division a") return "Tercera Division A";
    if (id == "tercera division b") return "Tercera Division B";
    return id;
}

string readLine(const string& prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    return trim(line);
}

int readInt(const string& prompt, int minVal, int maxVal) {
    while (true) {
        string line = readLine(prompt);
        if (line.empty()) continue;
        try {
            int val = stoi(line);
            if (val < minVal || val > maxVal) {
                cout << "Opcion invalida." << endl;
                continue;
            }
            return val;
        } catch (...) {
            cout << "Entrada invalida." << endl;
        }
    }
}

long long readLongLong(const string& prompt, long long minVal, long long maxVal) {
    while (true) {
        string line = readLine(prompt);
        if (line.empty()) continue;
        try {
            long long val = stoll(line);
            if (val < minVal || val > maxVal) {
                cout << "Opcion invalida." << endl;
                continue;
            }
            return val;
        } catch (...) {
            cout << "Entrada invalida." << endl;
        }
    }
}

string normalizeTeamId(const string& name) {
    string out;
    for (unsigned char c : name) {
        if (isalnum(c)) out.push_back(static_cast<char>(tolower(c)));
    }
    return out;
}

bool loadTeamsList(const string& divisionFolder, vector<pair<string, string>>& out) {
    string listPath = joinPath(divisionFolder, "teams.txt");
    if (!pathExists(listPath)) return false;
    vector<string> lines;
    if (!readTextFileLines(listPath, lines)) return false;

    for (string line : lines) {
        line = trim(line);
        if (line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = trim(line.substr(3));
        }
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        auto parts = splitByDelimiter(line, '|');
        if (parts.empty()) continue;
        string display = parts[0];
        string folder = (parts.size() >= 2) ? parts[1] : parts[0];
        display = trim(display);
        folder = trim(folder);
        if (display.empty() || folder.empty()) continue;
        out.push_back({display, folder});
    }
    return !out.empty();
}
