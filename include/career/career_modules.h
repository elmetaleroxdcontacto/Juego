#pragma once

#include "career/app_services.h"
#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

class TeamRepository {
public:
    explicit TeamRepository(Career& career);

    Team* getTeamById(TeamId id) const;
    const Team* getTeamByIdConst(TeamId id) const;
    TeamId getTeamIdFor(const Team* team) const;
    TeamId getTeamIdByName(const std::string& name) const;
    TeamId getActiveTeamIdAt(int index) const;
    Team* getActiveTeamByScheduleIndex(int index) const;
    std::vector<TeamId> getDivisionTeamIds(const std::string& divisionId) const;

private:
    Career& career_;
};

class SeasonManager {
public:
    explicit SeasonManager(Career& career);

    SeasonStepResult simulateWeek(IdleCallback idleCallback = nullptr) const;
    void resetSeason();
    void advanceWeek();

private:
    Career& career_;
};

class FinanceManager {
public:
    explicit FinanceManager(Career& career);

    long long budgetFor(TeamId id) const;
    long long debtFor(TeamId id) const;
    bool canAfford(TeamId id, long long amount) const;
    bool adjustBudget(TeamId id, long long delta);

private:
    Career& career_;
};

class TransferManager {
public:
    explicit TransferManager(Career& career);

    std::size_t pendingTransferCount() const;
    void queueTransfer(const PendingTransfer& transfer);
    void executeDueTransfers();

private:
    Career& career_;
};

class InboxManager {
public:
    explicit InboxManager(Career& career);

    void addManagerItem(const std::string& item, const std::string& channel = "");
    void addScoutItem(const std::string& item);
    std::vector<std::string> recentManagerItems(std::size_t limit = 10) const;
    std::vector<std::string> recentScoutItems(std::size_t limit = 10) const;

private:
    Career& career_;
};

class NewsManager {
public:
    explicit NewsManager(Career& career);

    void add(const std::string& item);
    std::vector<std::string> recent(std::size_t limit = 10) const;
    void clear();

private:
    Career& career_;
};
