#include "career/sub_services.h"
#include <algorithm>
#include <numeric>

// ============================================================================
// LeagueManagementService Implementation
// ============================================================================

void LeagueManagementService::updateLeagueTable() {
    // Existing logic for updating league table
}

void LeagueManagementService::sortLeagueTable() {
    // Sort table by points, goal difference, goals scored
}

std::vector<Team*> LeagueManagementService::getTeamsInDivision(const std::string& division) {
    std::vector<Team*> teams;
    for (auto& team : career_.allTeams) {
        if (team.division == division) {
            teams.push_back(&team);
        }
    }
    return teams;
}

void LeagueManagementService::advanceCupRound() {
    if (career_.cupActive) {
        career_.cupRound++;
    }
}

void LeagueManagementService::generateCupMatches() {
    // Generate cup matchups for current round
}

bool LeagueManagementService::isCupActive() const {
    return career_.cupActive;
}

void LeagueManagementService::assignTeamsToDivisions() {
    // Assign teams to divisions based on league structure
}

std::string LeagueManagementService::getTeamDivision(const Team* team) const {
    if (!team) return "";
    return team->division;
}

std::vector<DivisionInfo> LeagueManagementService::getDivisions() const {
    return career_.divisions;
}

void LeagueManagementService::processPromotionsAndRelegations() {
    // Handle promotion/relegation between divisions
}

void LeagueManagementService::handleTeamRelegation(Team* team) {
    if (!team) return;
    // Move team to lower division
}

void LeagueManagementService::handleTeamPromotion(Team* team) {
    if (!team) return;
    // Move team to higher division
}

// ============================================================================
// FinanceService Implementation
// ============================================================================

long long FinanceService::calculateWeeklyWages() {
    if (!career_.myTeam) return 0;
    
    long long total = 0;
    for (const auto& player : career_.myTeam->players) {
        total += player.wage / 52;  // wage not salary
    }
    return total;
}

void FinanceService::processWeeklyWages() {
    long long wages = calculateWeeklyWages();
    
    if (career_.myTeam->budget >= wages) {
        career_.myTeam->budget -= wages;
    } else {
        career_.debtStatus.totalDebt += (wages - career_.myTeam->budget);
        career_.myTeam->budget = 0;
    }
}

void FinanceService::processSponsorship() {
    long long sponsorship = calculateSponsorshipRevenue();
    career_.myTeam->budget += sponsorship;
}

long long FinanceService::calculateSponsorshipRevenue() {
    // Base on team prestige and stadium level
    if (!career_.myTeam) return 0;
    return (long long)(career_.myTeam->clubPrestige * career_.myTeam->stadiumLevel * 100);
}

void FinanceService::processFacilityMaintenance() {
    long long maintenance = getFacilityMaintenance();
    career_.myTeam->budget -= maintenance;
}

long long FinanceService::getFacilityMaintenance() const {
    if (!career_.myTeam) return 0;
    
    long long cost = 0;
    cost += career_.infrastructure.levels.stadium * 5000;
    cost += career_.infrastructure.levels.youthAcademy * 3000;
    cost += career_.infrastructure.levels.trainingGround * 2000;
    
    return cost / 52; // Weekly
}

long long FinanceService::calculateMatchDayRevenue() {
    if (!career_.myTeam) return 0;
    
    // Based on attendance and ticket prices
    int baseCapacity = 20000 + (career_.myTeam->stadiumLevel * 5000); // Base + improvements
    int attendance = (baseCapacity * 70) / 100; // 70% average
    return (long long)attendance * 100; // $100 per ticket (simplified)
}

void FinanceService::checkBoardBudgetCompliance() {
    if (career_.myTeam->budget < career_.boardBudgetTarget) {
        career_.boardConfidence--;
    }
}

