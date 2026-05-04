#include "career/game_events_system.h"

#include <algorithm>
#include <ctime>

namespace {
std::vector<career_events::GameEvent> g_eventLog;
const size_t MAX_EVENTS = 100;
const int EVENT_RETENTION_DAYS = 7;
}

namespace career_events {

void EventNotificationSystem::recordEvent(EventType type, const std::string& title, const std::string& message) {
    GameEvent event;
    event.type = type;
    event.title = title;
    event.message = message;
    event.timestamp = std::time(nullptr);
    event.isRead = false;
    
    g_eventLog.push_back(event);
    
    if (g_eventLog.size() > MAX_EVENTS) {
        g_eventLog.erase(g_eventLog.begin());
    }
}

std::vector<GameEvent> EventNotificationSystem::getUnreadEvents() {
    std::vector<GameEvent> result;
    for (const auto& event : g_eventLog) {
        if (!event.isRead) {
            result.push_back(event);
        }
    }
    return result;
}

std::vector<GameEvent> EventNotificationSystem::getAllEvents() {
    return g_eventLog;
}

void EventNotificationSystem::markEventAsRead(size_t index) {
    if (index < g_eventLog.size()) {
        g_eventLog[index].isRead = true;
    }
}

void EventNotificationSystem::clearOldEvents() {
    time_t now = std::time(nullptr);
    time_t cutoff = now - (EVENT_RETENTION_DAYS * 24 * 60 * 60);
    
    auto it = std::remove_if(g_eventLog.begin(), g_eventLog.end(),
        [cutoff](const GameEvent& event) { return event.timestamp < cutoff && event.isRead; });
    g_eventLog.erase(it, g_eventLog.end());
}

int EventNotificationSystem::getUnreadCount() {
    return static_cast<int>(getUnreadEvents().size());
}

bool checkCareerMilestone(const Career& career, int& outMilestoneWeek) {
    if (!career.myTeam) return false;
    
    int totalWeeksPlayed = (career.currentSeason - 1) * 30 + career.currentWeek;
    
    std::vector<int> milestones = {10, 50, 100, 200, 500};
    
    for (int milestone : milestones) {
        if (totalWeeksPlayed == milestone) {
            outMilestoneWeek = milestone;
            return true;
        }
    }
    
    return false;
}

std::string GetMilestoneDescription(int weekNumber, const Career& career) {
    if (weekNumber < 0) return "¡Campeón de la temporada!";
    const std::string managerPrefix = career.managerName.empty() ? "" : (career.managerName + ": ");
    
    switch (weekNumber) {
        case 10: return managerPrefix + "10 semanas de carrera completadas.";
        case 50: return managerPrefix + "50 semanas de carrera. Ya eres veterano.";
        case 100: return managerPrefix + "100 semanas. Centenario de partidos bajo tu mando.";
        case 200: return managerPrefix + "200 semanas. Tu legado crece.";
        case 500: return managerPrefix + "500 semanas. Leyenda viviente del fútbol.";
        default: return managerPrefix + "Hito alcanzado.";
    }
}

}  // namespace career_events
