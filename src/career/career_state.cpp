#include "career/career_state.h"
#include "competition.h"

CareerState::CareerState()
    : currentSeason(0),
      currentWeek(0),
      activeDivision(""),
      activeTeamCount(0),
      saveFile(""),
      initialized(false) {}

CareerState CareerState::fromCareer(const Career& career) {
    CareerState state;
    state.captureFrom(career);
    return state;
}

void CareerState::captureFrom(const Career& career) {
    currentSeason = career.currentSeason;
    currentWeek = career.currentWeek;
    activeDivision = career.activeDivision;
    activeTeamCount = career.getActiveTeamCount();
    saveFile = career.saveFile;
    divisions = career.divisions;
    initialized = career.initialized;
}

bool CareerState::usesSegundaFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::SegundaGroups &&
           (config.expectedTeamCount <= 0 || activeTeamCount == config.expectedTeamCount);
}

bool CareerState::usesTerceraBFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::TerceraB &&
           (config.expectedTeamCount <= 0 || activeTeamCount == config.expectedTeamCount);
}

bool CareerState::usesGroupFormat() const {
    return competitionUsesGroupStage(activeDivision, activeTeamCount);
}

void CareerState::setActiveDivision(const std::string& id) {
    activeDivision = id;
}
