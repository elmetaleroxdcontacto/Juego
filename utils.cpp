#include "utils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <sys/stat.h>
#include <direct.h>
#include <windows.h>

using namespace std;

#define stat _stat
#define mkdir _mkdir

static std::mt19937 rng{std::random_device{}()};

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
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool isDirectory(const string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return (buffer.st_mode & S_IFDIR) != 0;
    }
    return false;
}

string joinPath(const string& a, const string& b) {
    if (a.empty()) return b;
    string result = a;
    if (!result.empty() && result.back() != '/' && result.back() != '\\') {
        result += "\\";
    }
    result += b;
    return result;
}

string pathFilename(const string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == string::npos) return path;
    return path.substr(pos + 1);
}

string pathExtension(const string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == string::npos || pos == 0) return "";
    return path.substr(pos);
}

vector<string> listDirectories(const string& root) {
    vector<string> out;
    if (!isDirectory(root)) return out;
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((root + "\\*").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return out;
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            string name(findData.cFileName);
            if (name != "." && name != "..") {
                out.push_back(joinPath(root, name));
            }
        }
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    return out;
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
    ifstream file(listPath);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
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
