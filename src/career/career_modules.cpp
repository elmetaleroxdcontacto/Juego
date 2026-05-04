#include "career/career_modules.h"

#include <algorithm>

using namespace std;

namespace {

template <typename T>
vector<T> tailCopy(const vector<T>& items, size_t limit) {
    if (limit == 0 || items.empty()) return {};
    const size_t start = items.size() > limit ? items.size() - limit : 0;
    return vector<T>(items.begin() + static_cast<long long>(start), items.end());
}

}  // namespace

TeamRepository::TeamRepository(Career& career) : career_(career) {}

Team* TeamRepository::getTeamById(TeamId id) const {
    return career_.getTeamById(id);
}

const Team* TeamRepository::getTeamByIdConst(TeamId id) const {
    return static_cast<const Career&>(career_).getTeamById(id);
}

TeamId TeamRepository::getTeamIdFor(const Team* team) const {
    return career_.getTeamIdFor(team);
}

TeamId TeamRepository::getTeamIdByName(const string& name) const {
    return career_.getTeamIdByName(name);
}

TeamId TeamRepository::getActiveTeamIdAt(int index) const {
    return career_.getActiveTeamIdAt(index);
}

Team* TeamRepository::getActiveTeamByScheduleIndex(int index) const {
    const TeamId id = career_.getActiveTeamIdAt(index);
    return career_.getTeamById(id);
}

vector<TeamId> TeamRepository::getDivisionTeamIds(const string& divisionId) const {
    vector<TeamId> ids;
    const vector<Team*> teams = career_.getDivisionTeams(divisionId);
    ids.reserve(teams.size());
    for (const Team* team : teams) {
        const TeamId id = career_.getTeamIdFor(team);
        if (id != kInvalidTeamId) ids.push_back(id);
    }
    return ids;
}

SeasonManager::SeasonManager(Career& career) : career_(career) {}

SeasonStepResult SeasonManager::simulateWeek(IdleCallback idleCallback) const {
    return simulateSeasonStepService(career_, idleCallback);
}

void SeasonManager::resetSeason() {
    career_.resetSeason();
}

void SeasonManager::advanceWeek() {
    ++career_.currentWeek;
}

FinanceManager::FinanceManager(Career& career) : career_(career) {}

long long FinanceManager::budgetFor(TeamId id) const {
    const Team* team = static_cast<const Career&>(career_).getTeamById(id);
    return team ? team->budget : 0;
}

long long FinanceManager::debtFor(TeamId id) const {
    const Team* team = static_cast<const Career&>(career_).getTeamById(id);
    return team ? team->debt : 0;
}

bool FinanceManager::canAfford(TeamId id, long long amount) const {
    const Team* team = static_cast<const Career&>(career_).getTeamById(id);
    return team && amount >= 0 && team->budget >= amount;
}

bool FinanceManager::adjustBudget(TeamId id, long long delta) {
    Team* team = career_.getTeamById(id);
    if (!team) return false;
    const long long nextBudget = team->budget + delta;
    if (nextBudget < 0) return false;
    team->budget = nextBudget;
    return true;
}

TransferManager::TransferManager(Career& career) : career_(career) {}

size_t TransferManager::pendingTransferCount() const {
    return career_.pendingTransfers.size();
}

void TransferManager::queueTransfer(const PendingTransfer& transfer) {
    career_.pendingTransfers.push_back(transfer);
}

void TransferManager::executeDueTransfers() {
    career_.executePendingTransfers();
}

InboxManager::InboxManager(Career& career) : career_(career) {}

void InboxManager::addManagerItem(const string& item, const string& channel) {
    career_.addInboxItem(item, channel);
}

void InboxManager::addScoutItem(const string& item) {
    if (item.empty()) return;
    career_.scoutInbox.push_back(item);
    if (career_.scoutInbox.size() > 60) {
        career_.scoutInbox.erase(career_.scoutInbox.begin(),
                                 career_.scoutInbox.begin() + static_cast<long long>(career_.scoutInbox.size() - 60));
    }
}

vector<string> InboxManager::recentManagerItems(size_t limit) const {
    return tailCopy(career_.managerInbox, limit);
}

vector<string> InboxManager::recentScoutItems(size_t limit) const {
    return tailCopy(career_.scoutInbox, limit);
}

NewsManager::NewsManager(Career& career) : career_(career) {}

void NewsManager::add(const string& item) {
    career_.addNews(item);
}

vector<string> NewsManager::recent(size_t limit) const {
    return tailCopy(career_.newsFeed, limit);
}

void NewsManager::clear() {
    career_.newsFeed.clear();
}
