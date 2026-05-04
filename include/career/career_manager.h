#pragma once

#include "career/app_services.h"
#include "career/career_modules.h"
#include "career/career_state.h"
#include "engine/models.h"

class CareerManager {
public:
    CareerManager();

    Career& state();
    const Career& state() const;
    CareerState stateSnapshot() const;

    void initialize(bool forceReload = false);
    ServiceResult startNewCareer(const std::string& divisionId,
                                 const std::string& teamName,
                                 const std::string& managerName);
    ServiceResult loadCareer();
    ServiceResult saveCareer();
    SeasonStepResult simulateSeasonStep(IdleCallback idleCallback = nullptr);
    ServiceResult simulateWeek(IdleCallback idleCallback = nullptr);
    TeamRepository& teams();
    const TeamRepository& teams() const;
    SeasonManager& seasons();
    const SeasonManager& seasons() const;
    FinanceManager& finances();
    const FinanceManager& finances() const;
    TransferManager& transfers();
    const TransferManager& transfers() const;
    InboxManager& inbox();
    const InboxManager& inbox() const;
    NewsManager& news();
    const NewsManager& news() const;

private:
    Career career_;
    TeamRepository teamRepository_;
    SeasonManager seasonManager_;
    FinanceManager financeManager_;
    TransferManager transferManager_;
    InboxManager inboxManager_;
    NewsManager newsManager_;
};
