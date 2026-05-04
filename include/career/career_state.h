#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

class CareerState {
public:
    int currentSeason;
    int currentWeek;
    std::string activeDivision;
    std::string saveFile;
    std::vector<DivisionInfo> divisions;
    bool initialized;

    CareerState();
    static CareerState fromCareer(const Career& career);
    void captureFrom(const Career& career);
    bool usesSegundaFormat() const;
    bool usesTerceraBFormat() const;
    bool usesGroupFormat() const;
    void setActiveDivision(const std::string& id);
};
