#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>

namespace career_match_analysis {

void storeMatchAnalysis(Career& career,
                        const Team& home,
                        const Team& away,
                        const MatchResult& result,
                        bool cupMatch);

std::string buildLastMatchInsightText(const Career& career,
                                      std::size_t maxReportLines = 4,
                                      std::size_t maxEvents = 5);

}  // namespace career_match_analysis