int FinanceService::getBudgetComplianceStatus() const {
    if (!career_.myTeam) return 0;
    
    if (career_.myTeam->budget >= career_.boardBudgetTarget * 1.2) return 2; // Exceeding
    if (career_.myTeam->budget >= career_.boardBudgetTarget) return 1; // On track
    return 0; // Below target
}

void FinanceService::processDebtInterests() {
    // Apply interest to debt
    if (career_.debtStatus.totalDebt > 0) {
        long long interest = (career_.debtStatus.totalDebt * 5) / 100 / 52; // 5% annual, weekly
        career_.debtStatus.totalDebt += interest;
    }
}

long long FinanceService::getTotalDebt() const {
    return career_.debtStatus.totalDebt;
}

// ============================================================================
// UIStateService Implementation
// ============================================================================

void UIStateService::addNews(const std::string& message) {
    career_.newsFeed.push_back(message);
    
    // Keep last 50 news items
    if (career_.newsFeed.size() > 50) {
        career_.newsFeed.erase(career_.newsFeed.begin());
    }
}

std::vector<std::string> UIStateService::getRecentNews(size_t count) {
    if (career_.newsFeed.empty()) return {};
    
    size_t start = (career_.newsFeed.size() > count) ? (career_.newsFeed.size() - count) : 0;
    return std::vector<std::string>(
        career_.newsFeed.begin() + start,
        career_.newsFeed.end()
    );
}

void UIStateService::clearNews() {
    career_.newsFeed.clear();
}

void UIStateService::addManagerMessage(const std::string& message) {
    career_.managerInbox.push_back(message);
}

std::vector<std::string> UIStateService::getManagerMessages() {
    return career_.managerInbox;
}

void UIStateService::clearManagerMessages() {
    career_.managerInbox.clear();
}

void UIStateService::addScoutMessage(const std::string& message) {
    career_.scoutInbox.push_back(message);
}

std::vector<std::string> UIStateService::getScoutMessages() {
    return career_.scoutInbox;
}

void UIStateService::addToScoutingShortlist(const std::string& playerName) {
    auto it = std::find(career_.scoutingShortlist.begin(), career_.scoutingShortlist.end(), playerName);
    if (it == career_.scoutingShortlist.end()) {
        career_.scoutingShortlist.push_back(playerName);
    }
}

std::vector<std::string> UIStateService::getScoutingShortlist() {
    return career_.scoutingShortlist;
}

void UIStateService::removeFromShortlist(const std::string& playerName) {
    auto it = std::find(career_.scoutingShortlist.begin(), career_.scoutingShortlist.end(), playerName);
    if (it != career_.scoutingShortlist.end()) {
        career_.scoutingShortlist.erase(it);
    }
}

// ============================================================================
// MatchHistoryService Implementation
// ============================================================================

void MatchHistoryService::recordMatchAnalysis(const std::string& analysis) {
    career_.lastMatchAnalysis = analysis;
}

std::string MatchHistoryService::getLastMatchAnalysis() const {
    return career_.lastMatchAnalysis;
}

void MatchHistoryService::recordMatchEvent(const std::string& event) {
    career_.lastMatchEvents.push_back(event);
    
    // Keep last 100 events
    if (career_.lastMatchEvents.size() > 100) {
        career_.lastMatchEvents.erase(career_.lastMatchEvents.begin());
    }
}

std::vector<std::string> MatchHistoryService::getMatchEvents() const {
    return career_.lastMatchEvents;
}

void MatchHistoryService::setPlayerOfTheMatch(const std::string& playerName) {
    career_.lastMatchPlayerOfTheMatch = playerName;
}

std::string MatchHistoryService::getPlayerOfTheMatch() const {
    return career_.lastMatchPlayerOfTheMatch;
}

void MatchHistoryService::recordMatchCenter(const MatchCenterSnapshot& snapshot) {
    career_.lastMatchCenter = snapshot;
}

MatchCenterSnapshot MatchHistoryService::getLastMatchCenter() const {
    return career_.lastMatchCenter;
}
