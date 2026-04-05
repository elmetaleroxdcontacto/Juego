#include "utils/event_system.h"

namespace Events {

// EventDispatcher implementations
void EventDispatcher::publishStress(const ManagerStressChangedEvent& evt) {
    for (auto& listener : stressListeners_) {
        try {
            listener(evt);
        } catch (const std::exception&) {
            // Log error but don't crash other listeners
        }
    }
}

void EventDispatcher::publishSeason(const SeasonProgressedEvent& evt) {
    for (auto& listener : seasonListeners_) {
        try {
            listener(evt);
        } catch (const std::exception&) {
            // Log error but don't crash
        }
    }
}

void EventDispatcher::publishMatch(const MatchCompletedEvent& evt) {
    for (auto& listener : matchListeners_) {
        try {
            listener(evt);
        } catch (const std::exception&) {
            // Log error but don't crash
        }
    }
}

} // namespace Events
