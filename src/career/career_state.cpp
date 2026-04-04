#include "career/career_state.h"
#include "competition.h"

CareerState::CareerState()
    : currentSeason(0),
      currentWeek(0),
      activeDivision(""),
      saveFile(""),
      initialized(false) {}

bool CareerState::usesSegundaFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    // Note: This method now assumes team count is not available here; may need adjustment
    return config.seasonHandler == CompetitionSeasonHandler::SegundaGroups;
}

bool CareerState::usesTerceraBFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::TerceraB;
}

bool CareerState::usesGroupFormat() const {
    // Note: Need team count; placeholder
    return competitionUsesGroupStage(activeDivision, 0); // Adjust as needed
}

void CareerState::setActiveDivision(const std::string& id) {
    activeDivision = id;
}