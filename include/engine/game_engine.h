#pragma once

#include "career/career_manager.h"
#include "simulation/simulation.h"

class GameEngine {
public:
    GameEngine();

    CareerManager& careerManager();
    const CareerManager& careerManager() const;
    Career& career();
    const Career& career() const;

    void initialize(bool forceReload = false);
    ServiceResult startCareer(const std::string& divisionId,
                              const std::string& teamName,
                              const std::string& managerName);
    ServiceResult loadCareer();
    ServiceResult saveCareer();
    ServiceResult simulateCareerWeek(IdleCallback idleCallback = nullptr);
    MatchResult runQuickMatch(Team& home,
                              Team& away,
                              bool verbose,
                              bool keyMatch = false,
                              bool neutralVenue = false);

private:
    CareerManager careerManager_;
};
