#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

// ============================================================================
// PHASE 4: Simplified Event System (TypeSafe Event Dispatching)
// ============================================================================
// Lightweight event dispatcher for game systems
// Solves: global state, circular dependencies, hard-to-test callbacks

namespace Events {

// Forward declarations of concrete event types
struct ManagerStressChangedEvent;
struct SeasonProgressedEvent;
struct MatchCompletedEvent;

// Type-safe event listeners for concrete event types
using ManagerStressListener = std::function<void(const ManagerStressChangedEvent&)>;
using SeasonListener = std::function<void(const SeasonProgressedEvent&)>;
using MatchListener = std::function<void(const MatchCompletedEvent&)>;

// Central event dispatcher (typeafe, no abstract types)
class EventDispatcher {
private:
    std::vector<ManagerStressListener> stressListeners_;
    std::vector<SeasonListener> seasonListeners_;
    std::vector<MatchListener> matchListeners_;

public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    // Subscribe to specific event types
    void subscribeToStress(ManagerStressListener listener) {
        stressListeners_.push_back(listener);
    }

    void subscribeToSeason(SeasonListener listener) {
        seasonListeners_.push_back(listener);
    }

    void subscribeToMatch(MatchListener listener) {
        matchListeners_.push_back(listener);
    }

    // Publish events (implementations below)
    void publishStress(const ManagerStressChangedEvent& evt);
    void publishSeason(const SeasonProgressedEvent& evt);
    void publishMatch(const MatchCompletedEvent& evt);

    // Clear listeners
    void clear() {
        stressListeners_.clear();
        seasonListeners_.clear();
        matchListeners_.clear();
    }
};

// ============================================================================
// CONCRETE EVENT TYPES
// ============================================================================

struct ManagerStressChangedEvent {
    int newStress = 0;
    std::string reason;
};

struct SeasonProgressedEvent {
    int season = 0;
    int week = 0;
};

struct MatchCompletedEvent {
    std::string homeTeam;
    std::string awayTeam;
    int homeScore = 0;
    int awayScore = 0;
};

struct RivalryCreatedEvent {
    std::string team1;
    std::string team2;
    int intensity = 0;
};

struct BudgetChangedEvent {
    std::string teamName;
    long long newBudget = 0;
    long long delta = 0;
};

struct PlayerInjuredEvent {
    std::string playerName;
    std::string teamName;
    int weeksOut = 0;
};

struct TransferCompletedEvent {
    std::string playerName;
    std::string fromTeam;
    std::string toTeam;
    long long fee = 0;
};

} // namespace Events
