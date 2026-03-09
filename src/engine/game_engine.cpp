#include "engine/game_engine.h"

GameEngine::GameEngine() = default;

CareerManager& GameEngine::careerManager() {
    return careerManager_;
}

const CareerManager& GameEngine::careerManager() const {
    return careerManager_;
}

Career& GameEngine::career() {
    return careerManager_.state();
}

const Career& GameEngine::career() const {
    return careerManager_.state();
}

void GameEngine::initialize(bool forceReload) {
    careerManager_.initialize(forceReload);
}

ServiceResult GameEngine::startCareer(const std::string& divisionId,
                                      const std::string& teamName,
                                      const std::string& managerName) {
    return careerManager_.startNewCareer(divisionId, teamName, managerName);
}

ServiceResult GameEngine::loadCareer() {
    return careerManager_.loadCareer();
}

ServiceResult GameEngine::saveCareer() {
    return careerManager_.saveCareer();
}

ServiceResult GameEngine::simulateCareerWeek() {
    return careerManager_.simulateWeek();
}

MatchResult GameEngine::runQuickMatch(Team& home,
                                      Team& away,
                                      bool verbose,
                                      bool keyMatch,
                                      bool neutralVenue) {
    return playMatch(home, away, verbose, keyMatch, neutralVenue);
}
