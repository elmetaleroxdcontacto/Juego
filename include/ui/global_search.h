#pragma once

#include <string>
#include <vector>

struct SearchResult {
    std::string playerName;
    std::string clubName;
    std::string position;
    int age;
    int skill;
    long long value;
};

struct Career;  // Forward declaration

namespace global_search {

class PlayerSearchEngine {
public:
    static void initialize(Career* career);
    static std::vector<SearchResult> search(const std::string& query);
    static std::vector<SearchResult> searchByPosition(const std::string& position);
    static std::vector<SearchResult> searchByClub(const std::string& clubName);
    static bool isInitialized();
    
private:
    static Career* g_career;
    static std::string toLower(const std::string& str);
    static bool matches(const std::string& text, const std::string& query);
};

}  // namespace global_search
