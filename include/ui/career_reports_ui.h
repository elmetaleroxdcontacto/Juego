#pragma once

#include "engine/models.h"

namespace ui_reports {

void displayCompetitionCenter(Career& career);
void displayBoardStatus(Career& career);
void displayNewsFeed(const Career& career);
void displaySeasonHistory(const Career& career);
void displayClubOperations(Career& career);

}  // namespace ui_reports
