#pragma once

#include "engine/models.h"
#include <string>
#include <vector>

// ============================================================================
// PHASE 3: Sub-Service Extraction (Career Struct Decomposition)
// ============================================================================
// Extract league/competition logic into separate service
// Part of breaking down the God Object

class LeagueManagementService {
private:
    Career& career_;

public:
    explicit LeagueManagementService(Career& career) : career_(career) {}

    // League table management
    void updateLeagueTable();
    void sortLeagueTable();
    std::vector<Team*> getTeamsInDivision(const std::string& division);
    
    // Cup competition management
    void advanceCupRound();
    void generateCupMatches();
    bool isCupActive() const;
    
    // Division management
    void assignTeamsToDivisions();
    std::string getTeamDivision(const Team* team) const;
    std::vector<DivisionInfo> getDivisions() const;
    
    // Relegation/Promotion
    void processPromotionsAndRelegations();
    void handleTeamRelegation(Team* team);
    void handleTeamPromotion(Team* team);
};

// ============================================================================
// Finance Service: Extract board/budget logic
// ============================================================================

class FinanceService {
private:
    Career& career_;

public:
    explicit FinanceService(Career& career) : career_(career) {}

    // Wage calculations and processing
    long long calculateWeeklyWages();
    void processWeeklyWages();
    
    // Sponsorship and revenue
    void processSponsorship();
    long long calculateSponsorshipRevenue();
    
    // Facility maintenance and upgrades
    void processFacilityMaintenance();
    long long getFacilityMaintenance() const;
    
    // Stadium revenue (match-day income)
    long long calculateMatchDayRevenue();
    
    // Board budget target compliance
    void checkBoardBudgetCompliance();
    int getBudgetComplianceStatus() const;
    
    // Debt management
    void processDebtInterests();
    long long getTotalDebt() const;
};

// ============================================================================
// UI State Service: Extract message/news logic
// ============================================================================

class UIStateService {
private:
    Career& career_;

public:
    explicit UIStateService(Career& career) : career_(career) {}

    // News feed management
    void addNews(const std::string& message);
    std::vector<std::string> getRecentNews(size_t count = 10);
    void clearNews();
    
    // Manager inbox management
    void addManagerMessage(const std::string& message);
    std::vector<std::string> getManagerMessages();
    void clearManagerMessages();
    
    // Scout inbox management
    void addScoutMessage(const std::string& message);
    std::vector<std::string> getScoutMessages();
    
    // Shortlist management
    void addToScoutingShortlist(const std::string& playerName);
    std::vector<std::string> getScoutingShortlist();
    void removeFromShortlist(const std::string& playerName);
};

// ============================================================================
// Match History Service: Extract match analysis and reports
// ============================================================================

class MatchHistoryService {
private:
    Career& career_;

public:
    explicit MatchHistoryService(Career& career) : career_(career) {}

    // Store match analysis
    void recordMatchAnalysis(const std::string& analysis);
    std::string getLastMatchAnalysis() const;
    
    // Store match events
    void recordMatchEvent(const std::string& event);
    std::vector<std::string> getMatchEvents() const;
    
    // Player of the match
    void setPlayerOfTheMatch(const std::string& playerName);
    std::string getPlayerOfTheMatch() const;
    
    // Match center snapshot
    void recordMatchCenter(const MatchCenterSnapshot& snapshot);
    MatchCenterSnapshot getLastMatchCenter() const;
};
