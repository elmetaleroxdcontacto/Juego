#include "ui/global_search.h"

#include "engine/models.h"

#include <algorithm>
#include <cctype>

namespace global_search {

Career* PlayerSearchEngine::g_career = nullptr;

void PlayerSearchEngine::initialize(Career* career) {
    g_career = career;
}

bool PlayerSearchEngine::isInitialized() {
    return g_career != nullptr;
}

std::string PlayerSearchEngine::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool PlayerSearchEngine::matches(const std::string& text, const std::string& query) {
    if (query.empty()) return true;
    return toLower(text).find(toLower(query)) != std::string::npos;
}

std::vector<SearchResult> PlayerSearchEngine::search(const std::string& query) {
    std::vector<SearchResult> results;
    if (!g_career || query.empty()) return results;
    
    for (const auto& team : g_career->allTeams) {
        for (const auto& player : team.players) {
            if (matches(player.name, query) || matches(team.name, query)) {
                SearchResult result;
                result.playerName = player.name;
                result.clubName = team.name;
                result.position = player.position;
                result.age = player.age;
                result.skill = player.skill;
                result.value = player.value;
                results.push_back(result);
            }
        }
    }
    
    // Limitar a 20 resultados
    if (results.size() > 20) {
        results.resize(20);
    }
    
    return results;
}

std::vector<SearchResult> PlayerSearchEngine::searchByPosition(const std::string& position) {
    std::vector<SearchResult> results;
    if (!g_career) return results;
    
    for (const auto& team : g_career->allTeams) {
        for (const auto& player : team.players) {
            if (matches(player.position, position)) {
                SearchResult result;
                result.playerName = player.name;
                result.clubName = team.name;
                result.position = player.position;
                result.age = player.age;
                result.skill = player.skill;
                result.value = player.value;
                results.push_back(result);
            }
        }
    }
    
    return results;
}

std::vector<SearchResult> PlayerSearchEngine::searchByClub(const std::string& clubName) {
    std::vector<SearchResult> results;
    if (!g_career) return results;
    
    for (const auto& team : g_career->allTeams) {
        if (matches(team.name, clubName)) {
            for (const auto& player : team.players) {
                SearchResult result;
                result.playerName = player.name;
                result.clubName = team.name;
                result.position = player.position;
                result.age = player.age;
                result.skill = player.skill;
                result.value = player.value;
                results.push_back(result);
            }
        }
    }
    
    return results;
}

}  // namespace global_search
