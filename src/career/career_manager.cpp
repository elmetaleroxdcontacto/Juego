#include "career/career_manager.h"

CareerManager::CareerManager()
    : career_(),
      teamRepository_(career_),
      seasonManager_(career_),
      financeManager_(career_),
      transferManager_(career_),
      inboxManager_(career_),
      newsManager_(career_) {}

Career& CareerManager::state() {
    return career_;
}

const Career& CareerManager::state() const {
    return career_;
}

CareerState CareerManager::stateSnapshot() const {
    return CareerState::fromCareer(career_);
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

SeasonStepResult CareerManager::simulateSeasonStep(IdleCallback idleCallback) {
    return seasonManager_.simulateWeek(idleCallback);
}

ServiceResult CareerManager::simulateWeek(IdleCallback idleCallback) {
    SeasonStepResult step = seasonManager_.simulateWeek(idleCallback);
    ServiceResult result;
    result.ok = step.ok;
    result.messages = step.week.messages;
    if (result.messages.empty()) result.messages.push_back(step.ok ? "Semana simulada." : "No se pudo simular la semana.");
    return result;
}

TeamRepository& CareerManager::teams() {
    return teamRepository_;
}

const TeamRepository& CareerManager::teams() const {
    return teamRepository_;
}

SeasonManager& CareerManager::seasons() {
    return seasonManager_;
}

const SeasonManager& CareerManager::seasons() const {
    return seasonManager_;
}

FinanceManager& CareerManager::finances() {
    return financeManager_;
}

const FinanceManager& CareerManager::finances() const {
    return financeManager_;
}

TransferManager& CareerManager::transfers() {
    return transferManager_;
}

const TransferManager& CareerManager::transfers() const {
    return transferManager_;
}

InboxManager& CareerManager::inbox() {
    return inboxManager_;
}

const InboxManager& CareerManager::inbox() const {
    return inboxManager_;
}

NewsManager& CareerManager::news() {
    return newsManager_;
}

const NewsManager& CareerManager::news() const {
    return newsManager_;
}
