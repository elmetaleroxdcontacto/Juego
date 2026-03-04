#pragma once

#include <string>
#include <utility>
#include <vector>

int randInt(int minVal, int maxVal);
double rand01();

std::string trim(const std::string& s);
std::string toLower(const std::string& s);

bool pathExists(const std::string& path);
bool isDirectory(const std::string& path);
std::string joinPath(const std::string& a, const std::string& b);
std::string pathFilename(const std::string& path);
std::string pathExtension(const std::string& path);
std::vector<std::string> listDirectories(const std::string& root);

std::vector<std::string> splitCsvLine(const std::string& line);
std::vector<std::string> splitByDelimiter(const std::string& line, char delim);

int clampInt(int v, int lo, int hi);

std::string normalizePosition(std::string raw);
int parseAge(std::string s);
long long parseMarketValue(std::string s);
int computeSkillFromValue(long long value, int age, const std::string& division);
void getDivisionSkillRange(const std::string& division, int& minSkill, int& maxSkill);
std::string divisionDisplay(const std::string& id);

std::string readLine(const std::string& prompt);
int readInt(const std::string& prompt, int minVal, int maxVal);
long long readLongLong(const std::string& prompt, long long minVal, long long maxVal);

std::string normalizeTeamId(const std::string& name);
bool loadTeamsList(const std::string& divisionFolder, std::vector<std::pair<std::string, std::string>>& out);
