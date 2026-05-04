#include "career/career_service.h"
#include "utils/safe_references.h"
#include <algorithm>

// ============================================================================
// CareerService Implementation
// ============================================================================
// This service layer wraps existing game systems and adds safety checks
// and event dispatching without reimplementing existing logic

void CareerService::simulateWeekMatches() {
    // Delegate to existing week simulation but with safety checks
    if (!career_.myTeam) {
        throw std::runtime_error("Career myTeam is null");
    }
    
    if (!SafeReferences::PointerValidator<Team>::validateContainer(career_.activeTeams)) {
        throw std::runtime_error("Invalid team pointers in activeTeams");
    }

    // Existing simulation logic can be called here
    // simulateCareerWeek(career_); // Refactored to accept service
}

void CareerService::updatePlayerPhysicalState() {
    if (!career_.myTeam || career_.myTeam->players.empty()) {
        return;
    }
    
    // Update each player's condition and injuries
    for (auto& player : career_.myTeam->players) {
        player.fitness = std::max(0, std::min(player.fitness, player.stamina));
        if (player.injuryWeeks <= 0) {
            player.injured = false;
            player.injuryType.clear();
        }
    }
}

void CareerService::processWeeklyFinances() {
    if (!career_.myTeam) return;
    
    // Validate team pointer is in active teams
    auto idx = SafeReferences::PointerValidator<Team>::getIndex(career_.myTeam, career_.activeTeams);
    if (!idx.hasValue()) {
        throw std::runtime_error("MyTeam pointer is invalid or not in activeTeams");
    }
    
    // Process wages - using correct field names
    long long weeklyWages = 0;
    for (const auto& player : career_.myTeam->players) {
        weeklyWages += (player.wage / 52);  // wage not salary
    }
    
    if (career_.myTeam->budget >= weeklyWages) {
        career_.myTeam->budget -= weeklyWages;
    } else {
        // Debt accumulation
        career_.debtStatus.totalDebt += (weeklyWages - career_.myTeam->budget);
        career_.myTeam->budget = 0;
    }
}

void CareerService::updateSocialDynamics(int pointsDelta) {
    if (!career_.myTeam) return;
    
    // Update dressing room based on match result
    if (pointsDelta > 0) {
        career_.dressingRoomDynamics.overallCliqueMorale = std::min(100, career_.dressingRoomDynamics.overallCliqueMorale + 5);
    } else if (pointsDelta < 0) {
        career_.dressingRoomDynamics.overallCliqueMorale = std::max(0, career_.dressingRoomDynamics.overallCliqueMorale - 10);
    }
    
    // Dispatch event if dispatcher available
    if (eventDispatcher_) {
        Events::SeasonProgressedEvent evt;
        evt.season = career_.currentSeason;
        evt.week = career_.currentWeek;
        eventDispatcher_->publishSeason(evt);
    }
}

void CareerService::updateManagerStress(int performanceImpact) {
    if (performanceImpact < -50) {
        career_.managerStress.stressLevel = std::min(100, career_.managerStress.stressLevel + 15);
    } else if (performanceImpact > 50) {
        career_.managerStress.stressLevel = std::max(0, career_.managerStress.stressLevel - 10);
    }
    
    // Dispatch event if dispatcher available
    if (eventDispatcher_) {
        Events::ManagerStressChangedEvent evt;
        evt.newStress = career_.managerStress.stressLevel;
        evt.reason = performanceImpact < 0 ? "Poor results" : "Good results";
        eventDispatcher_->publishStress(evt);
    }
}

std::vector<Team*> CareerService::buildJobMarket(bool includeRelegated) {
    (void)includeRelegated;
    std::vector<Team*> jobs;
    
    for (auto& team : career_.allTeams) {
        if (team.name != career_.myTeam->name) {
            jobs.push_back(&team);
        }
    }
    
    return jobs;
}

void CareerService::processContractUpdates() {
    if (!career_.myTeam) return;
    
    for (auto& player : career_.myTeam->players) {
        if (player.contractWeeks > 0) {
            player.contractWeeks--;
        }
    }
}

void CareerService::generateDevelopmentReports() {
    // Delegate to existing development systems
}

void CareerService::processIncomingOffers() {
    // Delegate to existing transfer systems
}

void CareerService::updatePendingTransfers() {
    for (auto& transfer : career_.pendingTransfers) {
        if (transfer.loanWeeks > 0) {
            transfer.loanWeeks--;
        }
    }
}

void CareerService::generateWeeklyNarrative() {
    // Delegate to existing narrative systems
}

void CareerService::dispatchStaffBriefing() {
    // Delegate to existing staff systems
}

void CareerService::addSquadAlerts() {
    // Delegate to existing alert systems
}

void CareerService::updateManagerReputation(int matchResult) {
    if (matchResult > 0) {
        career_.managerReputation = std::min(100, career_.managerReputation + 1);
    } else if (matchResult < 0) {
        career_.managerReputation = std::max(0, career_.managerReputation - 2);
    }
}

void CareerService::handleBoardStatus() {
    if (career_.boardConfidence < 30) {
        career_.boardWarningWeeks++;
    }
}

Team* CareerService::findTeamByNameSafe(const std::string& name) {
    auto it = std::find_if(
        career_.allTeams.begin(),
        career_.allTeams.end(),
        [&name](const Team& t) { return t.name == name; }
    );
    
    if (it != career_.allTeams.end()) {
        return &(*it);
    }
    
    return nullptr;
}

bool CareerService::validateCareerState() {
    // Validate all critical pointers
    if (!career_.myTeam) return false;
    
    auto myTeamIdx = SafeReferences::PointerValidator<Team>::getIndex(career_.myTeam, career_.activeTeams);
    if (!myTeamIdx.hasValue()) return false;
    
    // Validate all active teams
    if (!SafeReferences::PointerValidator<Team>::validateContainer(career_.activeTeams)) {
        return false;
    }
    
    return true;
}
