#pragma once

#include "career/app_services.h"
#include "engine/models.h"

class CareerManager {
public:
    CareerManager();

    Career& state();
    const Career& state() const;

    void initialize(bool forceReload = false);
    ServiceResult startNewCareer(const std::string& divisionId,
                                 const std::string& teamName,
                                 const std::string& managerName);
    ServiceResult loadCareer();
    ServiceResult saveCareer();
    SeasonStepResult simulateSeasonStep();
    ServiceResult simulateWeek();

private:
    Career career_;
};
