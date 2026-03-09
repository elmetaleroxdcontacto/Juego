#include "career/career_manager.h"

CareerManager::CareerManager() = default;

Career& CareerManager::state() {
    return career_;
}

const Career& CareerManager::state() const {
    return career_;
}

void CareerManager::initialize(bool forceReload) {
    career_.initializeLeague(forceReload);
}

ServiceResult CareerManager::startNewCareer(const std::string& divisionId,
                                            const std::string& teamName,
                                            const std::string& managerName) {
    return startCareerService(career_, divisionId, teamName, managerName);
}

ServiceResult CareerManager::loadCareer() {
    return loadCareerService(career_);
}

ServiceResult CareerManager::saveCareer() {
    return saveCareerService(career_);
}

ServiceResult CareerManager::simulateWeek() {
    return simulateCareerWeekService(career_);
}
